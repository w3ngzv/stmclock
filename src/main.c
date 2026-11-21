#include "actuators.h"
#include "alarm.h"
#include "app.h"
#include "bt.h"
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
	actuators_init();
	bt_init();
	app_init();

	for (;;) {
		static int was_ringing;
		int ringing;

		mpu_update();
		bt_poll();
		alarm_poll(&g_now, g_ms);
		ringing = alarm_ringing();
		if (ringing != was_ringing) {
			actuators_ring(ringing, g_alarm.vibrate);
			was_ringing = ringing;
		}
		actuators_update();
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
			/*
			 * TODO(练习: 自动熄屏和亮屏, 考核第 11 项的 "光感启动 OLED"):
			 * 记下最后一次有输入 (in 的三个字段任一不为 0) 的 g_ms. 超过
			 * 一段时间 (比如 15 s) 没有输入, 并且 app_keep_awake() 为 0 时,
			 * 调 oled_display_on(0) 熄屏. 熄屏期间以下任一情况亮屏:
			 * 按键或编码器, mpu_take_shake() 返回 1 (晃动), 光敏读数从暗变亮
			 * (见 page_sensor.c), 红外检测到手靠近. 亮屏用 oled_display_on(1).
			 * 两点要注意: 唤醒屏幕的那次按键不要再传给 app_update(), 否则一按
			 * 就既亮屏又进了菜单; app_keep_awake() 在闹钟响时为 1, 闹钟必须
			 * 能把屏幕点亮.
			 */
			app_update(&in);
			oled_flush();
		}
	}
}
