#include "fmt.h"

#define DEC_DIGITS_MAX 10

char *fmt_dec(char *dst, int32_t value, int min_digits)
{
	char tmp[DEC_DIGITS_MAX];
	int n = 0;
	uint32_t u;

	if (min_digits > DEC_DIGITS_MAX)
		min_digits = DEC_DIGITS_MAX;
	if (value < 0) {
		*dst++ = '-';
		u = (uint32_t)(-(value + 1)) + 1u; /* 避免 INT32_MIN 取反溢出 */
	} else {
		u = (uint32_t)value;
	}

	do {
		tmp[n++] = (char)('0' + u % 10u);
		u /= 10u;
	} while (u);
	while (n < min_digits)
		tmp[n++] = '0';
	while (n)
		*dst++ = tmp[--n];
	return dst;
}

char *fmt_tenths(char *dst, int32_t tenths)
{
	int32_t whole;
	int frac;

	if (tenths < 0) {
		*dst++ = '-';
		tenths = -tenths;
	}
	whole = tenths / 10;
	frac = (int)(tenths % 10);
	dst = fmt_dec(dst, whole, 1);
	*dst++ = '.';
	*dst++ = (char)('0' + frac);
	return dst;
}

char *fmt_hex(char *dst, uint32_t value, int digits)
{
	int i;

	for (i = digits - 1; i >= 0; i--)
		*dst++ = "0123456789ABCDEF"[(value >> (4 * i)) & 0xFu];
	return dst;
}

char *fmt_str(char *dst, const char *src)
{
	while (*src)
		*dst++ = *src++;
	return dst;
}
