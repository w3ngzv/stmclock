#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

enum {
	KEY_BACK,   /* 返回上一级 */
	KEY_OK,     /* 确认 */
	KEY_AUX,    /* 页面自定义功能 */
	KEY_ENC,    /* 编码器按下, 作用与 OK 相同, 长按回到时钟 */
	KEY_COUNT
};

#define KEYM(k) (1u << (k))

void input_init(void);
/* 由 SysTick 每毫秒调用, 负责按键消抖 */
void input_tick(void);
/* 取走并清除自上次调用以来的短按和长按事件, 每位对应一个按键 */
uint32_t input_take_short(void);
uint32_t input_take_long(void);
/* 自上次调用以来编码器转过的格数, 顺时针为正 */
int input_take_encoder(void);

#endif
