#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;

int main(void);
void reset_handler(void);
void systick_handler(void);

/* A fault before the clock tree is set up has nothing useful to report. */
static void default_handler(void)
{
	for (;;)
		;
}

/* Core exceptions only, RM0008 Table 63 positions 0 to 15. IRQ entries are
 * added together with the driver that enables them. */
__attribute__((section(".vectors"), used))
static void (*const vector_table[16])(void) = {
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
