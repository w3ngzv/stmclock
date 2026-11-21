#include <stddef.h>

/*
 * 链接选项带 -nostdlib, 但 gcc 在复制结构体或初始化数组时仍可能生成
 * 对这三个函数的调用.
 */
void *memcpy(void *dst, const void *src, size_t n)
{
	unsigned char *d = dst;
	const unsigned char *s = src;

	while (n--)
		*d++ = *s++;
	return dst;
}

void *memset(void *dst, int c, size_t n)
{
	unsigned char *d = dst;

	while (n--)
		*d++ = (unsigned char)c;
	return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
	const unsigned char *pa = a;
	const unsigned char *pb = b;

	for (; n; n--, pa++, pb++) {
		if (*pa != *pb)
			return *pa - *pb;
	}
	return 0;
}
