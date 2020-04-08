#pragma once

#define STRZ_LEN(s) (sizeof(s)-1)
#define PREFIX_MATCHED(pref, buf, len) (len >= STRZ_LEN(pref) && !strncmp(pref, (const char*)buf, STRZ_LEN(pref)))

static inline char to_hex(uint8_t v)
{
	return v < 10 ? '0' + v : 'A' + v - 10;
}

static inline void byte_to_hex(uint8_t v, char buf[2])
{
	buf[0] = to_hex(v >> 4);
	buf[1] = to_hex(v & 0xf);
}

static inline void u16_to_hex(uint16_t v, char buf[4])
{
	byte_to_hex(v >> 8, buf);
	byte_to_hex(v, buf + 2);
}

static inline void u32_to_hex(uint32_t v, char buf[8])
{
	u16_to_hex(v >> 16, buf);
	u16_to_hex(v, buf + 4);
}

static inline unsigned skip_through(char c, uint8_t const* buf, unsigned len)
{
	unsigned cnt;
	for (cnt = 0; len; --len, ++buf, ++cnt) {
		if (*buf == c)
			return cnt + 1;
	}
	return 0;
}
