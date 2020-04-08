#include "usb_tmc.h"
#include "usb_tmc_commands.h"
#include "version.h"
#include "str_util.h"
#include "uuid.h"
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

static void tmc_schedule_reply(unsigned len)
{
	tmc_pending = true;
	tmc_reply_len = len;
	tmc_reply_rdy = true;
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
unsigned tmc_overrun;

void tmc_rx_std_command(uint8_t const* pbuf, unsigned len)
{
	if (PREFIX_MATCHED(CMD_IDN, pbuf, len)) {
		tmc_schedule_handler(tmc_idn_reply);
		return;
	}
	// unhanded command
	++tmc_wr_ignored;
}

void tmc_rx_dev_command(uint8_t const* pbuf, unsigned len)
{
	if (PREFIX_MATCHED(CMD_ECHO, pbuf, len)) {
		pbuf += STRZ_LEN(CMD_ECHO);
		len  -= STRZ_LEN(CMD_ECHO);
		if (len > USB_TMC_TX_MAX_DATA_SZ)
			len = USB_TMC_TX_MAX_DATA_SZ;
		tmc_schedule_reply_buff(pbuf, len);
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
