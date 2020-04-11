#include "usb_tmc.h"
#include "usb_tmc_commands.h"
#include "version.h"
#include "str_util.h"
#include "uuid.h"
#include "pl.h"
#include "pl_flash.h"
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

static void tmc_idn_handler(void)
{
	if (!idn_ptr)
		idn_init();
	tmc_schedule_reply_buff((uint8_t const*)idn_ptr, STRZ_LEN(idn_buff));
}

// Error counters for debug
unsigned tmc_wr_ignored;
unsigned tmc_rd_empty;
unsigned tmc_rd_truncated;
unsigned tmc_overrun;

static void tmc_rx_std_command(uint8_t const* pbuf, unsigned len)
{
	if (PREFIX_MATCHED(CMD_IDN, pbuf, len)) {
		tmc_schedule_handler(tmc_idn_handler);
		return;
	}
	// unhanded command
	++tmc_wr_ignored;
}

// Flash transaction size
static unsigned tmc_pl_flash_tx_sz;
static unsigned tmc_pl_flash_wait;

static void tmc_pl_flash_tx_handler(void)
{
	pl_flash_tx(USB_TMC_TxDataBuffer(), tmc_pl_flash_tx_sz);
	tmc_ready_to_reply();
}

static void tmc_pl_flash_wait_handler(void)
{
	USB_TMC_TxDataBuffer()[0] = pl_flash_wait(tmc_pl_flash_wait);
	tmc_schedule_reply(1);
}

static void tmc_pl_flash_prog_handler(void)
{
	uint8_t wr_en_cmd = 0x06;
	uint8_t *buff = USB_TMC_TxDataBuffer();
	uint8_t status = pl_flash_wait(tmc_pl_flash_wait);
	if (!(status & FLASH_STATUS_BUSY)) {
		pl_flash_tx(&wr_en_cmd, 1);
		pl_flash_tx(buff, tmc_pl_flash_tx_sz);
	}
	buff[0] = status;
	tmc_schedule_reply(1);
}

static void tmc_pl_flash_tx(uint8_t const* pbuf, unsigned len, unsigned rd_len, bool rd_reply)
{
	if (rd_reply && len + rd_len > USB_TMC_TX_MAX_DATA_SZ) {
		// unhanded command
		++tmc_wr_ignored;
		return;
	}
	memcpy(USB_TMC_TxDataBuffer(), pbuf, len);
	tmc_pl_flash_tx_sz = len + rd_len;
	tmc_reply_len = rd_reply ? tmc_pl_flash_tx_sz : 0;
	tmc_schedule_handler(tmc_pl_flash_tx_handler);
}

static void tmc_pl_flash_prog(uint8_t const* pbuf, unsigned len, unsigned wait)
{
	memcpy(USB_TMC_TxDataBuffer(), pbuf, len);
	tmc_pl_flash_tx_sz = len;
	tmc_pl_flash_wait = wait;
	tmc_schedule_handler(tmc_pl_flash_prog_handler);
}

static void tmc_rx_pl_flash_sub_command(uint8_t const* pbuf, unsigned len)
{
	unsigned skip, skip_arg, arg;
	if (PREFIX_MATCHED(CMD_WR, pbuf, len)) {
		tmc_pl_flash_tx(pbuf + STRZ_LEN(CMD_WR), len - STRZ_LEN(CMD_WR), 0, false);
		return;
	}
	if (PREFIX_MATCHED(CMD_RD, pbuf, len)
		&& (skip = skip_through('#', pbuf, len))
		&& (skip_arg = scan_u(pbuf + skip, len - skip, &arg))
		&& len > skip + skip_arg && pbuf[skip + skip_arg] == '#'
	) {
		tmc_pl_flash_tx(pbuf + skip + skip_arg + 1, len - skip - skip_arg - 1, arg, true);
		return;
	}
	if (PREFIX_MATCHED(CMD_WAIT, pbuf, len)
		&& (skip = skip_through('#', pbuf, len))
		&& scan_u(pbuf + skip, len - skip, &arg)
	) {
		tmc_pl_flash_wait = arg;
		tmc_schedule_handler(tmc_pl_flash_wait_handler);
		return;
	}
	if (PREFIX_MATCHED(CMD_PROG, pbuf, len)
		&& (skip = skip_through('#', pbuf, len))
		&& (skip_arg = scan_u(pbuf + skip, len - skip, &arg))
		&& len > skip + skip_arg && pbuf[skip + skip_arg] == '#'
	) {
		tmc_pl_flash_prog(pbuf + skip + skip_arg + 1, len - skip - skip_arg - 1, arg);
		return;
	}
	// unhanded command
	++tmc_wr_ignored;
}

static inline void tmc_pl_report_status(void)
{
	uint8_t resp = '0' + pl_status;
	tmc_schedule_reply_buff(&resp, 1);
}

static void tmc_rx_pl_sub_command(uint8_t const* pbuf, unsigned len)
{
	unsigned skip, arg;
	if (PREFIX_MATCHED(CMD_ACTIVE, pbuf, len)) {
		if (len > STRZ_LEN(CMD_ACTIVE) && pbuf[STRZ_LEN(CMD_ACTIVE)] == '?') {
			tmc_pl_report_status();
			return;
		}
		if ((skip = skip_through('#', pbuf, len)) && scan_u(pbuf + skip, len - skip, &arg)) {
			pl_enable(arg != 0);
			return;
		}
	}

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
	unsigned len = tmc_reply_len;
	if (len > tmc_reply_max_len) {
		len = tmc_reply_max_len;
		++tmc_rd_truncated;
	}
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
