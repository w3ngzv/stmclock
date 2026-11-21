/*
 * STM32F103C8T6 as specified in the datasheet: 64 KB Flash, 20 KB SRAM.
 * OpenOCD reports 128 KiB Flash on the board in hand, but boards bought
 * elsewhere may be genuine 64 KB parts, so the image is kept within 64 KB.
 */
MEMORY
{
	FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
	RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

ENTRY(reset_handler)

SECTIONS
{
	.text : {
		KEEP(*(.vectors))
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
	} > FLASH

	_sidata = LOADADDR(.data);

	.data : {
		_sdata = .;
		*(.data*)
		. = ALIGN(4);
		_edata = .;
	} > RAM AT > FLASH

	.bss (NOLOAD) : {
		_sbss = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > RAM

	_estack = ORIGIN(RAM) + LENGTH(RAM);

	/DISCARD/ : { *(.ARM.*) *(.comment) }
}
