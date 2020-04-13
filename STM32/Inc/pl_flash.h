#pragma once

#include <stdint.h>

//
// Programmable logic flash access functions
//

// Write buffer to flash SPI port and put response back to the buffer
void pl_flash_tx(uint8_t *buff, unsigned len);

#define FLASH_STATUS_BUSY 1

// Wait non-busy status. Returns the last status received from the flash.
// So the FLASH_STATUS_BUSY will be set on result if operation is timed out.
uint8_t pl_flash_wait(uint32_t tout);
