#include "app.h"
#include "fastmath.h"
#include "fmt.h"
#include "input.h"
#include "mpu.h"
#include "oled.h"

#define LEVEL_CX        64
#define LEVEL_CY        36
#define LEVEL_R_OUT     26
#define LEVEL_R_BUBBLE  4
#define LEVEL_RANGE_DEG 20.0f   /* 气泡到达边缘时对应的倾角 */
#define LEVEL_OK_DEG    1.5f

static int view; /* 0: 数值, 1: 水平仪 */

static int round_tenths(float v)
{
	return (int)(v >= 0.0f ? v * 10.0f + 0.5f : v * 10.0f - 0.5f);
}

static char *fmt_angle(char *p, float deg)
{
	p = fmt_tenths(p, round_tenths(deg));
	*p++ = '~';
	return p;
}

static void draw_values(const mpu_state_t *s)
{
	char line[24];
	char *p;

	p = fmt_str(line, "PITCH ");
	p = fmt_angle(p, s->pitch);
	*p = 0;
	gfx_text(0, 14, line, 1, 1);

	p = fmt_str(line, "ROLL  ");
	p = fmt_angle(p, s->roll);
	*p = 0;
	gfx_text(0, 24, line, 1, 1);

	p = fmt_str(line, "YAW   ");
	p = fmt_angle(p, s->yaw);
	*p = 0;
	gfx_text(0, 34, line, 1, 1);

	p = fmt_str(line, "STEPS ");
	p = fmt_dec(p, (int32_t)s->steps, 1);
	*p = 0;
	gfx_text(0, 46, line, 2, 1);
}

static void draw_level(const mpu_state_t *s)
{
	const int reach = LEVEL_R_OUT - LEVEL_R_BUBBLE - 1;
	float dx = s->roll * (float)reach / LEVEL_RANGE_DEG;
	float dy = s->pitch * (float)reach / LEVEL_RANGE_DEG;
	float len = fm_sqrt(dx * dx + dy * dy);
	int level = fm_abs(s->roll) < LEVEL_OK_DEG && fm_abs(s->pitch) < LEVEL_OK_DEG;
	int bx, by;
	char line[16];
	char *p;

	if (len > (float)reach) {
		dx *= (float)reach / len;
		dy *= (float)reach / len;
	}
	/* TODO: 气泡移动方向与模块朝向有关, 实测后可能要改符号 */
	bx = LEVEL_CX + (int)dx;
	by = LEVEL_CY + (int)dy;

	gfx_circle(LEVEL_CX, LEVEL_CY, LEVEL_R_OUT, 1);
	gfx_hline(LEVEL_CX - 6, LEVEL_CY, 13, 1);
	gfx_fill_rect(LEVEL_CX, LEVEL_CY - 6, 1, 13, 1);
	if (level)
		gfx_fill_circle(bx, by, LEVEL_R_BUBBLE, 1);
	else
		gfx_circle(bx, by, LEVEL_R_BUBBLE, 1);

	p = fmt_str(line, "R");
	p = fmt_tenths(p, round_tenths(s->roll));
	*p = 0;
	gfx_text(0, 2, line, 1, 1);
	p = fmt_str(line, "P");
	p = fmt_tenths(p, round_tenths(s->pitch));
	*p = 0;
	gfx_text(OLED_WIDTH - gfx_text_width(line, 1), 2, line, 1, 1);
	if (level)
		gfx_text_center(LEVEL_CX, 56, "LEVEL", 1, 1);
}

static void motion_enter(void)
{
	view = 0;
}

static void motion_update(const input_state_t *in)
{
	const mpu_state_t *s = mpu_state();

	if (in->keys & KEYM(KEY_AUX))
		view = !view;
	if (in->keys & (KEYM(KEY_OK) | KEYM(KEY_ENC))) {
		mpu_calibrate_gyro(); /* 阻塞约 0.4 秒, 期间要保持静止 */
		mpu_zero();
	}

	gfx_clear();
	if (!mpu_present()) {
		gfx_text(0, 2, "MOTION", 1, 1);
		gfx_text(0, 20, "MPU NOT FOUND", 1, 1);
		gfx_text(0, 32, "CHECK PB6 PB7 3V3", 1, 1);
		return;
	}

	if (view) {
		draw_level(s);
	} else {
		gfx_text(0, 2, "MOTION", 1, 1);
		draw_values(s);
	}
	if (!view)
		gfx_text(OLED_WIDTH - gfx_text_width("OK:ZERO", 1), 2, "OK:ZERO", 1, 1);
}

const page_t page_motion = {
	.name = "MOTION", .enter = motion_enter, .update = motion_update,
	.keep_awake = 1,
};
