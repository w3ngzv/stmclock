#include "app.h"
#include "fmt.h"
#include "input.h"
#include "oled.h"
#include "watch.h"

enum { F_YEAR, F_MONTH, F_DAY, F_HOUR, F_MIN, F_SEC, F_COUNT };

static datetime_t edit;
static int field;

static void settime_enter(void)
{
	edit = g_now;
	field = F_YEAR;
}

static int clamp_wrap(int v, int lo, int hi)
{
	if (v < lo)
		return hi - (lo - v - 1) % (hi - lo + 1);
	if (v > hi)
		return lo + (v - hi - 1) % (hi - lo + 1);
	return v;
}

static void step_field(int delta)
{
	switch (field) {
	case F_YEAR:
		edit.year = (uint16_t)clamp_wrap(edit.year + delta, YEAR_MIN, YEAR_MAX);
		break;
	case F_MONTH:
		edit.month = (uint8_t)clamp_wrap(edit.month + delta, 1, 12);
		break;
	case F_DAY:
		edit.day = (uint8_t)clamp_wrap(edit.day + delta, 1,
					       calendar_days_in_month(edit.year, edit.month));
		break;
	case F_HOUR:
		edit.hour = (uint8_t)clamp_wrap(edit.hour + delta, 0, 23);
		break;
	case F_MIN:
		edit.min = (uint8_t)clamp_wrap(edit.min + delta, 0, 59);
		break;
	default:
		edit.sec = (uint8_t)clamp_wrap(edit.sec + delta, 0, 59);
		break;
	}

	/* 改年或月之后, 日可能超出当月天数 */
	if (edit.day > calendar_days_in_month(edit.year, edit.month))
		edit.day = (uint8_t)calendar_days_in_month(edit.year, edit.month);
}

/* 字段在文本行内的起始字符序号与长度, 日期行和时间行各 3 个字段 */
static const struct { uint8_t row, start, len; } field_pos[F_COUNT] = {
	[F_YEAR]  = { 0, 0, 4 },
	[F_MONTH] = { 0, 5, 2 },
	[F_DAY]   = { 0, 8, 2 },
	[F_HOUR]  = { 1, 0, 2 },
	[F_MIN]   = { 1, 3, 2 },
	[F_SEC]   = { 1, 6, 2 },
};

#define SET_SCALE   2
#define DATE_Y      16
#define TIME_Y      38
#define DATE_X      4
#define TIME_X      16

static void settime_update(const input_state_t *in)
{
	char line[16];
	char *p;
	int cell = FONT_ADVANCE * SET_SCALE;
	int row_y, x0;

	if (in->enc)
		step_field(in->enc);

	if (in->keys & (KEYM(KEY_OK) | KEYM(KEY_ENC))) {
		if (++field >= F_COUNT) {
			watch_set(&edit);
			app_goto(&page_clock);
			return;
		}
	}

	gfx_clear();
	gfx_text(0, 2, "SET DATE TIME", 1, 1);

	p = fmt_dec(line, edit.year, 4);
	*p++ = '-';
	p = fmt_dec(p, edit.month, 2);
	*p++ = '-';
	p = fmt_dec(p, edit.day, 2);
	*p = 0;
	gfx_text(DATE_X, DATE_Y, line, SET_SCALE, 1);

	p = fmt_dec(line, edit.hour, 2);
	*p++ = ':';
	p = fmt_dec(p, edit.min, 2);
	*p++ = ':';
	p = fmt_dec(p, edit.sec, 2);
	*p = 0;
	gfx_text(TIME_X, TIME_Y, line, SET_SCALE, 1);

	row_y = (field_pos[field].row ? TIME_Y : DATE_Y) + FONT_H * SET_SCALE + 2;
	x0 = (field_pos[field].row ? TIME_X : DATE_X) + field_pos[field].start * cell;
	gfx_fill_rect(x0, row_y, field_pos[field].len * cell - SET_SCALE, 2, 1);

	gfx_text(0, 56, "ENC:ADJ OK:NEXT", 1, 1);
}

const page_t page_settime = {
	.name = "SET TIME", .enter = settime_enter, .update = settime_update,
	.keep_awake = 1,
};
