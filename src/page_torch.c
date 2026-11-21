#include "app.h"
#include "input.h"
#include "oled.h"

#define CONTRAST_MAX 0xFFu
#define CONTRAST_DEFAULT 0xCFu

static void torch_enter(void)
{
	oled_contrast(CONTRAST_MAX);
	oled_all_on(1); /* 0xA5 让整屏点亮, 与显存内容无关 */
}

static void torch_leave(void)
{
	oled_all_on(0);
	oled_contrast(CONTRAST_DEFAULT);
}

static void torch_update(const input_state_t *in)
{
	/* 任意按键或转动编码器都退出, BACK 键已经在 app_update 中处理 */
	if (in->keys || in->enc)
		app_menu();
}

const page_t page_torch = {
	.name = "TORCH", .enter = torch_enter, .leave = torch_leave,
	.update = torch_update, .keep_awake = 1,
};
