/* ld-script : SC6531E FDL */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)

EXTRA_START = 0x40000000; EXTRA_SIZE = 0x7000;
/* IMAGE_START = 0x14000000; */ IMAGE_SIZE = 0x100000;

ENTRY(_start)
SECTIONS {
	.text IMAGE_START : {
		__image_start = .;
		*(.text._start) *(.text .text.*)
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		*(.data .data.* .gnu.linkonce.d.*)
		. = ALIGN(4);
		__image_end = .;
	}
	.bss : {
		__bss_start = .;
		*(.bss .bss.* .gnu.linkonce.b.*) *(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	}

	__image_size = __image_end - __image_start;
	__bss_size = __bss_end - __image_end;

	__stack_top = EXTRA_START;
	__stack_bottom = __stack_top + EXTRA_SIZE;

	ASSERT(__bss_end - IMAGE_START <= IMAGE_SIZE, "image overflow")

	/* Clang's linker tries to insert this section between .text and .bss if I don't list it here or discard it. */
  .dynamic : { *(.dynamic) }
}
