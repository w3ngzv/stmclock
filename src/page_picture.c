#include "app.h"
#include "input.h"
#include "oled.h"
#include "picture.h"
#include "sys.h"
#include "w25q.h"

#define STATUS_SHOW_MS 2000u

static const char *status_text;
static uint32_t status_until;

static void picture_enter(void)
{
	picture_load();
	status_text = 0;
}

static void picture_update(const input_state_t *in)
{
	unsigned i;

	if (in->longs & KEYM(KEY_OK)) {
		int rc;

		picture_make_demo();
		rc = picture_save();
		status_text = rc == 0 ? "SAVED" : rc == -1 ? "WRITE FAIL" : "VERIFY FAIL";
		status_until = g_ms + STATUS_SHOW_MS;
	}

	gfx_clear();
	if (!w25q_present()) {
		gfx_text(0, 2, "PICTURE", 1, 1);
		gfx_text(0, 20, "NO W25Q FOUND", 1, 1);
		gfx_text(0, 32, "CHECK PB12-PB15", 1, 1);
	} else if (!picture_valid()) {
		gfx_text(0, 2, "PICTURE", 1, 1);
		gfx_text(0, 20, "FLASH HAS NO IMAGE", 1, 1);
		gfx_text(0, 32, "HOLD OK: SAVE DEMO", 1, 1);
		gfx_text(0, 44, "OR SEND IMGW BY BT", 1, 1);
	} else {
		for (i = 0; i < OLED_BUF_SIZE; i++)
			oled_buffer()[i] = picture_buf()[i];
	}

	if (status_text && (int32_t)(status_until - g_ms) > 0) {
		int w = gfx_text_width(status_text, 1);

		gfx_fill_rect(OLED_WIDTH / 2 - w / 2 - 3, 26, w + 6, 13, 0);
		gfx_fill_rect(OLED_WIDTH / 2 - w / 2 - 3, 26, w + 6, 1, 1);
		gfx_fill_rect(OLED_WIDTH / 2 - w / 2 - 3, 38, w + 6, 1, 1);
		gfx_text_center(OLED_WIDTH / 2, 29, status_text, 1, 1);
	}
}

const page_t page_picture = {
	.name = "PICTURE", .enter = picture_enter, .update = picture_update,
	.keep_awake = 1,
};
