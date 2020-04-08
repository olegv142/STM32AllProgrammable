#pragma once

#include <stdint.h>

// Message received
void USB_TMC_Receive(uint8_t const* pbuf, unsigned len);

// Input response requested
void USB_TMC_RequestResponse(uint8_t tag, unsigned max_len);

// Reply to host
uint8_t USB_TMC_Reply(uint8_t const* pbuf, unsigned len, uint8_t tag);
