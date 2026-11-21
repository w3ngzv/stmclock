#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;

int main(void);
void reset_handler(void);
void systick_handler(void);
void usart2_irq_handler(void);

/* 时钟树建立之前的异常没有可报告的内容, 原地停住便于调试器查看 */
static void default_handler(void)
{
	for (;;)
		;
}

/*
 * 前 16 项是栈顶和内核异常, 顺序见 PM0056 2.3.2 Table 16. 外设中断从第 16
 * 项起, 序号见 RM0008 Table 63. 外设中断项随各自的驱动加入.
 */
__attribute__((section(".vectors"), used))
static void (*const vector_table[16 + 39])(void) = {
	(void (*)(void))&_estack,
	reset_handler,
	default_handler, /* NMI */
	default_handler, /* HardFault */
	default_handler, /* MemManage */
	default_handler, /* BusFault */
	default_handler, /* UsageFault */
	0, 0, 0, 0,
	default_handler, /* SVCall */
	default_handler, /* DebugMon */
	0,
	default_handler, /* PendSV */
	systick_handler,
	[16 + 38] = usart2_irq_handler, /* RM0008 Table 63: 位置 38 */
};

void reset_handler(void)
{
	uint32_t *src = &_sidata;
	uint32_t *dst;

	for (dst = &_sdata; dst < &_edata;)
		*dst++ = *src++;
	for (dst = &_sbss; dst < &_ebss;)
		*dst++ = 0;

	main();
	for (;;)
		;
}
