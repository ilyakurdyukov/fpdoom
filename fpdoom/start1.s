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
//  1 : Alignment fault enable
//  2 : Level 1 Data Cache enable
//  8 : System protection
//  9 : ROM protection
// 12 : Instruction Cache enable
// 13 : Location of exception vectors

	mov	r1, #0
	mcr	p15, #0, r1, c7, c5, #0	// Invalidate ICache
	mrc	p15, #0, r0, c1, c0, #0 // Read Control Register
	bic	r0, #5 // 2 + 0
	// SR=01, read-only
	bic	r0, #0x100 // 8
	orr	r0, #2 // 1
	orr	r0, #0x3200 // 13 + 12 + 9
	mcr	p15, #0, r0, c1, c0, #0 // Write Control Register
	msr	cpsr_c, #0xdf // SYS mode
	ldr	sp, 1f

	ldr	r2, 3f
	ldr	r1, 4f
	adr	r0, _start
	subs	r2, r0, r2
	push	{r0-r1}
	add	r1, r0
	blne	apply_reloc
	add	r3, r1, #3
	pop	{r0-r1}
	bic	r3, #3

	ldr	r2, 5f
	push	{r4}
	bl	entry_main
	add	sp, #4
	mov	r3, r0
	mov	r0, r4
	ldr	r1, [r4, #0x10]	
	ldr	r2, [r4, #0x14]	
	ldr	pc, [r4, #0x18]	// entry_main

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

