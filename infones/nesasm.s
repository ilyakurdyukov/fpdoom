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

CODE32_FN scr_update_1d1_asm
	push	{r4-r5,lr}
	ldr	lr, =~0x200020
	lsl	r2, #8
1:	ldmia	r0!, {r3-r5,r12}
	subs	r2, #8
	and	r3, lr
	and	r4, lr
	and	r5, lr
	and	r12, lr
	stmia	r1!, {r3-r5,r12}
	bhi	1b
	pop	{r4-r5,pc}

// more complicated version than C reference
CODE32_FN scr_update_5x4d4_asm
	push	{r4-r11,lr}
	ldr	lr, =0x7fffffff - (0x08410841 >> 1)
	ldr	r11, =~0x200020
	lsl	r2, #8
1:	ldmia	r0!, {r3-r5,r9}
	subs	r2, #8
	and	r3, r11
	and	r4, r11
	and	r5, r11
	and	r9, r11

	lsr	r7, r5, #16
	lsl	r5, #16
	orr	r5, r5, r4, lsr #16
	lsl	r4, #16

	lsr	r8, r3, #16
	orr	r6, r4, r7
	orr	r8, r8, r9, lsl #16

	and	r10, r6, r8, ror #16
	orr	r12, r6, r8, ror #16
	and	r6, lr, r6, lsr #1
	and	r8, lr, r8, ror #17
	add	r6, r8
	and	r12, r6
	orr	r10, r12
	bic	r10, r10, lr, lsl #1
	add	r6, r10
	orr	r4, r4, r6, lsr #16
	orr	r6, r7, r6, lsl #16

	stmia	r1!, {r3-r6,r9}
	bhi	1b
	pop	{r4-r11,pc}

CODE32_FN scr_update_1d2_asm
	ldr	r12, =0x07c0f81f
	push	{r4-r6,lr}
	bic	lr, r12, r12, lsl #1 // 0x00400801
1:	sub	r2, #256 << 16
2:	ldr	r6, [r0, #512]
	ldr	r4, [r0], #4
	and	r3, r12, r6
	and	r5, r12, r4
	and	r6, r12, r6, ror #16
	and	r4, r12, r4, ror #16
	add	r5, r3
	add	r4, r6
	add	r4, r5
	// rounding half to even
	and	r5, lr, r4, lsr #2
	add	r4, lr
	add	r4, r5
	and	r4, r12, r4, lsr #2
	orr	r4, r4, r4, lsr #16
	strh	r4, [r1], #2
	adds	r2, #2 << 16
	bmi	2b
	add	r0, #512
	subs	r2, #2
	bhi	1b
	pop	{r4-r6,pc}

CODE32_FN scr_update_5x4d8_asm
	ldr	r12, =0x07c0f81f
	push	{r4-r11,lr}
	bic	lr, r12, r12, lsl #1
1:	sub	r2, #256 << 16
2:	ldr	r11, [r0, #512]
	ldr	r10, [r0], #4	// 01

	and	r9, r12, r11
	and	r7, r12, r10
	and	r6, r12, r11, ror #16
	and	r4, r12, r10, ror #16
	add	r7, r9
	add	r6, r4
	add	r6, r7

	and	r9, lr, r6, lsr #2
	add	r6, lr
	add	r6, r9
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r1], #2	// 0011

	lsr	r9, r11, #16
	lsr	r7, r10, #16
	orr	r7, r9, r7, lsl #16
	and	r9, r12, r7
	and	r7, r12, r7, ror #16
	add	r5, r7, r9

	ldr	r11, [r0, #512]
	ldr	r10, [r0], #4	// 23

	lsl	r8, r11, #16
	lsl	r6, r10, #16
	orr	r6, r8, r6, lsr #16
	and	r8, r12, r6
	and	r6, r12, r6, ror #16

	lsr	r9, r11, #16
	lsr	r7, r10, #16
	orr	r7, r9, r7, lsl #16
	and	r9, r12, r7
	and	r7, r12, r7, ror #16

	add	r6, r8
	add	r6, r5, r6, lsl #1
	add	r5, r7, r9
	add	r6, r5
	and	r9, lr, r6, lsr #3
	add	r6, lr
	add	r6, r6, lr, lsl #1
	add	r6, r9
	and	r6, r12, r6, lsr #3
	orr	r6, r6, r6, lsr #16
	strh	r6, [r1], #2	// 1223

	ldr	r11, [r0, #512]
	ldr	r10, [r0], #4	// 45

	lsl	r8, r11, #16
	lsl	r6, r10, #16
	orr	r6, r8, r6, lsr #16
	and	r8, r12, r6
	and	r6, r12, r6, ror #16

	lsr	r9, r11, #16
	lsr	r7, r10, #16
	orr	r7, r9, r7, lsl #16
	and	r9, r12, r7
	and	r7, r12, r7, ror #16

	add	r6, r8
	add	r7, r9
	add	r9, r5, r6
	add	r5, r6, r7, lsl #1
	and	r6, lr, r9, lsr #2
	add	r6, lr
	add	r6, r9
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r1], #2	// 3344

	ldr	r11, [r0, #512]
	ldr	r10, [r0], #4	// 67

	lsl	r8, r11, #16
	lsl	r6, r10, #16
	orr	r6, r8, r6, lsr #16
	and	r8, r12, r6
	and	r6, r12, r6, ror #16

	add	r6, r8
	add	r6, r5
	and	r9, lr, r6, lsr #3
	add	r6, lr
	add	r6, r6, lr, lsl #1
	add	r6, r9
	and	r6, r12, r6, lsr #3
	orr	r6, r6, r6, lsr #16
	strh	r6, [r1], #2	// 4556

	and	r9, r12, r11
	and	r7, r12, r10
	and	r6, r12, r11, ror #16
	and	r4, r12, r10, ror #16
	add	r7, r9
	add	r6, r4
	add	r6, r7

	and	r9, lr, r6, lsr #2
	add	r6, lr
	add	r6, r9
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r1], #2	// 6677

	adds	r2, #8 << 16
	bmi	2b
	add	r0, #512
	subs	r2, #2
	bhi	1b
	pop	{r4-r11,pc}

.macro SCALE_3D2_V1 inc, r6, r7
	ldrh	r4, [r0], #2
	ldrh	r5, [r3], #2
	bic	r4, #0x20
	bic	r5, #0x20
	strh	r4, [r1, -r11]
	strh	r5, [r1, r11]
	orr	r4, r4, r5, lsl #16
	and	\r6, r12, r4 // a0
	and	\r7, r12, r4, ror #16 // a1
	add	r5, \r6, \r7
	and	r4, r8, r5, lsr #1
	add	r5, r4
	and	r5, r12, r5, lsr #1
	orr	r5, r5, r5, lsr #16
	strh	r5, [r1], #\inc * 2
.endm
.macro SCALE_3D2_V2 inc
	add	r6, r9	// a0 + b0
	add	r7, lr	// a1 + b1

	and	r4, r8, r6, lsr #1
	and	r5, r8, r7, lsr #1
	add	r4, r6
	add	r5, r7
	and	r4, r12, r4, lsr #1
	and	r5, r12, r5, lsr #1
	orr	r4, r4, r5, ror #16
	lsr	r5, r4, #16
	strh	r4, [r1, -r11]
	strh	r5, [r1, r11]
	add	r4, r6, r7
	and	r5, r8, r4, lsr #2
	add	r4, r8
	add	r4, r5
	and	r4, r12, r4, lsr #2
	orr	r4, r4, r4, lsr #16
	strh	r4, [r1], #\inc * 2
.endm

CODE32_FN scr_update_4d3_asm
	ldr	r12, =0x07c0f81f
	push	{r4-r11,lr}
	bic	r8, r12, r12, lsl #1 // 0x400801
1:	sub	r2, #255 << 16
	mov	r11, #342 * 2
	add	r1, r11
	add	r3, r0, #512
2:	SCALE_3D2_V1 1, r6, r7
	SCALE_3D2_V1 2, r6, r7
	SCALE_3D2_V1 -1, r9, lr
	SCALE_3D2_V2 2
	adds	r2, #3 << 16
	bmi	2b
	SCALE_3D2_V1 2, r6, r7

	subs	r2, #2
	bls	9f
	add	r0, #512
	add	r1, r11
	ldr	r11, =~0x200020
	sub	r2, #255 << 16
3:	ldrh	r4, [r0], #6
	ldrh	r5, [r0, #-4]
	ldrh	r6, [r0, #-2]
	orr	r4, r4, r5, lsl #16
	and	r4, r11
	and	r6, r11, r6, lsl #16
	orr	r5, r6, r4, lsr #16
	and	r7, r12, r5, ror #16
	and	r5, r12, r5
	add	r5, r7
	and	r7, r8, r5, lsr #1
	add	r5, r7
	and	r5, r12, r5, lsr #1
	orr	r5, r5, r5, lsl #16
	orr	r6, r6, r5, lsr #16
	stmia	r1!, {r4,r6}
	adds	r2, #3 << 16
	bmi	3b
	ldrh	r4, [r0], #2
	subs	r2, #1
	bic	r4, #0x20
	strh	r4, [r1], #4
	bhi	1b
9:	pop	{r4-r11,pc}

CODE32_FN scr_update_5x4d3_asm
	ldr	r12, =0x07c0f81f
	push	{r4-r11,lr}
	bic	r8, r12, r12, lsl #1 // 0x400801
	mov	r11, #426 * 2
1:	sub	r2, #255 << 16
	add	r1, r11
	add	r3, r0, #512
2:	SCALE_3D2_V1 2, r6, r7
	SCALE_3D2_V1 -1, r9, lr
	SCALE_3D2_V2 3
	SCALE_3D2_V1 -1, r6, r7
	SCALE_3D2_V2 2
	adds	r2, #3 << 16
	bmi	2b
	SCALE_3D2_V1 1, r6, r7

	subs	r2, #2
	bls	9f
	add	r0, #512
	add	r1, r11
	sub	r2, #255 << 16
3:	ldrh	r4, [r0], #6
	ldrh	r5, [r0, #-4]
	bic	r4, #0x20
	bic	r5, #0x20
	strh	r4, [r1], #10
	strh	r5, [r1, #-6]
	orr	r4, r4, r5, lsl #16
	and	r7, r12, r4
	and	r6, r12, r4, ror #16
	add	r7, r6
	and	r6, r8, r7, lsr #1
	add	r7, r6
	and	r7, r12, r7, lsr #1
	orr	r7, r7, r7, lsr #16
	ldrh	r4, [r0, #-2]
	strh	r7, [r1, #-8]
	bic	r4, #0x20
	strh	r4, [r1, #-2]
	orr	r4, r4, r5, lsl #16
	and	r7, r12, r4
	and	r6, r12, r4, ror #16
	add	r7, r6
	and	r6, r8, r7, lsr #1
	add	r7, r6
	and	r7, r12, r7, lsr #1
	orr	r7, r7, r7, lsr #16
	strh	r7, [r1, #-4]
	adds	r2, #3 << 16
	bmi	3b
	ldrh	r4, [r0], #2
	subs	r2, #1
	bic	r4, #0x20
	strh	r4, [r1], #2
	bhi	1b
9:	pop	{r4-r11,pc}

