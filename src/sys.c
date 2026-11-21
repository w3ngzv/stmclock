#include "sys.h"
#include "input.h"

uint32_t g_sysclk_hz = 8000000u;
int g_clock_hse;
volatile uint32_t g_ms;

#define SYST_CTRL   REG32(0xE000E010u)
#define SYST_LOAD   REG32(0xE000E014u)
#define SYST_VAL    REG32(0xE000E018u)
/* ENABLE | TICKINT | CLKSOURCE=处理器时钟, PM0056 4.5.1 */
#define SYST_CTRL_RUN_CORE_CLK 7u
#define NVIC_ISER(n) REG32(0xE000E100u + 4u * (n))   /* PM0056 4.3.2, 基地址见 4.3.11 */

#define HSE_START_SPINS 0x5000u
#define PLL_LOCK_SPINS  0x5000u
#define HSE_MHZ         8u      /* 核心板晶振, 按常见 Blue Pill 设计假定, 未测量 */
#define PLL_MUL_HSE     9u      /* 8 MHz * 9 = 72 MHz, PLL 输出上限 72 MHz, RM0008 7.3.2 */
#define PLL_MUL_HSI     16u     /* HSI 经 /2 为 4 MHz, 回退时 4 * 16 = 64 MHz */

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

	/* 48 MHz 以上要 2 个等待周期: RM0008 3.3.3, FLASH_ACR.LATENCY */
	FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

	/*
	 * PCLK1 上限 36 MHz, 所以 APB1 二分频 (RM0008 7.3.2, PPRE1).
	 * APB 预分频不为 1 时, 定时器时钟为 PCLK 的 2 倍 (RM0008 7.2 的时钟树脚注),
	 * 因此 TIM2/TIM3 的输入时钟等于系统时钟, 定时器分频只依赖 g_sysclk_hz.
	 * ADC 时钟上限 14 MHz (DS5319 5.3.18), 72 MHz 经 ADCPRE 六分频为 12 MHz.
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
	/* PLL 没有锁定时留在复位默认的 8 MHz HSI 上, 速度降低但功能可用 */
}

void sys_init(void)
{
	clock_init();

	RCC_APB2ENR |= RCC_APB2_AFIO | RCC_APB2_IOPA | RCC_APB2_IOPB |
		       RCC_APB2_IOPC | RCC_APB2_ADC1;

	/* STK_LOAD 只有 24 位 (PM0056 4.5.2), 72000 - 1 在范围内 */
	SYST_LOAD = g_sysclk_hz / 1000u - 1u;
	SYST_VAL = 0;
	SYST_CTRL = SYST_CTRL_RUN_CORE_CLK;
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

/* CRL 管 0 到 7 脚, CRH 管 8 到 15 脚, 每脚 4 位: RM0008 9.2.1, 9.2.2 */
void gpio_config(uint32_t port, unsigned pin, uint32_t cfg)
{
	uint32_t reg = port + (pin < 8u ? 0x00u : 0x04u);
	unsigned shift = (pin & 7u) * 4u;

	REG32(reg) = (REG32(reg) & ~(0xFu << shift)) | (cfg << shift);
}

/* 上拉/下拉输入由 ODR 位选方向, ODR=1 为上拉: RM0008 9.1.7 和 Table 20 */
void gpio_config_pullup(uint32_t port, unsigned pin)
{
	gpio_config(port, pin, GPIO_IN_PULL);
	gpio_write(port, pin, 1);
}
