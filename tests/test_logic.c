/*
 * 主机上运行的单元测试, 只覆盖不依赖寄存器的纯逻辑.
 * 用法: make test
 */
#include <stdio.h>
#include <string.h>

#include "calendar.h"
#include "fmt.h"

static int failures;

#define CHECK(cond) do { \
	if (!(cond)) { \
		printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); \
		failures++; \
	} \
} while (0)

static void check_str(const char *got, const char *want, int line)
{
	if (strcmp(got, want)) {
		printf("FAIL line %d: got \"%s\" want \"%s\"\n", line, got, want);
		failures++;
	}
}
#define CHECK_STR(got, want) check_str((got), (want), __LINE__)

static void test_calendar(void)
{
	datetime_t t;

	CHECK(calendar_is_leap(2024));
	CHECK(!calendar_is_leap(2100));
	CHECK(calendar_is_leap(2000));
	CHECK(calendar_days_in_month(2024, 2) == 29);
	CHECK(calendar_days_in_month(2026, 2) == 28);
	CHECK(calendar_days_in_month(2026, 12) == 31);

	/* 2026-11-21 是星期六, 2024-02-29 是星期四, 2000-01-01 是星期六 */
	CHECK(calendar_weekday(2026, 11, 21) == 6);
	CHECK(calendar_weekday(2024, 2, 29) == 4);
	CHECK(calendar_weekday(2000, 1, 1) == 6);

	t = (datetime_t){ 2026, 12, 31, 23, 59, 59 };
	datetime_tick(&t);
	CHECK(t.year == 2027 && t.month == 1 && t.day == 1);
	CHECK(t.hour == 0 && t.min == 0 && t.sec == 0);

	t = (datetime_t){ 2024, 2, 28, 23, 59, 59 };
	datetime_tick(&t);
	CHECK(t.month == 2 && t.day == 29);

	t = (datetime_t){ 2024, 2, 29, 23, 59, 59 };
	datetime_tick(&t);
	CHECK(t.month == 3 && t.day == 1);

	t = (datetime_t){ 2099, 12, 31, 23, 59, 59 };
	datetime_tick(&t);
	CHECK(t.year == YEAR_MIN);

	t = (datetime_t){ 2026, 2, 29, 0, 0, 0 };
	CHECK(!datetime_valid(&t));
	t = (datetime_t){ 2026, 4, 31, 0, 0, 0 };
	CHECK(!datetime_valid(&t));
	t = (datetime_t){ 2026, 4, 30, 24, 0, 0 };
	CHECK(!datetime_valid(&t));
}

static void test_build_time(void)
{
	datetime_t t;

	datetime_from_build(&t, "Nov 21 2026", "22:50:01");
	CHECK(t.year == 2026 && t.month == 11 && t.day == 21);
	CHECK(t.hour == 22 && t.min == 50 && t.sec == 1);

	/* __DATE__ 的日期不足两位时前面是空格 */
	datetime_from_build(&t, "Jan  5 2027", "03:04:05");
	CHECK(t.year == 2027 && t.month == 1 && t.day == 5);

	/* 解析出非法日期时回到 2000-01-01 */
	datetime_from_build(&t, "Feb 30 2026", "00:00:00");
	CHECK(t.year == YEAR_MIN && t.month == 1 && t.day == 1);
}

static void test_fmt(void)
{
	char buf[32];
	char *p;

	p = fmt_dec(buf, 7, 2);
	*p = 0;
	CHECK_STR(buf, "07");
	p = fmt_dec(buf, 2026, 4);
	*p = 0;
	CHECK_STR(buf, "2026");
	p = fmt_dec(buf, 0, 1);
	*p = 0;
	CHECK_STR(buf, "0");
	p = fmt_dec(buf, -45, 1);
	*p = 0;
	CHECK_STR(buf, "-45");
	p = fmt_dec(buf, INT32_MIN, 1);
	*p = 0;
	CHECK_STR(buf, "-2147483648");
	p = fmt_tenths(buf, 253);
	*p = 0;
	CHECK_STR(buf, "25.3");
	p = fmt_tenths(buf, -5);
	*p = 0;
	CHECK_STR(buf, "-0.5");
	p = fmt_hex(buf, 0xEF4014, 6);
	*p = 0;
	CHECK_STR(buf, "EF4014");
}

int main(void)
{
	test_calendar();
	test_build_time();
	test_fmt();
	if (failures) {
		printf("%d failure(s)\n", failures);
		return 1;
	}
	printf("all tests passed\n");
	return 0;
}
