#include "actuators.h"
#include "sys.h"

/*
 * 舵机周期约 20 ms, 脉宽 1.5 ms 为中位, 可用范围 500 到 2400 us (SG90 数据手册
 * Specifications 和 Additional Specifications). 定时器分频到 1 MHz, 一个计数
 * 就是 1 us, CCR3 直接写脉宽.
 */
#define SERVO_PERIOD_US   20000u
#define SERVO_CENTER_US   1500u
#define SERVO_SWING_US    200u    /* 约 +-19 度, TODO: 振幅是估计值, 要在实物上调 */
#define SERVO_FLIP_MS     80u     /* SG90 约 0.1 s 转 60 度, 半周期太短摆不到位 */
#define SERVO_OFF         0u      /* 脉宽 0 即不输出脉冲, 舵机放松, 不抖也不耗电 */

#define BEEP_CYCLE_MS     1200u
#define BEEP_ON_MS        150u
#define BEEP_SECOND_MS    300u    /* 第二声开始的时刻, 两声一组 */
#define TEST_BEEP_MS      400u
#define TEST_VIBRATE_MS   1000u

static int ring_on;
static int ring_vibrate;
static uint32_t ring_start_ms;
static uint32_t beep_until_ms;
static uint32_t vibrate_until_ms;

/* 有源蜂鸣器模块低电平触发 (器件清单第 13 项), 所以导通时写 0 */
static void buzzer(int on)
{
	gpio_write(BUZZER_PORT, BUZZER_PIN, !on);
}

static void servo_pulse_us(uint32_t us)
{
	TIM_CCR3(TIM3_BASE) = (uint16_t)us;
}

void actuators_init(void)
{
	/* ODR 复位值为 0, 先写高再切成输出, 否则低电平触发的模块会在上电时响一下 */
	buzzer(0);
	gpio_config(BUZZER_PORT, BUZZER_PIN, GPIO_OUT_PP_2M);

	gpio_config(SERVO_PORT, SERVO_PIN, GPIO_AF_PP_2M);
	RCC_APB1ENR |= RCC_APB1_TIM3;

	/*
	 * PWM 模式 1: OC3M=110, 并开启预装载 OC3PE, 使脉宽在周期边界生效,
	 * 避免中途改 CCR 产生毛刺 (RM0008 15.3.9, 15.4.8). 使能输出 CC3E 见 15.4.9.
	 * 定时器输入时钟等于系统时钟的原因见 sys.c.
	 */
	TIM_PSC(TIM3_BASE) = (uint16_t)(g_sysclk_hz / 1000000u - 1u);
	TIM_ARR(TIM3_BASE) = (uint16_t)(SERVO_PERIOD_US - 1u);
	TIM_CCMR2(TIM3_BASE) = (6u << 4) | (1u << 3);
	TIM_CCER(TIM3_BASE) = 1u << 8;
	servo_pulse_us(SERVO_OFF);
	TIM_EGR(TIM3_BASE) = 1;                 /* UG: 立即装载 PSC 和 ARR */
	TIM_CR1(TIM3_BASE) = (1u << 7) | 1u;    /* ARPE | CEN */
}

void actuators_ring(int on, int vibrate)
{
	if (on && !ring_on)
		ring_start_ms = g_ms;
	ring_on = on;
	ring_vibrate = vibrate;
}

void actuators_test_beep(void)
{
	beep_until_ms = g_ms + TEST_BEEP_MS;
}

void actuators_test_vibrate(void)
{
	vibrate_until_ms = g_ms + TEST_VIBRATE_MS;
}

void actuators_update(void)
{
	uint32_t now = g_ms;
	int beep = 0;
	int vibrate = 0;

	if (ring_on) {
		uint32_t t = (now - ring_start_ms) % BEEP_CYCLE_MS;

		beep = t < BEEP_ON_MS || (t >= BEEP_SECOND_MS && t < BEEP_SECOND_MS + BEEP_ON_MS);
		vibrate = ring_vibrate;
	}
	/* 有符号差值比较, g_ms 回绕时仍然正确 */
	if ((int32_t)(beep_until_ms - now) > 0)
		beep = 1;
	if ((int32_t)(vibrate_until_ms - now) > 0)
		vibrate = 1;

	buzzer(beep);
	if (vibrate)
		servo_pulse_us(((now / SERVO_FLIP_MS) & 1u) ? SERVO_CENTER_US + SERVO_SWING_US
							   : SERVO_CENTER_US - SERVO_SWING_US);
	else
		servo_pulse_us(SERVO_OFF);
}
