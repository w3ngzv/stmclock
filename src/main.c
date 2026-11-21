#include <stdint.h>

/* Addresses and bit positions from RM0008 (STM32F101xx/102xx/103xx). */
#define RCC_APB2ENR   (*(volatile uint32_t *)0x40021018u)
#define RCC_IOPCEN    (1u << 4)
#define GPIOC_CRH     (*(volatile uint32_t *)0x40011004u)
#define GPIOC_BSRR    (*(volatile uint32_t *)0x40011010u)

#define PIN13_SHIFT   13u
#define CRH_PIN13_POS 20u
#define CRH_PIN_MASK  0xFu
/* MODE=10 (output, 2 MHz), CNF=00 (push-pull) */
#define CRH_OUT_PP_2M 0x2u

/* The Blue Pill LED sits between 3.3 V and PC13, so it lights when PC13 is low. */

/* Rough delay on the reset-default 8 MHz HSI. Replaced by SysTick later. */
#define BLINK_SPINS   400000u

static void spin(uint32_t count)
{
	while (count--)
		__asm__ volatile("");
}

int main(void)
{
	RCC_APB2ENR |= RCC_IOPCEN;
	/* The read-back orders the clock enable before the first GPIOC access. */
	(void)RCC_APB2ENR;

	GPIOC_CRH = (GPIOC_CRH & ~(CRH_PIN_MASK << CRH_PIN13_POS)) |
		    (CRH_OUT_PP_2M << CRH_PIN13_POS);

	for (;;) {
		GPIOC_BSRR = 1u << (PIN13_SHIFT + 16u); /* low, LED on */
		spin(BLINK_SPINS);
		GPIOC_BSRR = 1u << PIN13_SHIFT;         /* high, LED off */
		spin(BLINK_SPINS);
	}
}
