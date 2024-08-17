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
	bl	__image_end
	.long	__image_size
//	ldr	sp, 1f
	blx	entry_main
//1:	.long	0x40009000
