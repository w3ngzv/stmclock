/*
 * 主机上运行的单元测试, 只覆盖不依赖寄存器的纯逻辑.
 * 用法: make test
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "calendar.h"
#include "cmd.h"
#include "fastmath.h"
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

static void test_fastmath(void)
{
	double worst_angle = 0.0, worst_sqrt = 0.0;
	int xi, yi;
	float v;

	for (yi = -20; yi <= 20; yi++) {
		for (xi = -20; xi <= 20; xi++) {
			double want, got, err;

			if (xi == 0 && yi == 0)
				continue;
			want = atan2((double)yi, (double)xi) * 180.0 / M_PI;
			got = (double)fm_atan2_deg((float)yi, (float)xi);
			err = fabs(want - got);
			if (err > 180.0)
				err = 360.0 - err; /* +-180 度处的等价角 */
			if (err > worst_angle)
				worst_angle = err;
		}
	}
	/* 近似公式的最大误差约 0.09 度 */
	CHECK(worst_angle < 0.15);

	for (v = 0.001f; v < 100000.0f; v *= 1.37f) {
		double rel = fabs((double)fm_sqrt(v) - sqrt((double)v)) / sqrt((double)v);

		if (rel > worst_sqrt)
			worst_sqrt = rel;
	}
	CHECK(worst_sqrt < 1e-5);
	CHECK(fm_sqrt(0.0f) == 0.0f);
	CHECK(fm_sqrt(-4.0f) == 0.0f);
	CHECK(fm_atan2_deg(0.0f, 0.0f) == 0.0f);
	printf("fastmath worst atan2 error %.4f deg, sqrt relative %.2e\n", worst_angle, worst_sqrt);
}

static void test_cmd(void)
{
	cmd_t c;

	c = cmd_parse("TIME 07:05:09");
	CHECK(c.type == CMD_TIME && c.a == 7 && c.b == 5 && c.c == 9);
	c = cmd_parse("time 23:59");
	CHECK(c.type == CMD_TIME && c.a == 23 && c.b == 59 && c.c == 0);
	c = cmd_parse("TIME 24:00");
	CHECK(c.type == CMD_BAD);
	c = cmd_parse("TIME 12:60");
	CHECK(c.type == CMD_BAD);
	c = cmd_parse("TIMEX 12:00");
	CHECK(c.type == CMD_BAD);

	c = cmd_parse("DATE 2026-11-21");
	CHECK(c.type == CMD_DATE && c.a == 2026 && c.b == 11 && c.c == 21);
	c = cmd_parse("DATE 2026-02-29");
	CHECK(c.type == CMD_BAD);
	c = cmd_parse("DATE 2024-02-29");
	CHECK(c.type == CMD_DATE);
	c = cmd_parse("DATE 1999-01-01");
	CHECK(c.type == CMD_BAD);

	c = cmd_parse("ALARM 06:45");
	CHECK(c.type == CMD_ALARM_SET && c.a == 6 && c.b == 45);
	CHECK(cmd_parse("alarm on").type == CMD_ALARM_ON);
	CHECK(cmd_parse("ALARM OFF").type == CMD_ALARM_OFF);
	CHECK(cmd_parse("ALARM 25:00").type == CMD_BAD);
	CHECK(cmd_parse("ALARM ON now").type == CMD_BAD);

	CHECK(cmd_parse("VIB ON").type == CMD_VIB_ON);
	CHECK(cmd_parse("VIB OFF").type == CMD_VIB_OFF);
	CHECK(cmd_parse("VIB TEST").type == CMD_VIB_TEST);
	CHECK(cmd_parse("VIB").type == CMD_BAD);
	CHECK(cmd_parse("BEEP TEST").type == CMD_BEEP_TEST);
	CHECK(cmd_parse("  status  ").type == CMD_STATUS);
	CHECK(cmd_parse("IMGW").type == CMD_IMGW);

	/* 模块回显的 OK 和 ERR 不能触发回复 */
	CHECK(cmd_parse("OK").type == CMD_NONE);
	CHECK(cmd_parse("ERR").type == CMD_NONE);
	CHECK(cmd_parse("").type == CMD_NONE);
	CHECK(cmd_parse("   ").type == CMD_NONE);
	CHECK(cmd_parse("HELLO").type == CMD_BAD);
}

static void test_alarm(void)
{
	datetime_t t = { 2026, 11, 21, 7, 29, 59 };
	uint32_t ms = 1000;

	g_alarm.hour = 7;
	g_alarm.min = 30;
	g_alarm.enabled = 1;

	alarm_poll(&t, ms);
	CHECK(!alarm_ringing());

	t.min = 30;
	t.sec = 0;
	alarm_poll(&t, ms);
	CHECK(alarm_ringing());

	/* 按停后同一分钟内不再响 */
	alarm_stop();
	t.sec = 30;
	alarm_poll(&t, ms + 30000);
	CHECK(!alarm_ringing());

	/* 第二天同一时刻再次响 */
	t.day = 22;
	t.min = 31;
	alarm_poll(&t, ms + 60000);
	t.min = 30;
	t.sec = 0;
	alarm_poll(&t, ms + 86400000u);
	CHECK(alarm_ringing());

	/* 没人理会时 60 秒后自动停止 */
	alarm_poll(&t, ms + 86400000u + 59999u);
	CHECK(alarm_ringing());
	alarm_poll(&t, ms + 86400000u + 60000u);
	CHECK(!alarm_ringing());

	/* 关闭闹钟后不响 */
	g_alarm.enabled = 0;
	t.min = 31;
	alarm_poll(&t, ms + 90000000u);
	t.min = 30;
	alarm_poll(&t, ms + 100000000u);
	CHECK(!alarm_ringing());
}

int main(void)
{
	test_calendar();
	test_build_time();
	test_fmt();
	test_fastmath();
	test_cmd();
	test_alarm();
	if (failures) {
		printf("%d failure(s)\n", failures);
		return 1;
	}
	printf("all tests passed\n");
	return 0;
}
