#ifndef SYS_H
#define SYS_H

#include <stdint.h>
#include "board.h"

extern uint32_t g_sysclk_hz;
extern int g_clock_hse;         /* 1: HSE 经 PLL 到 72 MHz, 0: 回退到 HSI */
extern volatile uint32_t g_ms;  /* SysTick 累加的毫秒数, 约 49 天回绕 */

void sys_init(void);
void delay_ms(uint32_t ms);
void nvic_enable(unsigned irq);

void gpio_config(uint32_t port, unsigned pin, uint32_t cfg);
void gpio_config_pullup(uint32_t port, unsigned pin);

static inline void gpio_write(uint32_t port, unsigned pin, int level)
{
	GPIO_BSRR(port) = level ? (1u << pin) : (1u << (pin + 16u));
}

static inline int gpio_read(uint32_t port, unsigned pin)
{
	return (GPIO_IDR(port) >> pin) & 1u;
}

static inline void irq_disable(void) { __asm__ volatile("cpsid i" ::: "memory"); }
static inline void irq_enable(void)  { __asm__ volatile("cpsie i" ::: "memory"); }

#endif
