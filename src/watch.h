#ifndef WATCH_H
#define WATCH_H

#include "calendar.h"

/* 当前日期时间, 只在主循环上下文中读写 */
extern datetime_t g_now;

void watch_init(void);
/* 补齐已经过去的整秒, 本次调用中走过至少一秒时返回 1 */
int watch_tick(void);
void watch_set(const datetime_t *t);

#endif
