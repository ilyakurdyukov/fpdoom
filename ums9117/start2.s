@ -*- tab-width: 8 -*-
	.arch armv7-a
	.syntax unified

.macro CODE32_FN name
	.section .text.\name, "ax", %progbits
	.p2align 2
	.code 32
	.type \name, %function
	.global \name
\name:
.endm

CODE32_FN _start
	.ascii "DHTB"
	.long 1
	.org _start + 0x30, 0
	.long __image_size - 0x200
	.org _start + 0x200, 0

	adr	r4, _start
	ldr	r0, [r4, #0x210]
	add	pc, r4, r0

1:	.long	__image_start
	.long	__image_size
	.long	__bss_size
	.long	entry_main2
