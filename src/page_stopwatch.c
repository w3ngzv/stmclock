#include "app.h"
#include "fmt.h"
#include "input.h"
#include "oled.h"
#include "sys.h"

#define MS_PER_MIN  60000u
#define MS_PER_SEC  1000u
#define MS_PER_CS   10u
#define SW_MIN_MAX  99u
#define SW_SCALE    2

static int running;
static uint32_t started_ms;   /* 本次运行开始时的 g_ms */
static uint32_t elapsed_ms;   /* 之前各段运行累计 */
static uint32_t lap_ms;
static int has_lap;

static uint32_t total_ms(void)
{
	return elapsed_ms + (running ? g_ms - started_ms : 0);
}

static char *fmt_stamp(char *dst, uint32_t ms)
{
	uint32_t min = ms / MS_PER_MIN;

	if (min > SW_MIN_MAX)
		min = SW_MIN_MAX; /* 超过 99 分钟时显示封顶, 计时本身继续 */
	dst = fmt_dec(dst, (int32_t)min, 2);
	*dst++ = ':';
	dst = fmt_dec(dst, (int32_t)(ms % MS_PER_MIN / MS_PER_SEC), 2);
	*dst++ = '.';
	return fmt_dec(dst, (int32_t)(ms % MS_PER_SEC / MS_PER_CS), 2);
}

static void sw_enter(void)
{
	/* 离开页面时不停表, 重新进入后继续显示 */
}

static void sw_update(const input_state_t *in)
{
	char line[16];
	char *p;

	if (in->keys & (KEYM(KEY_OK) | KEYM(KEY_ENC))) {
		if (running) {
			elapsed_ms += g_ms - started_ms;
			running = 0;
		} else {
			started_ms = g_ms;
			running = 1;
		}
	}
	if (in->keys & KEYM(KEY_AUX)) {
		if (running) {
			lap_ms = total_ms();
			has_lap = 1;
		} else {
			elapsed_ms = 0;
			has_lap = 0;
		}
	}

	gfx_clear();
	gfx_text(0, 2, "STOPWATCH", 1, 1);
	gfx_text(OLED_WIDTH - gfx_text_width(running ? "RUN" : "STOP", 1), 2,
		 running ? "RUN" : "STOP", 1, 1);

	p = fmt_stamp(line, total_ms());
	*p = 0;
	gfx_text_center(OLED_WIDTH / 2, 20, line, SW_SCALE, 1);

	if (has_lap) {
		p = fmt_str(line, "LAP ");
		p = fmt_stamp(p, lap_ms);
		*p = 0;
		gfx_text_center(OLED_WIDTH / 2, 42, line, 1, 1);
	}
	gfx_text(0, 56, running ? "OK:STOP AUX:LAP" : "OK:START AUX:RESET", 1, 1);
}

const page_t page_stopwatch = {
	.name = "STOPWATCH", .enter = sw_enter, .update = sw_update,
	.keep_awake = 1,
};
