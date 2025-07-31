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

CODE32_FN scr_update_6x5d3_asm
	push	{r4-r10,lr}
	sub	r2, r1, #256 * 2
	ldr	r10, =0x07e0f81f
	mov	r3, #240
	bic	lr, r10, r10, lsl #1
1:	sub	r3, #160 << 16
2:	ldrb	r5, [r0, #160]
	ldrb	r4, [r0], #1
	lsl	r5, #1
	lsl	r4, #1
	ldrh	r5, [r2, r5]
	ldrh	r4, [r2, r4]
	orr	r5, r5, r5, lsl #16
	orr	r4, r4, r4, lsl #16
	str	r4, [r1], #4
	str	r5, [r1, #160 * 2 * 4 - 4]
	and	r4, r10
	and	r5, r10
	add	r4, r5
	and	r5, lr, r4, lsr #1
	add	r4, r5
	ldrb	r5, [r0, #160 * 2 - 1]
	and	r4, r10, r4, lsr #1
	lsl	r5, #1
	orr	r4, r4, r4, ror #16
	ldrh	r5, [r2, r5]
	str	r4, [r1, #160 * 4 - 4]
	orr	r5, r5, r5, lsl #16
	str	r5, [r1, #160 * 3 * 4 - 4]
	str	r5, [r1, #160 * 4 * 4 - 4]
	adds	r3, #1 << 16
	bmi	2b
	add	r0, #160 * 2
	add	r1, #160 * 4 * 4
	subs	r3, #5
	bhi	1b
	pop	{r4-r10,pc}

CODE32_FN scr_update_9x8d9_asm
	push	{r4-r10,lr}
	sub	r2, r1, #256 * 2
	ldr	r10, =0x07e0f81f
	mov	r3, #128
	bic	lr, r10, r10, lsl #1
	ldr	r9, =0x1fe
1:	sub	r3, #(160 * 7) << 16
2:	ldr	r7, [r0], #4
	adds	r3, #4 << 16
	and	r4, r9, r7, lsl #1
	and	r5, r9, r7, lsr #7
	and	r6, r9, r7, lsr #15
	and	r7, r9, r7, lsr #23
	ldrh	r4, [r2, r4]
	ldrh	r5, [r2, r5]
	ldrh	r6, [r2, r6]
	ldrh	r7, [r2, r7]
	orr	r4, r4, r5, lsl #16
	orr	r6, r6, r7, lsl #16
	stmia	r1!, {r4,r6}
	bmi	2b

	sub	r3, #160 << 16
2:	ldrb	r5, [r0, #160]
	ldrb	r4, [r0], #1
	lsl	r5, #1
	lsl	r4, #1
	ldrh	r5, [r2, r5]
	ldrh	r4, [r2, r4]
	adds	r3, #1 << 16
	orr	r4, r4, r5, lsl #16
	and	r5, r10, r4, ror #16
	and	r4, r10
	add	r4, r5
	and	r5, lr, r4, lsr #1
	add	r4, r5
	and	r4, r10, r4, lsr #1
	orr	r4, r4, r4, lsr #16
	strh	r4, [r1], #2
	bmi	2b
	add	r0, #160
	subs	r3, #8
	bhi	1b
	pop	{r4-r10,pc}

CODE32_FN scr_update_128x64_asm
	push	{r4-r11,lr}
	sub	r2, r1, #256
	mov	r3, #144
	mov	r10, #3 << 18
	ldr	r11, =0x2d83
	mov	r8, #0	// a0
	mov	r9, #0	// a1
	mov	lr, #0
1:	sub	r3, #128 << 16
2:
3:	add	lr, #9
4:	ldrb	r4, [r0, #1]
	ldrb	r5, [r0, #3]
	ldrb	r4, [r2, r4]
	ldrb	r5, [r2, r5]
	ldrb	r7, [r0, #2]
	orr	r6, r4, r5, lsl #16 // t1
	ldrb	r5, [r0, #4]
	ldrb	r4, [r0], #160	// s2 += 160
	ldrb	r5, [r2, r5]
	ldrb	r4, [r2, r4]
	ldrb	r7, [r2, r7]
	orr	r4, r4, r5, lsl #16 // t0
	orr	r7, r7, r7, lsl #16 // t2

	add	r4, r6, r4, lsl #2	// t0 = t0 * 4 + t1
	add	r7, r6			// t2 += t1
	add	r5, r6, r7, lsl #1	// t1 += t2 * 2

	add	r8, r8, r4, lsl #2	// a0 += t0 * 4
	add	r9, r9, r5, lsl #2	// a1 += t1 * 4
	subs	lr, #4
	bhi	4b
	mul	r4, lr, r4	// t0
	mul	r5, lr, r5	// t1
	add	r6, r8, r4	// a0
	add	r7, r9, r5	// a1
	rsb	r8, r4, #0	// a0
	rsb	r9, r5, #0	// a1

	lsr	r5, r7, #16		// b
	eor	r4, r7, r5, lsl #16	// a
	mla	r5, r11, r5, r10
	mla	r4, r11, r4, r10
	lsr	r5, #20
	lsr	r4, #20
	strb	r5, [r1, #2]
	strb	r4, [r1, #1]

	lsr	r5, r6, #16		// b
	eor	r4, r6, r5, lsl #16	// a
	mla	r5, r11, r5, r10
	mla	r4, r11, r4, r10
	lsr	r5, #20
	lsr	r4, #20
	strb	r5, [r1, #3]
	strb	r4, [r1], #128
	bne	3b
	sub	r0, #160 * 9
	sub	r1, #128 * 4
	add	r0, #5
	add	r1, #4
	adds	r3, #4 << 16
	bmi	2b
	add	r0, #160 * (9 - 1)
	add	r1, #128 * (4 - 1)
	subs	r3, #9
	bhi	1b
	pop	{r4-r11,pc}

