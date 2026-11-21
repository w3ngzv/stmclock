#include "alarm.h"
#include "app.h"
#include "fmt.h"
#include "input.h"
#include "oled.h"

enum { F_HOUR, F_MIN, F_ENABLE, F_VIBRATE, F_COUNT };

static int field;

static void alarm_enter(void)
{
	field = F_HOUR;
}

static int wrap(int v, int hi)
{
	return ((v % (hi + 1)) + hi + 1) % (hi + 1);
}

static void alarm_update(const input_state_t *in)
{
	char line[8];
	char *p;
	int on_color;

	/* 修改立即生效, 退出页面不需要另外保存 */
	if (in->enc) {
		switch (field) {
		case F_HOUR:
			g_alarm.hour = (uint8_t)wrap(g_alarm.hour + in->enc, 23);
			break;
		case F_MIN:
			g_alarm.min = (uint8_t)wrap(g_alarm.min + in->enc, 59);
			break;
		case F_ENABLE:
			g_alarm.enabled = !g_alarm.enabled;
			break;
		default:
			g_alarm.vibrate = !g_alarm.vibrate;
			break;
		}
	}
	if (in->keys & (KEYM(KEY_OK) | KEYM(KEY_ENC)))
		field = (field + 1) % F_COUNT;

	gfx_clear();
	gfx_text(0, 2, "ALARM", 1, 1);

	p = fmt_dec(line, g_alarm.hour, 2);
	*p++ = ':';
	p = fmt_dec(p, g_alarm.min, 2);
	*p = 0;
	gfx_text(19, 16, line, 3, 1);
	if (field == F_HOUR)
		gfx_fill_rect(19, 40, 35, 2, 1);
	if (field == F_MIN)
		gfx_fill_rect(73, 40, 35, 2, 1);

	on_color = field == F_ENABLE;
	if (on_color)
		gfx_fill_rect(0, 47, 60, 10, 1);
	gfx_text(3, 49, g_alarm.enabled ? "ALARM ON" : "ALARM OFF", 1, !on_color);

	on_color = field == F_VIBRATE;
	if (on_color)
		gfx_fill_rect(66, 47, 62, 10, 1);
	gfx_text(69, 49, g_alarm.vibrate ? "VIB ON" : "VIB OFF", 1, !on_color);
}

const page_t page_alarm = {
	.name = "ALARM", .enter = alarm_enter, .update = alarm_update,
	.keep_awake = 1,
};
