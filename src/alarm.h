#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>
#include "calendar.h"

typedef struct {
	uint8_t hour;
	uint8_t min;
	uint8_t enabled;
	uint8_t vibrate;   /* 响铃时舵机是否同时震动 */
} alarm_t;

extern alarm_t g_alarm;

/* 每个主循环调用一次, now 为当前时间, ms 为 g_ms */
void alarm_poll(const datetime_t *now, uint32_t ms);
int alarm_ringing(void);
void alarm_stop(void);

#endif
