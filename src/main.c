#include "app.h"
#include "i2c.h"
#include "input.h"
#include "mpu.h"
#include "oled.h"
#include "sys.h"
#include "w25q.h"
#include "watch.h"

#define FRAME_MS 50u

int main(void)
{
	uint32_t last_frame = 0;

	sys_init();
	gpio_config(LED_PORT, LED_PIN, GPIO_OUT_PP_2M);
	input_init();
	i2c_init();
	oled_init();
	mpu_init();
	w25q_init();
	watch_init();
	app_init();

	for (;;) {
		mpu_update();
		if (watch_tick()) {
			/* 板载 LED 低电平点亮, 每秒翻转一次表示主循环在运行 */
			gpio_write(LED_PORT, LED_PIN, g_now.sec & 1u);
		}

		if (g_ms - last_frame >= FRAME_MS) {
			input_state_t in;

			last_frame = g_ms;
			in.keys = input_take_short();
			in.longs = input_take_long();
			in.enc = input_take_encoder();
			app_update(&in);
			oled_flush();
		}
	}
}
