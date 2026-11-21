#include "alarm.h"

#define ALARM_RING_MAX_MS 60000u

alarm_t g_alarm = { .hour = 7, .min = 30, .enabled = 0, .vibrate = 1 };

static int ringing;
static int fired;       /* 本次匹配的那一分钟内已经响过 */
static uint32_t ring_start_ms;

void alarm_poll(const datetime_t *now, uint32_t ms)
{
	int match = g_alarm.enabled && now->hour == g_alarm.hour && now->min == g_alarm.min;

	/*
	 * 用"离开匹配分钟时清除 fired"代替记录日期: 用户在响铃那一分钟内按停后
	 * 不会再响, 第二天同一时刻又可以响.
	 */
	if (!match) {
		fired = 0;
	} else if (!fired) {
		fired = 1;
		ringing = 1;
		ring_start_ms = ms;
	}

	if (ringing && ms - ring_start_ms >= ALARM_RING_MAX_MS)
		ringing = 0;
}

int alarm_ringing(void)
{
	return ringing;
}

void alarm_stop(void)
{
	ringing = 0;
}
