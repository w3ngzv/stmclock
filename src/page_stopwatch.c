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

/*
 * TODO(练习: 秒表): 显示部分已经写好, 计时逻辑要你来补.
 *
 * 时间来源用 g_ms (sys.c), SysTick 中断每毫秒把它加 1, 秒表不需要另开定时器.
 * 需要记三个量:
 *   running     是否正在计时
 *   started_ms  本次按下开始时的 g_ms
 *   elapsed_ms  之前几段计时的累计 (停表再开始时不能清零)
 * 当前读数就是 elapsed_ms + (running ? g_ms - started_ms : 0), 写进
 * total_ms(). g_ms 和 started_ms 都是 uint32_t, 无符号相减在 g_ms 回绕
 * (约 49 天) 后结果仍然正确, 不要改成有符号类型.
 *
 * 按键在 sw_update() 里处理, 底部提示行已经按这个约定写好:
 *   OK 或编码器按键   计时中: 把这一段加进 elapsed_ms 并停下
 *                     停止时: 记下 started_ms 并开始
 *   AUX               计时中: lap_ms 记下当前读数, has_lap 置 1
 *                     停止时: 清零 elapsed_ms 和 has_lap
 *
 * 验收: 计时 10 s, 停 5 s, 再计时 10 s, 读数约为 00:20.00. 离开本页再回来,
 * 正在走的秒表应继续走.
 */
static int running;
static uint32_t lap_ms;
static int has_lap;

static uint32_t total_ms(void)
{
	return 0;
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

static void sw_update(const input_state_t *in)
{
	char line[16];
	char *p;

	(void)in;

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
	.name = "STOPWATCH", .update = sw_update,
	.keep_awake = 1,
};
