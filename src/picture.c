#include "picture.h"
#include "oled.h"
#include "w25q.h"

#define PIC_ADDR      0u
#define PIC_HDR_SIZE  4u
#define PIC_MAGIC_0   'I'
#define PIC_MAGIC_1   'M'
#define PIC_MAGIC_2   'G'
#define PIC_MAGIC_3   '1'

/* 与 SSD1306 的 GDDRAM 同样排列 (SSD1306 8.7), 显示时可以整块拷贝到显存 */
static uint8_t pic[OLED_BUF_SIZE];
static int valid;

uint8_t *picture_buf(void) { return pic; }
int picture_valid(void) { return valid; }

int picture_load(void)
{
	uint8_t hdr[PIC_HDR_SIZE];

	valid = 0;
	if (w25q_read(PIC_ADDR, hdr, sizeof(hdr)) != 0)
		return -1;
	if (hdr[0] != PIC_MAGIC_0 || hdr[1] != PIC_MAGIC_1 ||
	    hdr[2] != PIC_MAGIC_2 || hdr[3] != PIC_MAGIC_3)
		return -1;
	if (w25q_read(PIC_ADDR + PIC_HDR_SIZE, pic, sizeof(pic)) != 0)
		return -1;
	valid = 1;
	return 0;
}

int picture_save(void)
{
	static const uint8_t hdr[PIC_HDR_SIZE] = { PIC_MAGIC_0, PIC_MAGIC_1, PIC_MAGIC_2, PIC_MAGIC_3 };
	uint8_t check[OLED_BUF_SIZE];
	unsigned i;

	/* 文件头加图片共 1028 字节, 在一个 4 KB 扇区内 */
	if (w25q_erase_sector(PIC_ADDR) != 0)
		return -1;
	if (w25q_write(PIC_ADDR, hdr, sizeof(hdr)) != 0 ||
	    w25q_write(PIC_ADDR + PIC_HDR_SIZE, pic, sizeof(pic)) != 0)
		return -1;

	if (w25q_read(PIC_ADDR + PIC_HDR_SIZE, check, sizeof(check)) != 0)
		return -2;
	for (i = 0; i < sizeof(check); i++) {
		if (check[i] != pic[i])
			return -2;
	}
	valid = 1;
	return 0;
}

void picture_make_demo(void)
{
	unsigned i;
	int x;

	gfx_clear();
	gfx_fill_rect(0, 0, OLED_WIDTH, 1, 1);
	gfx_fill_rect(0, OLED_HEIGHT - 1, OLED_WIDTH, 1, 1);
	gfx_fill_rect(0, 0, 1, OLED_HEIGHT, 1);
	gfx_fill_rect(OLED_WIDTH - 1, 0, 1, OLED_HEIGHT, 1);
	gfx_circle(32, 32, 24, 1);
	gfx_fill_circle(32, 32, 10, 1);
	for (x = 0; x < 48; x++) {
		gfx_pixel(72 + x, 8 + x / 2, 1);
		gfx_pixel(72 + x, 56 - x / 2, 1);
	}
	gfx_text(70, 28, "W25Q", 2, 1);

	for (i = 0; i < OLED_BUF_SIZE; i++)
		pic[i] = oled_buffer()[i];
}
