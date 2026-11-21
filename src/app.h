#ifndef APP_H
#define APP_H

#include <stdint.h>

typedef struct {
	int enc;         /* 本帧编码器转过的格数, 顺时针为正 */
	uint32_t keys;   /* 本帧的短按事件, 位由 KEYM(KEY_xxx) 给出 */
	uint32_t longs;  /* 本帧的长按事件 */
} input_state_t;

typedef struct {
	const char *name;
	void (*enter)(void);
	void (*leave)(void);
	void (*update)(const input_state_t *in);
	uint8_t keep_awake; /* 1: 该页面显示期间不自动熄屏 */
} page_t;

extern const page_t page_clock;
extern const page_t page_settime;
extern const page_t page_stopwatch;
extern const page_t page_torch;
extern const page_t page_motion;
extern const page_t page_picture;
extern const page_t page_alarm;

void app_init(void);
void app_goto(const page_t *page);
void app_menu(void);
/* 当前页面是否要求保持亮屏 */
int app_keep_awake(void);
void app_update(const input_state_t *in);

#endif
