#include "pl_flash.h"
#include "main.h"

#include <stdbool.h>

unsigned pl_flash_errors;

static inline void pl_flash_cs(bool active)
{
	HAL_GPIO_WritePin(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin, active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void pl_flash_tx(uint8_t *buff, unsigned len)
{
	HAL_StatusTypeDef sta;
	pl_flash_cs(true);
	sta = HAL_SPI_TransmitReceive(
			&hspi3, buff, buff, len, HAL_MAX_DELAY
		);
	pl_flash_cs(false);
	if (sta != HAL_OK)
		++pl_flash_errors;
}

uint8_t pl_flash_wait(uint32_t tout)
{
	HAL_StatusTypeDef sta;
	uint8_t cmd = 0x5, status = FLASH_STATUS_BUSY;
	uint32_t start = HAL_GetTick();
	pl_flash_cs(true);
	sta = HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);
	if (sta != HAL_OK) {
		++pl_flash_errors;
		goto done;
	}
	for (;;)  {
		sta = HAL_SPI_Receive(&hspi3, &status, 1, HAL_MAX_DELAY);
		if (sta != HAL_OK) {
			++pl_flash_errors;
			goto done;
		}
		if (!(status & FLASH_STATUS_BUSY))
			break;
		if (HAL_GetTick() - start > tout)
			break;
	}
done:
	pl_flash_cs(false);
	return status;
}
