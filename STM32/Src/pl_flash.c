#include "pl_flash.h"
#include "main.h"

#include <stdbool.h>

// Error counter for debugging
unsigned pl_flash_errors;

// Select / deselect flash
static inline void pl_flash_cs(bool active)
{
	HAL_GPIO_WritePin(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin, active ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// Transmit data via SPI receiving response to the same buffer
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

// Wait non-busy status. Returns the last status received from the flash.
// So the FLASH_STATUS_BUSY will be set on result if operation is timed out.
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
