#ifndef OLED_H
#define OLED_H

#include <stdint.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_BUF_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)

#define FONT_W 5
#define FONT_ADVANCE 6   /* 字宽加 1 列间隙 */
#define FONT_H 7

extern int g_oled_present;

void oled_init(void);
void oled_flush(void);
void oled_display_on(int on);
void oled_contrast(uint8_t value);
/* 1: 忽略显存全部点亮 (手电筒), 0: 恢复显存内容 */
void oled_all_on(int on);
uint8_t *oled_buffer(void);

void gfx_clear(void);
void gfx_pixel(int x, int y, int on);
void gfx_fill_rect(int x, int y, int w, int h, int on);
void gfx_hline(int x, int y, int w, int on);
void gfx_circle(int cx, int cy, int r, int on);
void gfx_fill_circle(int cx, int cy, int r, int on);
/* 返回文字结束处的 x */
int gfx_text(int x, int y, const char *s, int scale, int on);
int gfx_text_width(const char *s, int scale);
/* 在以 x_center 为中心的位置绘制 */
void gfx_text_center(int x_center, int y, const char *s, int scale, int on);

const uint8_t *font_glyph(char c);

#endif
