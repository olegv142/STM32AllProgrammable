#pragma once

//
// String manipulation helpers
//

#define STRZ_LEN(s) (sizeof(s)-1)
#define PREFIX_MATCHED(pref, buf, len) (len >= STRZ_LEN(pref) && !strncmp(pref, (const char*)buf, STRZ_LEN(pref)))

// Convert value to single hexadecimal digit
static inline char to_hex(uint8_t v)
{
	return v >= 0xA ? 'A' + v - 0xA : '0' + v;
}

// Convert 8 bit value to hexadecimal representation
static inline void byte_to_hex(uint8_t v, char buf[2])
{
	buf[0] = to_hex(v >> 4);
	buf[1] = to_hex(v & 0xf);
}

// Convert 16 bit value to hexadecimal representation
static inline void u16_to_hex(uint16_t v, char buf[4])
{
	byte_to_hex(v >> 8, buf);
	byte_to_hex(v, buf + 2);
}

// Convert 32 bit value to hexadecimal representation
static inline void u32_to_hex(uint32_t v, char buf[8])
{
	u16_to_hex(v >> 16, buf);
	u16_to_hex(v, buf + 4);
}

// Scan buffer bytes until the specified character is found and skip it.
// Returns the number of bytes skipped or 0 if the character is not found.
// Use this function to find token prefixed by the given character.
static inline unsigned skip_through(char c, uint8_t const* buf, unsigned len)
{
	unsigned cnt;
	for (cnt = 0; len; --len, ++buf, ++cnt) {
		if (*buf == c)
			return cnt + 1;
		if (*buf == ':')
			break;
	}
	return 0;
}

// Read unsigned value from the buffer. The value may start from the radix prefix.
// Returns the number of bytes consumed or 0 if buffer does not start with the number.
static inline unsigned scan_u(uint8_t const* buf, unsigned len, unsigned* val)
{
	unsigned v = 0, radix = 10, cnt;
	for (cnt = 0; len; --len, ++buf, ++cnt)
	{
		unsigned char d;
		char c = *buf;
		if (!cnt) {
			switch (c) {
			case 'X':
			case 'x':
			case 'H':
			case 'h':
				radix = 16;
				continue;
			case 'Q':
				radix = 8;
				continue;
			case 'B':
				radix = 2;
				continue;
			default:
				;
			}
		}
		if (c < '0')
			break;
		if (radix <= 10 && c >= '0' + radix)
			break;
		if (c <= '9')
			d = c - '0';
		else if ('a' <= c && c <= 'f')
			d = c - 'a' + 0xa;
		else if ('A' <= c && c <= 'F')
			d = c - 'A' + 0xA;
		else
			break;
		v *= radix;
		v += d;
	}
	*val = v;
	return cnt;
}
