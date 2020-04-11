#pragma once

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


