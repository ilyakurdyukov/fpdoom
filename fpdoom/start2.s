@ -*- tab-width: 8 -*-
	.arch armv5te
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
	adr	r4, _start
	ldr	r0, [r4, #16]
	add	pc, r4, r0

1:	.long	__image_start
	.long	__image_size
	.long	__bss_size
	.long	entry_main2
