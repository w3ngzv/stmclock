#ifndef CALENDAR_H
#define CALENDAR_H

#include <stdint.h>

typedef struct {
	uint16_t year;
	uint8_t month; /* 1 到 12 */
	uint8_t day;   /* 1 到 31 */
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} datetime_t;

#define YEAR_MIN 2000
#define YEAR_MAX 2099

int calendar_is_leap(int year);
int calendar_days_in_month(int year, int month);
/* 0 为星期日 */
int calendar_weekday(int year, int month, int day);
int datetime_valid(const datetime_t *t);
void datetime_tick(datetime_t *t);
/* 解析编译器给出的 __DATE__ ("Nov 21 2026") 和 __TIME__ ("22:50:01") */
void datetime_from_build(datetime_t *t, const char *date, const char *time);

#endif
