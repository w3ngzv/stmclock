#include "calendar.h"

static const uint8_t month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int calendar_is_leap(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int calendar_days_in_month(int year, int month)
{
	if (month == 2 && calendar_is_leap(year))
		return 29;
	return month_days[month - 1];
}

/* Sakamoto 算法, 对公历全年份成立 */
int calendar_weekday(int year, int month, int day)
{
	static const uint8_t offset[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

	if (month < 3)
		year--;
	return (year + year / 4 - year / 100 + year / 400 + offset[month - 1] + day) % 7;
}

int datetime_valid(const datetime_t *t)
{
	if (t->year < YEAR_MIN || t->year > YEAR_MAX)
		return 0;
	if (t->month < 1 || t->month > 12)
		return 0;
	if (t->day < 1 || t->day > calendar_days_in_month(t->year, t->month))
		return 0;
	return t->hour < 24 && t->min < 60 && t->sec < 60;
}

void datetime_tick(datetime_t *t)
{
	if (++t->sec < 60)
		return;
	t->sec = 0;
	if (++t->min < 60)
		return;
	t->min = 0;
	if (++t->hour < 24)
		return;
	t->hour = 0;
	if (++t->day <= calendar_days_in_month(t->year, t->month))
		return;
	t->day = 1;
	if (++t->month <= 12)
		return;
	t->month = 1;
	if (++t->year > YEAR_MAX)
		t->year = YEAR_MIN;
}

static int two_digits(const char *s)
{
	int hi = s[0] == ' ' ? 0 : s[0] - '0';

	return hi * 10 + (s[1] - '0');
}

void datetime_from_build(datetime_t *t, const char *date, const char *time)
{
	static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	int m;

	t->month = 1;
	for (m = 0; m < 12; m++) {
		const char *p = &months[m * 3];

		if (date[0] == p[0] && date[1] == p[1] && date[2] == p[2]) {
			t->month = (uint8_t)(m + 1);
			break;
		}
	}
	t->day = (uint8_t)two_digits(date + 4);
	t->year = (uint16_t)(two_digits(date + 7) * 100 + two_digits(date + 9));
	t->hour = (uint8_t)two_digits(time);
	t->min = (uint8_t)two_digits(time + 3);
	t->sec = (uint8_t)two_digits(time + 6);
	if (!datetime_valid(t)) {
		t->year = YEAR_MIN;
		t->month = 1;
		t->day = 1;
		t->hour = 0;
		t->min = 0;
		t->sec = 0;
	}
}
