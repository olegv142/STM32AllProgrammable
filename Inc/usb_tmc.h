#pragma once

#include <stdint.h>

//
// Control requests specific to TMC class
//
#define USB_TMC_INITIATE_ABORT_BULK_OUT     1 // Required Aborts a Bulk-OUT transfer.
#define USB_TMC_CHECK_ABORT_BULK_OUT_STATUS 2 // Required Returns the status of the previously sent
                                              // INITIATE_ABORT_BULK_OUT request.
#define USB_TMC_INITIATE_ABORT_BULK_IN      3 // Required Aborts a Bulk-IN transfer.
#define USB_TMC_CHECK_ABORT_BULK_IN_STATUS  4 // Required Returns the status of the previously sent
                                              // INITIATE_ABORT_BULK_IN request.
#define USB_TMC_INITIATE_CLEAR              5 // Required Clears all previously sent pending and unprocessed Bulk-OUT USBTMC message
                                              // content and clears all pending Bulk-IN transfers from the USBTMC interface.
#define USB_TMC_CHECK_CLEAR_STATUS          6 // Required Returns the status of the previously sent INITIATE_CLEAR request.
#define USB_TMC_GET_CAPABILITIES            7


//
// Status codes
//
#define USB_TMC_STATUS_SUCCESS 0x01

// This status is valid if a device has received a USBTMC split transaction CHECK_STATUS
// request and the request is still being processed.
#define USB_TMC_STATUS_PENDING 0x02 

// Failure, unspecified reason, and a more specific USBTMC_status is not defined.
#define USB_TMC_STATUS_FAILED 0x80

// This status is only valid if a device has received an INITIATE_ABORT_BULK_OUT or
// INITIATE_ABORT_BULK_IN request and the specified transfer to abort is not in progress.
#define USB_TMC_STATUS_TRANSFER_NOT_IN_PROGRESS 0x81

// Failure This status is valid if the device received a CHECK_STATUS request and the device is not
// processing an INITIATE request.
#define USB_TMC_STATUS_SPLIT_NOT_IN_PROGRESS 0x82

// This status is valid if the device received a new class-specific request and the device is still processing an INITIATE.
#define USB_TMC_STATUS_SPLIT_IN_PROGRESS 0x83


//
// BULK message types (MsgID)
//
#define USB_TMC_DEV_DEP_MSG_OUT         1 // The USBTMC message is a USBTMC device dependent command message.
#define USB_TMC_REQUEST_DEV_DEP_MSG_IN  2 // The USBTMC message is a USBTMC command message that requests
                                          // the device to send a USBTMC response message on the Bulk-IN endpoint

// The USBTMC message is a USBTMC response message to the REQUEST_DEV_DEP_MSG_IN
#define USB_TMC_DEV_DEP_MSG_IN USB_TMC_REQUEST_DEV_DEP_MSG_IN

// The bulk endpoint message header size in bytes
#define USB_TMC_HDR_SZ 12


// Message received
void USB_TMC_Receive(uint8_t const* pbuf, unsigned len);

// Input response requested
void USB_TMC_RequestResponse(uint8_t tag, unsigned max_len);

// Reply to host
uint8_t USB_TMC_Reply(uint8_t const* pbuf, unsigned len, uint8_t tag);
