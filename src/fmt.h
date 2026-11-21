#ifndef FMT_H
#define FMT_H

#include <stdint.h>

/* 写入十进制, 不足 min_digits 位时左补零, 返回写入结束处, 不写结尾 NUL */
char *fmt_dec(char *dst, int32_t value, int min_digits);
/* value 以 0.1 为单位, 写成 "-12.3" */
char *fmt_tenths(char *dst, int32_t tenths);
/* 大写十六进制, 固定 digits 位 */
char *fmt_hex(char *dst, uint32_t value, int digits);
char *fmt_str(char *dst, const char *src);

#endif
