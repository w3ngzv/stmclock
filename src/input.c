#include "input.h"
#include "sys.h"

#define KEY_SAMPLE_MS        5u
#define KEY_DEBOUNCE_SAMPLES 3u
#define KEY_LONG_MS          800u
/*
 * EC11 一格机械定位对应的计数, 常见为 4 (A/B 两路共 4 个边沿), 少数型号为 2.
 * TODO: 编码器模块的型号没有确认, 转一格界面跳两格或半格时改这个值.
 */
#define ENC_COUNTS_PER_DETENT 4

static const struct {
	uint32_t port;
	uint8_t pin;
} key_pin[KEY_COUNT] = {
	[KEY_BACK] = { KEY_PORT, KEY_BACK_PIN },
	[KEY_OK]   = { KEY_PORT, KEY_OK_PIN },
	[KEY_AUX]  = { KEY_PORT, KEY_AUX_PIN },
	[KEY_ENC]  = { ENC_SW_PORT, ENC_SW_PIN },
};

static uint8_t pressed[KEY_COUNT];
static uint8_t mismatch[KEY_COUNT];
static uint8_t long_fired[KEY_COUNT];
static uint32_t press_start[KEY_COUNT];
static volatile uint32_t short_events;
static volatile uint32_t long_events;
static uint16_t enc_last;
static int enc_residual;

void input_init(void)
{
	unsigned k;

	for (k = 0; k < KEY_COUNT; k++)
		gpio_config_pullup(key_pin[k].port, key_pin[k].pin);

	/* 编码器模块自带上拉电阻, 内部上拉只是兜底 */
	gpio_config_pullup(ENC_PORT, ENC_A_PIN);
	gpio_config_pullup(ENC_PORT, ENC_B_PIN);

	/*
	 * 编码器接口模式见 RM0008 15.3.12. SMCR 的 SMS=011 是 TI1 和 TI2 两路
	 * 边沿都计数 (15.4.3). CCMR1 的 CC1S/CC2S=01 把通道接到 TI1/TI2, IC1F/IC2F
	 * 是输入滤波, 用来抑制机械触点抖动 (15.4.7).
	 */
	RCC_APB1ENR |= RCC_APB1_TIM2;
	TIM_SMCR(TIM2_BASE) = 3;
	TIM_CCMR1(TIM2_BASE) = (1u << 0) | (1u << 8) | (0x8u << 4) | (0x8u << 12);
	TIM_ARR(TIM2_BASE) = 0xFFFF;
	TIM_CR1(TIM2_BASE) = 1;
	enc_last = TIM_CNT(TIM2_BASE);
}

void input_tick(void)
{
	static uint8_t divider;
	unsigned k;

	if (++divider < KEY_SAMPLE_MS)
		return;
	divider = 0;

	for (k = 0; k < KEY_COUNT; k++) {
		int down = !gpio_read(key_pin[k].port, key_pin[k].pin);

		if (down == pressed[k]) {
			mismatch[k] = 0;
		} else if (++mismatch[k] >= KEY_DEBOUNCE_SAMPLES) {
			mismatch[k] = 0;
			pressed[k] = (uint8_t)down;
			if (down) {
				press_start[k] = g_ms;
				long_fired[k] = 0;
			} else if (!long_fired[k]) {
				short_events |= 1u << k;
			}
		}

		if (pressed[k] && !long_fired[k] &&
		    g_ms - press_start[k] >= KEY_LONG_MS) {
			long_fired[k] = 1;
			long_events |= 1u << k;
		}
	}
}

uint32_t input_take_short(void)
{
	uint32_t v;

	irq_disable();
	v = short_events;
	short_events = 0;
	irq_enable();
	return v;
}

uint32_t input_take_long(void)
{
	uint32_t v;

	irq_disable();
	v = long_events;
	long_events = 0;
	irq_enable();
	return v;
}

int input_take_encoder(void)
{
	uint16_t now = TIM_CNT(TIM2_BASE);
	int detents;

	enc_residual += (int16_t)(uint16_t)(now - enc_last);
	enc_last = now;
	detents = enc_residual / ENC_COUNTS_PER_DETENT;
	enc_residual -= detents * ENC_COUNTS_PER_DETENT;
	return detents;
}
