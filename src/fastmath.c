#include <stdint.h>

#include "fastmath.h"

#define PI_F          3.14159265f
#define RAD_TO_DEG    (180.0f / PI_F)
#define SQRT_MAGIC    0x1fbd1df5u

float fm_abs(float x)
{
	return x < 0.0f ? -x : x;
}

/*
 * 用整数运算给出平方根的初值 (相对误差约 3.5 %), 再做两次牛顿迭代,
 * 相对误差降到 1e-6 量级. 初值技巧见 Lomont 2003 "Fast Inverse Square Root".
 */
float fm_sqrt(float x)
{
	uint32_t bits;
	float y;

	if (x <= 0.0f)
		return 0.0f;
	y = x;
	__builtin_memcpy(&bits, &y, sizeof(bits));
	bits = SQRT_MAGIC + (bits >> 1);
	__builtin_memcpy(&y, &bits, sizeof(y));
	y = 0.5f * (y + x / y);
	y = 0.5f * (y + x / y);
	return y;
}

/*
 * |z| <= 1 的反正切, 弧度. 二次修正的多项式逼近 (Rajan 等 2006),
 * 最大误差约 0.0015 弧度, 即 0.09 度.
 */
static float atan_unit(float z)
{
	float a = fm_abs(z);

	return (PI_F / 4.0f) * z - z * (a - 1.0f) * (0.2447f + 0.0663f * a);
}

float fm_atan2_deg(float y, float x)
{
	float ax = fm_abs(x);
	float ay = fm_abs(y);
	float angle;

	if (ax == 0.0f && ay == 0.0f)
		return 0.0f;
	if (ax >= ay)
		angle = atan_unit(ay / ax) * RAD_TO_DEG;
	else
		angle = 90.0f - atan_unit(ax / ay) * RAD_TO_DEG;
	if (x < 0.0f)
		angle = 180.0f - angle;
	return y < 0.0f ? -angle : angle;
}
