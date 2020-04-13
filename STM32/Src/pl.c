#include "pl.h"
#include "main.h"

pl_status_t pl_status = pl_inactive;
bool volatile pl_enable_ = false;

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

static inline void pl_iface_deinit(void)
{
	HAL_GPIO_DeInit(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin);
	HAL_SPI_MspDeInit(&hspi3);
}

static void pl_stop(void)
{
	HAL_GPIO_WritePin(PROGRAM_B_GPIO_Port, PROGRAM_B_Pin, GPIO_PIN_RESET);
	if (pl_status != pl_configured)		
		pl_iface_init();
	pl_status = pl_inactive;
}

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

// Enable / disable PL
void pl_enable(bool en)
{
	pl_enable_ = en;
}

// Check PL status
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
}

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
