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

//  0 : MMU enable
//  2 : Level 1 Data Cache enable
// 12 : Instruction Cache enable

CODE32_FN _start
	b	3f
	.long	0xffffffff
	.ascii	"SDLOADER"
	.long	__image_size
	.global sdbootkey
sdbootkey: // sdboot + 0x14
	.long	0xffffffff
3:	mrc	p15, #0, r0, c1, c0, #0 // Read Control Register
	bic	r0, #5
.if 0
	bic	r0, #0x1000
.else // faster
	orr	r0, #0x1000
.endif
	mcr	p15, #0, r0, c1, c0, #0 // Write Control Register
	msr	cpsr_c, #0xd3
	ldr	sp, 1f
	adr	r0, _start
.if 1
	blx	entry_main
.else
	ldr	pc, 2f
2:	.long	entry_main
.endif
1:	.long	0x40009000 // __stack_bottom

