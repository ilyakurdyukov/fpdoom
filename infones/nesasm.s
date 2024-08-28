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

.macro UPDATE_COPY4 r6, r7
	and	lr, \r6, r4
	and	r12, \r7, r4
	and	\r6, r5
	and	\r7, r5
	add	\r6, lr
	add	\r7, r12
.endm

CODE32_FN scr_update_1d1_asm
	push	{r4-r9,lr}
	ldr	r4, =0x7fff7fff
	ldr	r5, =0x7fe07fe0
	lsl	r2, #8
1:	ldmia	r0!, {r6-r9}
	UPDATE_COPY4 r6, r7
	UPDATE_COPY4 r8, r9
	stmia	r1!, {r6-r9}
	subs	r2, #8
	bhi	1b
	pop	{r4-r9,pc}

CODE32_FN scr_update_1d2_asm
	push	{r4-r11,lr}
	ldr	r8, =0x03e07c1f
	bic	r9, r8, r8, lsl #1 // 0x00200401
1:	sub	r2, #256 << 16
2:	ldr	r6, [r0, #512]
	ldr	r4, [r0], #4
	and	r7, r6, r8
	and	r5, r4, r8
	and	r6, r8, r6, ror #16
	and	r4, r8, r4, ror #16
	add	r5, r7
	add	r4, r6
	add	r4, r5
	// rounding half to even
	and	r5, r9, r4, lsr #2
	add	r4, r9
	add	r4, r5
	and	r4, r8, r4, lsr #2
	orr	r4, r4, r4, lsr #16
	bic	r5, r4, #0x1f
	add	r4, r5
	strh	r4, [r1], #2
	adds	r2, #2 << 16
	bmi	2b
	add	r0, #512
	subs	r2, #2
	bhi	1b
	pop	{r4-r11,pc}

CODE32_FN scr_update_3d2_asm
	push	{r4-r11,lr}
	ldr	r8, =0x07c0f81f
	bic	r12, r8, r8, lsl #1 // 0x00400801
	ldr	r10, =0x7fff7fff
	ldr	r11, =0x7fe07fe0
1:	sub	r2, #256 << 16
2:	ldr	r6, [r0, #512]
	ldr	r4, [r0], #4
	and	r5, r4, r10
	and	r7, r6, r10
	and	r4, r11
	and	r6, r11
	add	r4, r5
	add	r6, r7

	and	r9, r8, r4, ror #16
	and	lr, r8, r4
	add	r5, r9, lr
	and	r7, r12, r5, lsr #1
	add	r5, r7
	and	r5, r8, r5, lsr #1
	lsr	r7, r4, #16
	orr	r5, r5, r5, lsr #16
	add	r3, r1, #0x180 * 4
	strh	r5, [r1, #2]
	strh	r7, [r1, #4]
	strh	r4, [r1], 6

	and	r5, r8, r6, ror #16
	and	r7, r8, r6
	add	r9, r5 // a1 + b1
	add	lr, r7 // a0 + b0
	add	r5, r7
	and	r7, r12, r5, lsr #1
	add	r5, r7
	and	r5, r8, r5, lsr #1
	lsr	r7, r6, #16
	orr	r5, r5, r5, lsr #16
	strh	r6, [r3]
	strh	r5, [r3, #2]
	strh	r7, [r3, #4]
	sub	r3, #0x180 * 2

	add	r5, lr, r9 // ab
	and	r4, r12, lr, lsr #1
	and	r6, r12, r9, lsr #1
	and	r7, r12, r5, lsr #2
	add	r5, r12
	add	r4, lr
	add	r6, r9
	add	r5, r7
	and	r4, r8, r4, lsr #1
	and	r6, r8, r6, lsr #1
	and	r5, r8, r5, lsr #2
	orr	r4, r4, r6, ror #16
	orr	r5, r5, r5, lsr #16

	lsr	r7, r4, #16
	strh	r4, [r3]
	strh	r5, [r3, #2]
	strh	r7, [r3, #4]
	adds	r2, #2 << 16
	bmi	2b
	add	r0, #512
	add	r1, #0x180 * 4
	subs	r2, #2
	bhi	1b
	pop	{r4-r11,pc}

CODE32_FN scr_update_4x3d2_asm
	push	{r4-r11,lr}
	ldr	r9, =0x08410841
	mvn	r8, r9, lsr #1
	ldr	r10, =0x7fff7fff
	ldr	r11, =0x7fe07fe0
	add	r0, #16
1:	sub	r2, #240 << 16
2:	ldr	r6, [r0, #512]
	ldr	r4, [r0], #4
	and	r5, r4, r10
	and	r7, r6, r10
	and	r4, r11
	and	r6, r11
	add	r4, r5
	add	r6, r7
	eor	r7, r4, r4, ror #16
	eor	r5, r4, r7, lsl #16
	eor	r7, r4, r7, lsr #16
	add	r3, r1, #480 * 4
	stmia	r1!, {r5,r7}
	eor	r7, r6, r6, ror #16
	eor	r5, r6, r7, lsl #16
	eor	r7, r6, r7, lsr #16
	stmia	r3, {r5,r7}

	and	r5, r8, r4, lsr #1
	and	r7, r8, r6, lsr #1
	add	r5, r7
	orr	r7, r4, r6
	and	r4, r6
	and	r7, r5
	orr	r4, r7
	and	r4, r9
	add	r4, r5

	sub	r3, #480 * 2
	eor	r7, r4, r4, ror #16
	eor	r5, r4, r7, lsl #16
	eor	r7, r4, r7, lsr #16
	stmia	r3, {r5,r7}
	adds	r2, #2 << 16
	bmi	2b
	add	r0, #512 + 32
	add	r1, #480 * 4
	subs	r2, #2
	bhi	1b
	pop	{r4-r11,pc}

