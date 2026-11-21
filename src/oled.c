#include "oled.h"
#include "i2c.h"

/*
 * SSD1306, 128x64, I2C. 从机地址为 0111100 加 SA0 位, 即 0x3C 或 0x3D
 * (SSD1306 8.1.5). 控制字节 0x00 之后的字节按命令解释, 0x40 之后按显存
 * 数据解释 (8.1.5.2). 显存按页组织: 每页 8 行, 字节内低位在上 (8.7 GDDRAM).
 */
#define OLED_ADDR_PRIMARY   0x3Cu
#define OLED_ADDR_SECONDARY 0x3Du
#define CTRL_CMD            0x00u
#define CTRL_DATA           0x40u

int g_oled_present;
static uint8_t oled_addr = OLED_ADDR_PRIMARY;
static uint8_t framebuf[OLED_BUF_SIZE];

/*
 * 各命令的含义见 SSD1306 第 10 节. 电荷泵 8Dh, 14h 以及上电次序来自同一个
 * PDF 末尾 App Note 的 Charge Pump Command Table 和 Figure 2
 * (Software Initialization Flow Chart): 电荷泵必须在 AFh 开显示之前使能.
 */
static const uint8_t init_seq[] = {
	0xAE,             /* 关显示 */
	0xD5, 0x80,       /* 时钟分频 */
	0xA8, 0x3F,       /* 64 行复用 */
	0xD3, 0x00,       /* 显示偏移 0 */
	0x40,             /* 起始行 0 */
	0x8D, 0x14,       /* 开启内部电荷泵 */
	0x20, 0x00,       /* 水平寻址模式 */
	0xA1,             /* 列重映射 */
	0xC8,             /* 行扫描方向反向 */
	0xDA, 0x12,       /* COM 引脚配置 */
	0x81, 0xCF,       /* 对比度 */
	0xD9, 0xF1,       /* 预充电周期 */
	0xDB, 0x40,       /* VCOMH 电平 */
	0xA4,             /* 显示跟随显存 */
	0xA6,             /* 正常显示, 不反相 */
	0xAF,             /* 开显示 */
};

static int send_cmds(const uint8_t *cmds, unsigned len)
{
	unsigned i;
	int ok;

	i2c_start();
	ok = i2c_put((uint8_t)(oled_addr << 1)) && i2c_put(CTRL_CMD);
	for (i = 0; ok && i < len; i++)
		ok = i2c_put(cmds[i]);
	i2c_stop();
	return ok;
}

void oled_init(void)
{
	/*
	 * 模块出厂地址通常是 0x3C, 个别焊接电阻后为 0x3D. 先试 0x3C 再试 0x3D.
	 * TODO: 屏幕方向没有在实物上确认. 上下颠倒时把 init_seq 中的 0xA1/0xC8
	 * 改成 0xA0/0xC0.
	 */
	oled_addr = OLED_ADDR_PRIMARY;
	g_oled_present = send_cmds(init_seq, sizeof(init_seq));
	if (!g_oled_present) {
		oled_addr = OLED_ADDR_SECONDARY;
		g_oled_present = send_cmds(init_seq, sizeof(init_seq));
	}
	gfx_clear();
	oled_flush();
}

void oled_flush(void)
{
	static const uint8_t window[] = { 0x21, 0, OLED_WIDTH - 1, 0x22, 0, OLED_HEIGHT / 8 - 1 };
	unsigned i;
	int ok;

	/* 屏幕没有应答时在这里返回, 不浪费约 30 ms 去移位 1 KB 数据 */
	if (!send_cmds(window, sizeof(window))) {
		g_oled_present = 0;
		return;
	}
	g_oled_present = 1;

	i2c_start();
	ok = i2c_put((uint8_t)(oled_addr << 1)) && i2c_put(CTRL_DATA);
	for (i = 0; ok && i < OLED_BUF_SIZE; i++)
		ok = i2c_put(framebuf[i]);
	i2c_stop();
}

void oled_display_on(int on)
{
	uint8_t cmd = on ? 0xAF : 0xAE;

	send_cmds(&cmd, 1);
}

void oled_contrast(uint8_t value)
{
	uint8_t cmds[2] = { 0x81, value };

	send_cmds(cmds, sizeof(cmds));
}

void oled_all_on(int on)
{
	uint8_t cmd = on ? 0xA5 : 0xA4;

	send_cmds(&cmd, 1);
}

uint8_t *oled_buffer(void)
{
	return framebuf;
}

void gfx_clear(void)
{
	unsigned i;

	for (i = 0; i < OLED_BUF_SIZE; i++)
		framebuf[i] = 0;
}

void gfx_pixel(int x, int y, int on)
{
	uint8_t mask;
	unsigned idx;

	if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
		return;
	idx = (unsigned)(y >> 3) * OLED_WIDTH + (unsigned)x;
	mask = (uint8_t)(1u << (y & 7));
	if (on)
		framebuf[idx] |= mask;
	else
		framebuf[idx] &= (uint8_t)~mask;
}

void gfx_fill_rect(int x, int y, int w, int h, int on)
{
	int i, j;

	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
			gfx_pixel(x + i, y + j, on);
}

void gfx_hline(int x, int y, int w, int on)
{
	gfx_fill_rect(x, y, w, 1, on);
}

/* 中点画圆法 */
void gfx_circle(int cx, int cy, int r, int on)
{
	int x = r, y = 0, err = 1 - r;

	while (x >= y) {
		gfx_pixel(cx + x, cy + y, on);
		gfx_pixel(cx - x, cy + y, on);
		gfx_pixel(cx + x, cy - y, on);
		gfx_pixel(cx - x, cy - y, on);
		gfx_pixel(cx + y, cy + x, on);
		gfx_pixel(cx - y, cy + x, on);
		gfx_pixel(cx + y, cy - x, on);
		gfx_pixel(cx - y, cy - x, on);
		y++;
		if (err < 0) {
			err += 2 * y + 1;
		} else {
			x--;
			err += 2 * (y - x) + 1;
		}
	}
}

void gfx_fill_circle(int cx, int cy, int r, int on)
{
	int x = r, y = 0, err = 1 - r;

	while (x >= y) {
		gfx_hline(cx - x, cy + y, 2 * x + 1, on);
		gfx_hline(cx - x, cy - y, 2 * x + 1, on);
		gfx_hline(cx - y, cy + x, 2 * y + 1, on);
		gfx_hline(cx - y, cy - x, 2 * y + 1, on);
		y++;
		if (err < 0) {
			err += 2 * y + 1;
		} else {
			x--;
			err += 2 * (y - x) + 1;
		}
	}
}

static void draw_char(int x, int y, char c, int scale, int on)
{
	const uint8_t *glyph = font_glyph(c);
	int col, row;

	for (col = 0; col < FONT_W; col++) {
		for (row = 0; row < FONT_H; row++) {
			if ((glyph[col] >> row) & 1u)
				gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, on);
		}
	}
}

int gfx_text(int x, int y, const char *s, int scale, int on)
{
	for (; *s; s++) {
		draw_char(x, y, *s, scale, on);
		x += FONT_ADVANCE * scale;
	}
	return x;
}

int gfx_text_width(const char *s, int scale)
{
	int n = 0;

	while (s[n])
		n++;
	return n ? n * FONT_ADVANCE * scale - scale : 0;
}

void gfx_text_center(int x_center, int y, const char *s, int scale, int on)
{
	gfx_text(x_center - gfx_text_width(s, scale) / 2, y, s, scale, on);
}
