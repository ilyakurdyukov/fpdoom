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
	ldr	r5, =0x7fe07fe0
	lsl	r2, #8
	orr	r4, r5, r5, lsr #5 // 0x7fff7fff
1:	ldmia	r0!, {r6-r9}
	UPDATE_COPY4 r6, r7
	UPDATE_COPY4 r8, r9
	stmia	r1!, {r6-r9}
	subs	r2, #8
	bhi	1b
	pop	{r4-r9,pc}

.macro SCALE_3NX1_4D3
3:	ldrh	r4, [r0], #6
	ldrh	r5, [r0, #-4]
	ldrh	r6, [r0, #-2]
	orr	r4, r4, r5, lsl #16
	and	r5, r4, r10
	and	r4, r11
	add	r4, r5
	and	r7, r9, r6, lsl #16
	add	r6, r7, r6, lsl #16
	orr	r5, r6, r4, lsr #16
	and	r7, r8, r5, ror #16
	and	r5, r8, r5
	add	r5, r7
	and	r7, r12, r5, lsr #1
	add	r5, r7
	and	r5, r8, r5, lsr #1
	orr	r5, r5, r5, lsl #16
	orr	r6, r6, r5, lsr #16
	stmia	r1!, {r4,r6}
	adds	r2, #3 << 16
	bmi	3b
.endm

CODE32_FN scr_update_4x3d3_asm
	push	{r4-r11,lr}
	add	r0, #16
	ldr	r8, =0x07c0f81f
	ldr	r11, =0x7fe07fe0
	bic	r12, r8, r8, lsl #1 // 0x00400801
	orr	r10, r11, r11, lsr #5 // 0x7fff7fff
	mvn	r9, #0x1f << 16
1:	sub	r2, #240 << 16
	SCALE_3NX1_4D3
	add	r0, #32
	subs	r2, #1
	bhi	1b
9:	pop	{r4-r11,pc}

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

CODE32_FN scr_update_4x3d6_asm
	push	{r4-r11,lr}
	add	r0, #16
	ldr	r12, =0x1f001f
	ldr	r11, =0x400040
	rsb	r10, r11, r11, lsl #3 // 0x1c001c0
	mov	lr, #0xab
1:	sub	r2, #240 << 16
2:	add	r9, r0, #512
	ldrh	r5, [r0, #2]
	ldrh	r6, [r0, #4]
	ldrh	r4, [r0], #6
	ldrh	r7, [r9]
	ldrh	r8, [r9, #2]
	ldrh	r9, [r9, #4]
	orr	r4, r4, r6, lsl #16 // a1 |= a3 << 16
	orr	r6, r7, r9, lsl #16 // a3 = b1 | b3 << 16
	orr	r5, r5, r8, lsl #16 // a2 |= b2 << 16

	and	r8, r12, r4, lsr #10
	and	r9, r12, r6, lsr #10
	add	r8, r9
	and	r9, r12, r5, lsr #10
	add	r9, r9, r9, ror #16
	add	r7, r9, r8, lsl #1

	and	r8, r12, r4, lsr #5
	and	r9, r12, r6, lsr #5
	add	r8, r9
	and	r9, r12, r5, lsr #5
	add	r9, r9, r9, ror #16
	add	r8, r9, r8, lsl #1

	and	r4, r12
	and	r6, r12
	add	r4, r6
	and	r9, r12, r5
	add	r9, r9, r9, ror #16
	add	r9, r9, r4, lsl #1

	mla	r4, r7, lr, r10
	mla	r5, r8, lr, r10
	mla	r6, r9, lr, r10
	// rounding half to even
	and	r7, r11, r4, lsr #4
	and	r8, r11, r5, lsr #4
	and	r9, r11, r6, lsr #4
	add	r4, r7
	add	r5, r8
	add	r6, r9
	and	r4, r12, r4, lsr #10
	and	r5, r12, r5, lsr #10
	and	r6, r12, r6, lsr #10
	orr	r5, r5, r4, lsl #5
	orr	r6, r6, r5, lsl #6
	str	r6, [r1], #4
	adds	r2, #3 << 16
	bmi	2b
	add	r0, #512 + 32
	subs	r2, #2
	bhi	1b
	pop	{r4-r11,pc}

.macro SCALE_1X2_3D2 inc
	add	r9, r0, #512
	ldrh	r4, [r0], #2
	ldrh	r5, [r9], #2
	mov	r3, #342 * 2
	orr	r4, r4, r5, lsl #16
	and	r5, r4, r10
	and	r4, r11
	add	r4, r5
	and	r6, r8, r4, ror #16
	and	lr, r8, r4
	add	r5, r6, lr
	and	r7, r12, r5, lsr #1
	add	r5, r7
	and	r5, r8, r5, lsr #1
	lsr	r7, r4, #16
	orr	r5, r5, r5, lsr #16
	lsl	r6, r3, #1
	strh	r5, [r1, r3]
	strh	r7, [r1, r6]
	strh	r4, [r1], #\inc
.endm

CODE32_FN scr_update_4d3_asm
	push	{r4-r11,lr}
	ldr	r8, =0x07c0f81f
	ldr	r11, =0x7fe07fe0
	bic	r12, r8, r8, lsl #1 // 0x00400801
	orr	r10, r11, r11, lsr #5 // 0x7fff7fff
1:	sub	r2, #255 << 16
2:
	SCALE_1X2_3D2 2

	ldrh	r5, [r0, #2]
	ldrh	r4, [r0], #4
	ldrh	r6, [r9]
	ldrh	r7, [r9, #2]
	orr	r4, r4, r5, lsl #16
	orr	r6, r6, r7, lsl #16
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
	add	r3, r1, r3, lsl #1
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
	sub	r3, #342 * 2

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
	adds	r2, #3 << 16
	bmi	2b

	SCALE_1X2_3D2 4

	subs	r2, #2
	bls	9f
	mov	r3, #342 * 2
	add	r0, #512
	add	r1, r3, lsl #1

	sub	r2, #255 << 16
	mvn	r9, #0x1f << 16
	SCALE_3NX1_4D3
	ldrh	r4, [r0], #2
	subs	r2, #1
	bic	r5, r4, #0x1f
	add	r4, r5
	strh	r4, [r1], #4
	bhi	1b
9:	pop	{r4-r11,pc}

.macro SCALE_256X2_3D2
	sub	r2, #256 << 16
2:	ldr	r6, [r0, #512]
	ldr	r4, [r0], #4
	and	r7, r6, r10
	and	r5, r4, r10
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
	strh	r4, [r1], #6

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
.endm

CODE32_FN scr_update_9x8d6_asm
	push	{r4-r11,lr}
	ldr	r8, =0x07c0f81f
	ldr	r11, =0x7fe07fe0
	bic	r12, r8, r8, lsl #1 // 0x00400801
	orr	r10, r11, r11, lsr #5 // 0x7fff7fff
1:	SCALE_256X2_3D2
	subs	r2, #2
	bls	9f
	add	r0, #512
	add	r1, #0x180 * 4

	sub	r2, #256 << 16
3:	ldr	r4, [r0], #4
	and	r5, r4, r10
	and	r4, r11
	add	r4, r5
	and	r9, r8, r4, ror #16
	and	lr, r8, r4
	add	r5, r9, lr
	and	r7, r12, r5, lsr #1
	add	r5, r7
	and	r5, r8, r5, lsr #1
	lsr	r7, r4, #16
	orr	r5, r5, r5, lsr #16
	strh	r5, [r1, #2]
	strh	r7, [r1, #4]
	strh	r4, [r1], #6
	adds	r2, #2 << 16
	bmi	3b

	subs	r2, #1
	bhi	1b
9:	pop	{r4-r11,pc}

CODE32_FN scr_update_3d2_asm
	push	{r4-r11,lr}
	ldr	r8, =0x07c0f81f
	ldr	r11, =0x7fe07fe0
	bic	r12, r8, r8, lsl #1 // 0x00400801
	orr	r10, r11, r11, lsr #5 // 0x7fff7fff
1:	SCALE_256X2_3D2
	add	r0, #512
	add	r1, #0x180 * 4
	subs	r2, #2
	bhi	1b
	pop	{r4-r11,pc}

CODE32_FN scr_update_4x3d2_asm
	push	{r4-r11,lr}
	ldr	r9, =0x08410841
	ldr	r11, =0x7fe07fe0
	mvn	r8, r9, lsr #1
	orr	r10, r11, r11, lsr #5 // 0x7fff7fff
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

