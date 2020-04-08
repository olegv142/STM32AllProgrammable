#pragma once

#include <stdint.h>

#include "usbd_def.h"
#include "usb_tmc_proto.h"

// Add safety margin to possible payload size and align to USB packet size
#define USB_TMC_RX_MAX_DATA_SZ (1024 + USB_HS_MAX_PACKET_SIZE - USB_TMC_HDR_SZ)
#define USB_TMC_TX_MAX_DATA_SZ (4096 + USB_HS_MAX_PACKET_SIZE - USB_TMC_HDR_SZ)

// Transmit data buffer
uint8_t* USB_TMC_TxDataBuffer(void);

// Message received
void USB_TMC_Receive(uint8_t const* pbuf, unsigned len);

// Input response requested
void USB_TMC_RequestResponse(uint8_t tag, unsigned max_len);

// Reply to host.
uint8_t USB_TMC_Reply(unsigned len, uint8_t tag);

// Asynchronous processing (in non-ISR context)
void USB_TMC_Process(void);
