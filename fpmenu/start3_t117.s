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

	bl	__image_end + 0x200
	.long	__image_size

	blx	entry_main

