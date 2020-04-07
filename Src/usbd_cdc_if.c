/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @version        : v1.0_Cube
  * @brief          : Usb device for Virtual Com Port.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"

/* USER CODE BEGIN INCLUDE */
#include "usb_tmc.h"
#include <string.h>
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_CDC_IF
  * @{
  */

/** @defgroup USBD_CDC_IF_Private_TypesDefinitions USBD_CDC_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Defines USBD_CDC_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */
/* Define size for the receive and transmit buffer over CDC */
/* It's up to user to redefine and/or remove those define */
#define APP_RX_DATA_SIZE  2048
#define APP_TX_DATA_SIZE  2048

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Macros USBD_CDC_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Variables USBD_CDC_IF_Private_Variables
  * @brief Private variables.
  * @{
  */

/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/** Received data over USB are stored in this buffer      */
uint8_t UserRxBufferHS[APP_RX_DATA_SIZE];

/** Data to send over USB CDC are stored in this buffer   */
uint8_t UserTxBufferHS[APP_TX_DATA_SIZE];

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Variables USBD_CDC_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceHS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_FunctionPrototypes USBD_CDC_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CDC_Init_HS(void);
static int8_t CDC_DeInit_HS(void);
static int8_t CDC_Control_HS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_HS(uint8_t* pbuf, uint32_t *Len);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_CDC_ItfTypeDef USBD_Interface_fops_HS =
{
  CDC_Init_HS,
  CDC_DeInit_HS,
  CDC_Control_HS,
  CDC_Receive_HS
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the CDC media low layer over the USB HS IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_HS(void)
{
  /* USER CODE BEGIN 8 */
  /* Set Application Buffers */
  USBD_CDC_SetTxBuffer(&hUsbDeviceHS, UserTxBufferHS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceHS, UserRxBufferHS);
  return (USBD_OK);
  /* USER CODE END 8 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @param  None
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_HS(void)
{
  /* USER CODE BEGIN 9 */
  return (USBD_OK);
  /* USER CODE END 9 */
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
uint8_t  usb_tmc_last_cmd;
uint16_t usb_tmc_last_len;

static int8_t CDC_Control_HS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 10 */
  switch(cmd)
  {
  case USB_TMC_GET_CAPABILITIES:
    pbuf[0] = USB_TMC_STATUS_SUCCESS;
    pbuf[1] = 0;
    pbuf[2] = 0;
    pbuf[3] = 1; // Version 1.0
    pbuf[4] = 0;
    pbuf[5] = 0;
    pbuf[6] = 0;
    pbuf[7] = 0;
    pbuf[8] = 0;
    pbuf[9] = 0;
    pbuf[10] = 0;
    pbuf[11] = 0;
    pbuf[12] = 0;
    break;

  default:
    break;
  }
  usb_tmc_last_cmd = cmd;
  usb_tmc_last_len = length;
  return (USBD_OK);
  /* USER CODE END 10 */
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will block any OUT packet reception on USB endpoint
  *         untill exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result
  *         in receiving more data while previous ones are still not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
unsigned usb_tmc_rx_ignored = 0;

static int8_t CDC_Receive_HS(uint8_t* Buf, uint32_t *Len)
{
  /* USER CODE BEGIN 11 */
  uint8_t msg_type = UserRxBufferHS[0];
  uint32_t msg_len = *(uint32_t*)(UserRxBufferHS + 4);
  uint8_t* rx_buff = UserRxBufferHS;
  if (
    (msg_type != USB_TMC_DEV_DEP_MSG_OUT && msg_type != USB_TMC_REQUEST_DEV_DEP_MSG_IN) ||
    (0xff ^ UserRxBufferHS[1] ^ UserRxBufferHS[2]) || UserRxBufferHS[3] != 0
  ) {
    // Ignore bad packets
    ++usb_tmc_rx_ignored;
    goto rx_restart;
  }

  if (msg_type == USB_TMC_DEV_DEP_MSG_OUT) {
    if (USB_TMC_HDR_SZ + msg_len > sizeof(UserRxBufferHS)) {
      // Ignore bad packets
      ++usb_tmc_rx_ignored;
      goto rx_restart;
    }
    if (Buf + *Len >= UserRxBufferHS + USB_TMC_HDR_SZ + msg_len) {
      // packet receive competed
      USB_TMC_Receive(UserRxBufferHS + USB_TMC_HDR_SZ, msg_len);
    } else if (Buf + *Len + USB_HS_MAX_PACKET_SIZE <= UserRxBufferHS + sizeof(UserRxBufferHS)) {
      // receive more
      rx_buff = Buf + *Len;
    } else {
      ++usb_tmc_rx_ignored;
    }
  } else {
    // msg_type == USB_TMC_REQUEST_DEV_DEP_MSG_IN
    if (USB_TMC_HDR_SZ + msg_len > sizeof(UserTxBufferHS))
      msg_len = sizeof(UserTxBufferHS) - USB_TMC_HDR_SZ;
    USB_TMC_RequestResponse(UserRxBufferHS[1], msg_len);
  }

rx_restart:
  USBD_CDC_SetRxBuffer(&hUsbDeviceHS, rx_buff);
  USBD_CDC_ReceivePacket(&hUsbDeviceHS);
  return (USBD_OK);
  /* USER CODE END 11 */
}

/**
  * @brief  Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_HS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 12 */
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceHS.pClassData;
  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }
  USBD_CDC_SetTxBuffer(&hUsbDeviceHS, Buf, Len);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceHS);
  /* USER CODE END 12 */
  return result;
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

uint8_t USB_TMC_Reply(uint8_t const* pbuf, unsigned len, uint8_t tag)
{
  if (USB_TMC_HDR_SZ + len > sizeof(UserTxBufferHS))
    return USBD_FAIL;

  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceHS.pClassData;
  if (hcdc->TxState != 0)
    return USBD_BUSY;

  UserTxBufferHS[0] = USB_TMC_DEV_DEP_MSG_IN;
  UserTxBufferHS[1] = tag;
  UserTxBufferHS[2] = ~tag;
  UserTxBufferHS[3] = 0;
  *(uint32_t*)(UserTxBufferHS + 4) = len;
  *(uint32_t*)(UserTxBufferHS + 8) = 1;
  memcpy(UserTxBufferHS + USB_TMC_HDR_SZ, pbuf, len);

  USBD_CDC_SetTxBuffer(&hUsbDeviceHS, UserTxBufferHS, USB_TMC_HDR_SZ + len);
  return USBD_CDC_TransmitPacket(&hUsbDeviceHS);
}

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
