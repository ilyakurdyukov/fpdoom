@ -*- tab-width: 8 -*-
	.arch armv7-a
	.syntax unified

.macro CODE16_FN name
	.section .text.\name, "ax", %progbits
	.p2align 1
	.code 16
	.type \name, %function
	.global \name
	.thumb_func
\name:
.endm

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
	.ascii "DHTB"
	.long 1
	.org _start + 0x30, 0
	.long __image_size - 0x200
.if 0
	.org _start + 0x1b8, 0
	.word 0 // signature
	.hword 0
	.word 0, 0xc, 0x20, 0x77ffe0
.endif
	.org _start + 0x1fe, 0
	.hword 0xaa55

	adr	r2, _start
	ldr	r0, 3f
4:	cmp	r2, r0
	bne	4b

	mrc	p15, #0, r0, c1, c0, #0 // Read Control Register
	bic	r0, #5
.if 0
	bic	r0, #0x1000
.else // faster
	orr	r0, #0x1000
.endif
	mcr	p15, #0, r0, c1, c0, #0 // Write Control Register

	mov	r0, #0xf00000
	mcr	p15, #0, r0, c1, c0, #2
	mov	r0, #0x40000000
	vmsr	FPEXC, r0
	isb	sy
	mov	r2, #0xd3
	msr	CPSR_c, r2
	mov	r0, #0x12000
	mov	sp, r0

	// 24 - FZ (flush to zero)
	// 25 - DN (default NaN)
	mov	r0, #0x3000000
	vmsr	FPSCR, r0

	ldr	pc, 2f
2:	.long	entry_main
3:	.long	__image_start

