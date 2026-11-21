#include "app.h"
#include "fmt.h"
#include "input.h"
#include "oled.h"
#include "watch.h"

static const char *const weekday_name[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

#define BIG_SCALE   3
#define SEC_SCALE   2

static void clock_update(const input_state_t *in)
{
	char line[24];
	char *p;
	int w;

	if (in->keys & (KEYM(KEY_OK) | KEYM(KEY_ENC))) {
		app_menu();
		return;
	}

	gfx_clear();

	p = fmt_dec(line, g_now.year, 4);
	*p++ = '-';
	p = fmt_dec(p, g_now.month, 2);
	*p++ = '-';
	p = fmt_dec(p, g_now.day, 2);
	*p++ = ' ';
	p = fmt_str(p, weekday_name[calendar_weekday(g_now.year, g_now.month, g_now.day)]);
	*p = 0;
	gfx_text_center(OLED_WIDTH / 2, 2, line, 1, 1);

	p = fmt_dec(line, g_now.hour, 2);
	*p++ = ':';
	p = fmt_dec(p, g_now.min, 2);
	*p = 0;
	w = gfx_text(2, 20, line, BIG_SCALE, 1);

	p = fmt_dec(line, g_now.sec, 2);
	*p = 0;
	gfx_text(w + 2, 20 + (FONT_H * BIG_SCALE - FONT_H * SEC_SCALE), line, SEC_SCALE, 1);

	/* 底部状态行由后续功能追加 */
	gfx_hline(0, 52, OLED_WIDTH, 1);
}

const page_t page_clock = {
	.name = "CLOCK", .update = clock_update,
};
