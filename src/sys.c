#include "sys.h"
#include "input.h"

uint32_t g_sysclk_hz = 8000000u;
int g_clock_hse;
volatile uint32_t g_ms;

#define SYST_CTRL   REG32(0xE000E010u)
#define SYST_LOAD   REG32(0xE000E014u)
#define SYST_VAL    REG32(0xE000E018u)
#define SYST_CTRL_ENABLE_TICKINT_CORE 7u
#define NVIC_ISER(n) REG32(0xE000E100u + 4u * (n))

#define HSE_START_SPINS 0x5000u
#define PLL_LOCK_SPINS  0x5000u
#define HSE_MHZ         8u
#define PLL_MUL_HSE     9u      /* 8 MHz * 9 = 72 MHz */
#define PLL_MUL_HSI     16u     /* HSI/2 = 4 MHz, * 16 = 64 MHz */

static void clock_init(void)
{
	uint32_t spins;
	int hse_ok = 0;
	int pll_ok = 0;

	RCC_CR |= RCC_CR_HSEON;
	for (spins = HSE_START_SPINS; spins; spins--) {
		if (RCC_CR & RCC_CR_HSERDY) {
			hse_ok = 1;
			break;
		}
	}

	/* 超过 48 MHz 需要 2 个 Flash 等待周期, RM0008 3.3.3 */
	FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

	/*
	 * APB1 二分频到 36 MHz 以内. APB2 不分频. 因为 APB1 分频系数不为 1,
	 * TIM2/TIM3 的输入时钟是 PCLK1 的 2 倍, 与系统时钟相同, 定时器分频
	 * 因此只依赖 g_sysclk_hz.
	 */
	RCC_CFGR = RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_ADCPRE_DIV6;
	if (hse_ok)
		RCC_CFGR |= RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMUL(PLL_MUL_HSE);
	else
		RCC_CFGR |= RCC_CFGR_PLLMUL(PLL_MUL_HSI);

	RCC_CR |= RCC_CR_PLLON;
	for (spins = PLL_LOCK_SPINS; spins; spins--) {
		if (RCC_CR & RCC_CR_PLLRDY) {
			pll_ok = 1;
			break;
		}
	}

	if (pll_ok) {
		RCC_CFGR |= RCC_CFGR_SW_PLL;
		while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL)
			;
		g_sysclk_hz = hse_ok ? HSE_MHZ * PLL_MUL_HSE * 1000000u
				     : 4u * PLL_MUL_HSI * 1000000u;
		g_clock_hse = hse_ok;
	}
	/* PLL 没有锁定时留在复位默认的 8 MHz HSI 上, 功能降速但可用 */
}

void sys_init(void)
{
	clock_init();

	RCC_APB2ENR |= RCC_APB2_AFIO | RCC_APB2_IOPA | RCC_APB2_IOPB |
		       RCC_APB2_IOPC | RCC_APB2_ADC1;
	(void)RCC_APB2ENR; /* 回读, 保证时钟使能先于第一次访问外设 */

	SYST_LOAD = g_sysclk_hz / 1000u - 1u;
	SYST_VAL = 0;
	SYST_CTRL = SYST_CTRL_ENABLE_TICKINT_CORE;
}

void systick_handler(void)
{
	g_ms++;
	input_tick();
}

void delay_ms(uint32_t ms)
{
	uint32_t start = g_ms;

	while (g_ms - start < ms)
		;
}

void nvic_enable(unsigned irq)
{
	NVIC_ISER(irq / 32u) = 1u << (irq % 32u);
}

void gpio_config(uint32_t port, unsigned pin, uint32_t cfg)
{
	uint32_t reg = port + (pin < 8u ? 0x00u : 0x04u);
	unsigned shift = (pin & 7u) * 4u;

	REG32(reg) = (REG32(reg) & ~(0xFu << shift)) | (cfg << shift);
}

void gpio_config_pullup(uint32_t port, unsigned pin)
{
	gpio_config(port, pin, GPIO_IN_PULL);
	gpio_write(port, pin, 1); /* 上下拉模式下 ODR 位决定方向, 1 为上拉 */
}
