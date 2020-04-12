#include "pl.h"
#include "main.h"

pl_status_t pl_status = pl_inactive;
bool volatile pl_enable_ = false;

static inline void pl_flash_cs_init(void)
{
	HAL_GPIO_WritePin(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin, GPIO_PIN_SET);
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = FFLASH_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(FFLASH_CS_GPIO_Port, &GPIO_InitStruct);
}

static inline void pl_flash_cs_deinit(void)
{
	HAL_GPIO_DeInit(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin);
}

static void pl_stop(void)
{
	HAL_GPIO_WritePin(PROGRAM_B_GPIO_Port, PROGRAM_B_Pin, GPIO_PIN_RESET);
	if (pl_status != pl_configured)
		HAL_SPI_MspInit(&hspi3);
	pl_flash_cs_init();
	pl_status = pl_inactive;
}

static void pl_resume(void)
{
	pl_flash_cs_deinit();
	HAL_SPI_MspDeInit(&hspi3);
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
		HAL_SPI_MspInit(&hspi3);
		pl_status = pl_configured;
	}
}