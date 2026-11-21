#include "watch.h"
#include "sys.h"

#define MS_PER_SEC 1000u

datetime_t g_now;
static uint32_t anchor_ms;

/*
 * 走时来自 SysTick, 即核心板上 8 MHz 晶振经 PLL 的 72 MHz, 晶振误差通常在
 * 数十 ppm, 一天约偏差 2 到 3 秒. 断电后时间回到编译时刻, 没有备份电池.
 * TODO: 核心板 PC14/PC15 旁若焊有 32.768 kHz 晶振, 可改用 RTC (RM0008 18),
 * 走时更准. 板上是否有这颗晶振还没有确认.
 */
void watch_init(void)
{
	datetime_from_build(&g_now, __DATE__, __TIME__);
	anchor_ms = g_ms;
}

int watch_tick(void)
{
	int ticked = 0;

	while (g_ms - anchor_ms >= MS_PER_SEC) {
		anchor_ms += MS_PER_SEC;
		datetime_tick(&g_now);
		ticked = 1;
	}
	return ticked;
}

void watch_set(const datetime_t *t)
{
	g_now = *t;
	anchor_ms = g_ms;
}
