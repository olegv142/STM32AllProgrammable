#include "usb_tmc.h"
#include "usb_tmc_commands.h"
#include "version.h"
#include "str_util.h"
#include "uuid.h"
#include <string.h>

typedef enum {
	tmc_idle,
	tmc_idn,
	tmc_echo,
} tmc_state_t;

static tmc_state_t tmc_state = tmc_idle;

#define IDN_VENDOR_PRODUCT VENDOR "," PRODUCT ","

static char idn_string[] = IDN_VENDOR_PRODUCT "****************," VERSION;
static unsigned const idn_len = STRZ_LEN(idn_string);
static const char* idn_ptr;

static void idn_init(void)
{
	char* sn_buff = idn_string + STRZ_LEN(IDN_VENDOR_PRODUCT);
	u32_to_hex(UUID[0] ^ UUID[1], sn_buff);
	u32_to_hex(UUID[2], sn_buff + 8);
	idn_ptr = idn_string;
}

static unsigned echo_len;

void USB_TMC_Receive(uint8_t const* pbuf, unsigned len)
{
	if (PREFIX_MATCHED(CMD_IDN, pbuf, len)) {
		tmc_state = tmc_idn;
	}
	else if (PREFIX_MATCHED(CMD_ECHO, pbuf, len)) {
		tmc_state = tmc_echo;
		pbuf += STRZ_LEN(CMD_ECHO);
		len  -= STRZ_LEN(CMD_ECHO);
		if (len > USB_TMC_TX_MAX_DATA_SZ)
			len = USB_TMC_TX_MAX_DATA_SZ;
		memcpy(USB_TMC_TxDataBuffer(), pbuf, echo_len = len);
	}
}

void USB_TMC_RequestResponse(uint8_t tag, unsigned max_len)
{
	switch (tmc_state) {
	case tmc_idn:
		if (!idn_ptr)
			idn_init();
		USB_TMC_Reply((uint8_t const*)idn_ptr, idn_len, tag);
		tmc_state = tmc_idle;
		break;
	case tmc_echo:
		if (echo_len > max_len)
			echo_len = max_len;
		USB_TMC_Reply(NULL, echo_len, tag);
		tmc_state = tmc_idle;
		break;
	}
}

