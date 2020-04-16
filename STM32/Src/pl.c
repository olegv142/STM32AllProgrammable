#include "pl.h"
#include "main.h"

// The current PL status
pl_status_t pl_status = pl_inactive;

// Target PL status. May be set from ISR context and processed later.
bool volatile pl_enable_ = false;

// Initialize SPI interface to PL
static inline void pl_iface_init(void)
{
	HAL_SPI_MspInit(&hspi3);
	HAL_GPIO_WritePin(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin, GPIO_PIN_SET);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = FFLASH_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(FFLASH_CS_GPIO_Port, &GPIO_InitStruct);
}

// De-initialize SPI interface to PL
static inline void pl_iface_deinit(void)
{
	HAL_GPIO_DeInit(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin);
	HAL_SPI_MspDeInit(&hspi3);
}

// Reset PL by turning PROGRAM_B low
static void pl_stop(void)
{
	HAL_GPIO_WritePin(PROGRAM_B_GPIO_Port, PROGRAM_B_Pin, GPIO_PIN_RESET);
	if (pl_status != pl_configured)		
		pl_iface_init();
	pl_status = pl_inactive;
}

// Resume PL by turning PROGRAM_B high
static void pl_resume(void)
{
	pl_iface_deinit();
	HAL_GPIO_WritePin(PROGRAM_B_GPIO_Port, PROGRAM_B_Pin, GPIO_PIN_SET);
	pl_status = pl_active;
}

// Start programmable logic configuration
void pl_start(void)
{
	pl_enable_ = true;
	pl_resume();
}

// Enable / disable PL. May be called from ISR context.
// The actual status switch will be performed within pl_process() call.
void pl_enable(bool en)
{
	pl_enable_ = en;
}

// Check PL status. Called periodically in the main() loop.
void pl_process(void)
{
	bool is_active = (pl_status != pl_inactive);
	bool enable = pl_enable_;
	if (is_active != enable) {
		if (enable)
			pl_resume();
		else
			pl_stop();
	}
	else if (pl_status == pl_active) {
		if (HAL_GPIO_ReadPin(DONE_GPIO_Port, DONE_Pin) == GPIO_PIN_RESET)
			return;
		pl_iface_init();
		pl_status = pl_configured;
	}
	else if (pl_status == pl_configured) {
		if (HAL_GPIO_ReadPin(DONE_GPIO_Port, DONE_Pin) == GPIO_PIN_SET)
			return;
		// FPGA reset itself unexpectedly
		pl_stop();
	}
}

// Error counter for debugging
unsigned pl_tx_errors;

// Perform transaction on SPI channel
bool pl_tx(uint8_t* buff, unsigned len)
{
	if (pl_status != pl_configured) {
		++pl_tx_errors;
		return false;
	}
	HAL_GPIO_WritePin(PL_CS_GPIO_Port, PL_CS_Pin, GPIO_PIN_RESET);
	HAL_StatusTypeDef sta = HAL_SPI_TransmitReceive(
			&hspi3, buff, buff, len, HAL_MAX_DELAY
		);
	HAL_GPIO_WritePin(PL_CS_GPIO_Port, PL_CS_Pin, GPIO_PIN_SET);
	if (sta != HAL_OK) {
		++pl_tx_errors;
		return false;
	}
	return true;
}

// Start waiting for the DCMI data frame
bool pl_start_pull(uint8_t* buff, unsigned len)
{
	if (HAL_DCMI_GetState(&hdcmi) != HAL_DCMI_STATE_READY) {
		HAL_DCMI_Stop(&hdcmi);
		++pl_tx_errors;
	}
	// The length must be the integral number of 32 bit words
	// In theory it should work with any frame length but in practice it does not
	if (len % 4 || HAL_OK != HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)buff, len / 4)) {
		++pl_tx_errors;
		return false;
	}
	return true;
}

// Get DCMI data frame reception status
pl_pull_status_t pl_get_pull_status(void)
{
	switch (HAL_DCMI_GetState(&hdcmi)) {
	case HAL_DCMI_STATE_READY:
		return pl_pull_ready;
	case HAL_DCMI_STATE_BUSY:
		return pl_pull_busy;
	default:
		return pl_pull_failed;
	}
}

// Stop waiting for the DCMI data frame
void pl_stop_pull(void)
{
	HAL_DCMI_Stop(&hdcmi);
}

