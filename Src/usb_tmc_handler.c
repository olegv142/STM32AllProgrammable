#include "usb_tmc.h"
#include "usb_tmc_commands.h"
#include "version.h"
#include "str_util.h"
#include "uuid.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

#define IDN_VENDOR_PRODUCT VENDOR "," PRODUCT ","

static char idn_buff[] = IDN_VENDOR_PRODUCT "****************," VERSION;
static const char* idn_ptr;

static void idn_init(void)
{
	char* sn_buff = idn_buff + STRZ_LEN(IDN_VENDOR_PRODUCT);
	u32_to_hex(UUID[0] + UUID[2], sn_buff);
	u32_to_hex(UUID[1], sn_buff + 8);
	idn_ptr = idn_buff;
}

typedef void (*tmc_handler_t)(void);

bool     tmc_pending;
bool     tmc_reply_rdy;
bool     tmc_reply_req;
unsigned tmc_reply_len;
unsigned tmc_reply_max_len;
uint8_t  tmc_reply_tag;
tmc_handler_t tmc_handler;

static inline void tmc_ready_to_reply(void)
{
	tmc_reply_rdy = true;
}

static inline void tmc_schedule_reply(unsigned len)
{
	tmc_pending = true;
	tmc_reply_len = len;
	tmc_ready_to_reply();
}

static void tmc_schedule_reply_buff(uint8_t const* buff, unsigned len)
{
	memcpy(USB_TMC_TxDataBuffer(), buff, len);
	tmc_schedule_reply(len);
}

static inline void tmc_schedule_handler(tmc_handler_t h)
{
	tmc_pending = true;
	tmc_handler = h;
}

static void tmc_idn_reply(void)
{
	if (!idn_ptr)
		idn_init();
	tmc_schedule_reply_buff((uint8_t const*)idn_ptr, STRZ_LEN(idn_buff));
}

// Error counters for debug
unsigned tmc_wr_ignored;
unsigned tmc_rd_empty;
unsigned tmc_errors;
unsigned tmc_overrun;

static void tmc_rx_std_command(uint8_t const* pbuf, unsigned len)
{
	if (PREFIX_MATCHED(CMD_IDN, pbuf, len)) {
		tmc_schedule_handler(tmc_idn_reply);
		return;
	}
	// unhanded command
	++tmc_wr_ignored;
}

static void tmc_pl_flash_handler(void)
{
	HAL_StatusTypeDef sta;
	uint8_t * buff = USB_TMC_TxDataBuffer();
	HAL_GPIO_WritePin(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin, GPIO_PIN_RESET);
	sta = HAL_SPI_TransmitReceive(
			&hspi3, buff, buff, tmc_reply_len, HAL_MAX_DELAY
		);
	HAL_GPIO_WritePin(FFLASH_CS_GPIO_Port, FFLASH_CS_Pin, GPIO_PIN_SET);
	if (sta != HAL_OK)
		++tmc_errors;
	tmc_ready_to_reply();
}

static void tmc_rx_pl_flash(uint8_t const* pbuf, unsigned len, unsigned rd_len)
{
	if (len + rd_len > USB_TMC_TX_MAX_DATA_SZ) {
		// unhanded command
		++tmc_wr_ignored;
		return;
	}
	memcpy(USB_TMC_TxDataBuffer(), pbuf, len);
	tmc_reply_len = len + rd_len;
	tmc_schedule_handler(tmc_pl_flash_handler);
}

static void tmc_rx_pl_flash_sub_command(uint8_t const* pbuf, unsigned len)
{
	unsigned skip, skip_len, rd_len;
	if (PREFIX_MATCHED(CMD_WR, pbuf, len)) {
		tmc_rx_pl_flash(pbuf + STRZ_LEN(CMD_WR), len - STRZ_LEN(CMD_WR), 0);
		return;
	}
	if (PREFIX_MATCHED(CMD_RD, pbuf, len)
		&& (skip = skip_through('#', pbuf, len))
		&& (skip_len = scan_u(pbuf + skip, len - skip, &rd_len))
		&& len > skip + skip_len && pbuf[skip + skip_len] == '#'
	) {
		tmc_rx_pl_flash(pbuf + skip + skip_len + 1, len - skip - skip_len - 1, rd_len);
		return;
	}
	// unhanded command
	++tmc_wr_ignored;
}

static void tmc_rx_pl_sub_command(uint8_t const* pbuf, unsigned len)
{
	unsigned skip;
	if (PREFIX_MATCHED(CMD_FLASH, pbuf, len) && (skip = skip_through(':', pbuf, len))) {
		tmc_rx_pl_flash_sub_command(pbuf + skip, len - skip);
		return;
	}
	// unhanded command
	++tmc_wr_ignored;
}

static void tmc_rx_test_echo(uint8_t const* pbuf, unsigned len)
{
	if (len > USB_TMC_TX_MAX_DATA_SZ)
		len = USB_TMC_TX_MAX_DATA_SZ;
	tmc_schedule_reply_buff(pbuf, len);
}

static void tmc_rx_test_sub_command(uint8_t const* pbuf, unsigned len)
{
	if (PREFIX_MATCHED(CMD_ECHO, pbuf, len)) {
		tmc_rx_test_echo(pbuf + STRZ_LEN(CMD_ECHO), len - STRZ_LEN(CMD_ECHO));
		return;
	}
	// unhanded command
	++tmc_wr_ignored;
}

static void tmc_rx_dev_command(uint8_t const* pbuf, unsigned len)
{
	unsigned skip;
	if (PREFIX_MATCHED(CMD_PL, pbuf, len) && (skip = skip_through(':', pbuf, len))) {
		tmc_rx_pl_sub_command(pbuf + skip, len - skip);
		return;
	}
	if (PREFIX_MATCHED(CMD_TEST, pbuf, len) && (skip = skip_through(':', pbuf, len))) {
		tmc_rx_test_sub_command(pbuf + skip, len - skip);
		return;
	}
	// unhanded command
	++tmc_wr_ignored;
}

void USB_TMC_Receive(uint8_t const* pbuf, unsigned len)
{
	if (tmc_pending) {
		++tmc_overrun;
	}
	if (!len) {
		++tmc_wr_ignored;
		return;
	}
	if (*pbuf == '*') {
		--len;
		++pbuf;
		tmc_rx_std_command(pbuf, len);
		return;
	}
	if (*pbuf == ':') {
		--len;
		++pbuf;
	}
	tmc_rx_dev_command(pbuf, len);
}

static void tmc_reply(void)
{
	unsigned len = tmc_reply_len <= tmc_reply_max_len ? tmc_reply_len : tmc_reply_max_len;
	tmc_reply_len = 0;
	tmc_reply_max_len = 0;
	tmc_reply_rdy = false;
	tmc_reply_req = false;
	tmc_pending = false;
	USB_TMC_Reply(len, tmc_reply_tag);
}

void USB_TMC_RequestResponse(uint8_t tag, unsigned max_len)
{
	if (!tmc_pending) {
		USB_TMC_Reply(0, tag);
		++tmc_rd_empty;
		return;
	}
	tmc_reply_tag = tag;
	tmc_reply_max_len = max_len;
	tmc_reply_req = true;
}

// Asynchronous processing (in non-ISR context)
void USB_TMC_Process(void)
{
	tmc_handler_t h = tmc_handler;
	if (h) {
		tmc_handler = 0;
		h();
	}
	if (tmc_reply_rdy && tmc_reply_req)
		tmc_reply();
}

extern uint8_t USBD_HS_DeviceDesc[];

void USB_TMC_init(void)
{
	USBD_HS_DeviceDesc[3] = 0x00; /*bDeviceClass: This is an Interface Class Defined Device*/
	USBD_HS_DeviceDesc[4] = 0x00; /*bDeviceSubClass*/
}
