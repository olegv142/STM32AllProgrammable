#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	pl_inactive,
	pl_active,
	pl_configured
} pl_status_t;

extern pl_status_t pl_status;

// Start programmable logic configuration
void pl_start(void);

// Enable / disable PL
void pl_enable(bool en);

// Check PL status
void pl_process(void);

// Perform transaction on SPI channel
bool pl_tx(uint8_t* buff, unsigned len);

//
// Fast DCMI bus data exchange API
//

typedef enum {
	pl_pull_ready,
	pl_pull_busy,
	pl_pull_failed
} pl_pull_status_t;

// Start waiting for the data frame
bool pl_start_pull(uint8_t* buff, unsigned len);

// Get data frame reception status
pl_pull_status_t pl_get_pull_status(void);

// Stop waiting for the data frame
void pl_stop_pull(void);
