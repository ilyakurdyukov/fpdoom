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

//  0 : MMU enable
//  2 : Level 1 Data Cache enable
// 12 : Instruction Cache enable

	mov	r1, #0
	mcr	p15, #0, r1, c7, c5, #0	// Invalidate ICache
	mrc	p15, #0, r0, c1, c0, #0 // Read Control Register
	bic	r0, #5
	orr	r0, #0x1000
	mcr	p15, #0, r0, c1, c0, #0 // Write Control Register
	msr	cpsr_c, #0xd3
	ldr	sp, 1f

	ldr	r2, 3f
	ldr	r1, 4f
	adr	r0, _start
	subs	r2, r0, r2
	push	{r0-r1}
	add	r1, r0
	blne	apply_reloc
	pop	{r0-r1}

	ldr	r2, 5f
	ldr	pc, 2f
2:	.long	entry_main
1:	.long	__stack_bottom
3:	.long	__image_start
4:	.long	__image_size
5:	.long	__bss_size

	.type apply_reloc, %function
	.global apply_reloc
apply_reloc:
	// r0 = image, r1 = reloc, r2 = diff
	push	{r4-r7,lr}
	mov	r5, #1
	ldrb	r6, [r1], #1	// nbits
	ldrb	r7, [r1], #1	// maxrun
1:	mov	ip, #0
	mov	lr, r7
2:	ldrb	r4, [r1], #1
	sub	lr, lr, r4, lsl ip
	lsrs	r4, r6
	add	ip, r6
	beq	2b
	adds	lr, lr, r5, lsl ip
	popeq	{r4-r7,pc}
	asrs	ip, lr, #1
	mvncc	r3, #0
	movmi	r3, lr
3:	ldr	r4, [r0, -r3, lsl #2]!
	add	r4, r2
	str	r4, [r0]
	subs	ip, #1
	bpl	3b
	b	1b

