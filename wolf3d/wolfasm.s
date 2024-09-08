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

CODE32_FN ScaleLineAsm
	push	{r4-r11,lr}
	ldr	r5, =vbuf
	ldr	lr, =bufferPitch
	ldr	r5, [r5]
	ldr	lr, [lr]
	ldr	r6, [sp, #9 * 4] // linecmds
	add	r0, r5
3:	ldrb	r10, [r6], #6
	ldrb	r4, [r6, #-5]
	orr	r10, r10, r4, lsl #8
	lsl	r10, #16
	asrs	r10, #17	// end
	beq	9f
	ldrb	r11, [r6, #-4]
	ldrb	r4, [r6, #-3]
	ldrb	r12, [r6, #-2]
	ldrb	r5, [r6, #-1]
	orr	r11, r11, r4, lsl #8
	orr	r12, r12, r5, lsl #8
	lsl	r11, #16
	lsl	r12, #16
	asr	r12, #17
	mul	r9, r2, r12
	add	r11, r3, r11, asr #16
	// endpix = (frac >> FRACBITS) + toppix
	add	r8, r1, r9, asr #16
	subs	r12, r10
	bge	3b
	ldr	r4, =viewheight
	add	r11, r10
	ldr	r10, [r4]
2:	cmp	r8, r10
	bge	3b
	add	r9, r2		// frac += fracstep
	bic	r7, r8, r8, asr #31
	// endpix = (frac >> FRACBITS) + toppix
	adds	r8, r1, r9, asr #16
	bmi	5f
	cmp	r8, r10
	movgt	r8, r10
	subs	r5, r8, r7
	ble	5f
	mla	r7, lr, r7, r0
	ldrb	r4, [r11, r12]
1:	subs	r5, #2
	strb	r4, [r7], lr
	strbpl	r4, [r7], lr
	bhi	1b
5:	adds	r12, #1
	bne	2b
	b	3b
9:	pop	{r4-r11,pc}

CODE32_FN pal_update16_asm
	push	{r4,lr}
	mov	r12, #256
	sub	r1, #256 * 2
1:	ldrb	r2, [r0, #1]
	ldrb	r3, [r0, #2]
	ldrb	r4, [r0], #4
	lsr	r2, #2
	lsr	r3, #3
	lsr	r4, #3
	orr	r3, r3, r2, lsl #5
	orr	r3, r3, r4, lsl #11
	strh	r3, [r1], #2
	subs	r12, #1
	bne	1b
	pop	{r4,pc}

CODE32_FN pal_update32_asm
	push	{r4,lr}
	mov	r12, #256
	sub	r1, #256 * 4
1:	ldrb	r2, [r0, #1]
	ldrb	r3, [r0, #2]
	ldrb	r4, [r0], #4
	lsl	r3, #11
	orr	r3, r3, r2, lsl #1
	orr	r3, r3, r4, lsl #22
	str	r3, [r1], #4
	subs	r12, #1
	bne	1b
	pop	{r4,pc}

CODE32_FN scr_repeat_asm
	push	{r4-r5,lr}
	mov	r3, r1
	mov	r4, r1
	mov	r5, r1
1:	stmia	r0!, {r1,r3-r5}
	subs	r2, #8
	bne	1b
	pop	{r4-r5,pc}

.macro UPDATE_COPY4 r6, r7, r8, r9
	and	\r6, r12, \r9, lsl #1
	and	\r7, r12, \r9, lsr #7
	and	\r8, r12, \r9, lsr #15
	and	\r9, r12, \r9, lsr #23
	ldrh	\r6, [r2, \r6]
	ldrh	\r7, [r2, \r7]
	ldrh	\r8, [r2, \r8]
	ldrh	\r9, [r2, \r9]
	orr	\r6, \r6, \r7, lsl #16
	orr	\r7, \r8, \r9, lsl #16
.endm

CODE32_FN scr_update_1d1_asm
	push	{r6-r11,lr}
	ldr	r12, =0x1fe
1:	ldmia	r1!, {r9,r11}
	UPDATE_COPY4 r6, r7, r8, r9
	UPDATE_COPY4 r8, r9, r10, r11
	stmia	r0!, {r6-r9}
	subs	r3, #8
	bne	1b
	pop	{r6-r11,pc}

CODE32_FN scr_update_1d2_asm
	push	{r4-r11,lr}
	ldr	lr, =0x00400802
	ldr	r5, =0xf81f07e0
	mov	r4, #0x3fc
1:	sub	r3, #320 << 16
2:	ldr	r11, [r1, #320]
	ldr	r10, [r1], #4
	and	r8, r4, r11, lsl #2
	and	r9, r4, r11, lsr #6
	and	r6, r4, r10, lsl #2
	and	r7, r4, r10, lsr #6
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	ldr	r9, [r2, r9]
	add	r6, r7
	add	r8, r9
	add	r6, r8
	and	r7, lr, r6, lsr #2
	add	r6, lr
	add	r6, r7
	and	r6, r5
	and	r8, r4, r11, lsr #14
	and	r9, r4, r11, lsr #22
	orr	r11, r6, r6, lsl #16
	lsr	r11, #16
	and	r6, r4, r10, lsr #14
	and	r7, r4, r10, lsr #22
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	ldr	r9, [r2, r9]
	add	r6, r7
	add	r8, r9
	add	r6, r8
	and	r7, lr, r6, lsr #2
	add	r6, lr
	add	r6, r7
	and	r6, r5
	orr	r6, r6, r6, lsr #16
	orr	r6, r11, r6, lsl #16
	str	r6, [r0], #4
	adds	r3, #4 << 16
	bmi	2b
	add	r1, #320
	subs	r3, #2
	bhi	1b
	pop	{r4-r11,pc}

.macro READ2X2
	mov	r4, #0x3fc
	ldrh	r8, [r1, r12]
	ldrh	r6, [r1], #2
	and	r9, r4, r8, lsr #6
	and	r7, r4, r6, lsr #6
	and	r6, r4, r6, lsl #2
	and	r8, r4, r8, lsl #2
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	ldr	r9, [r2, r9]
.endm

.macro SCALE15X_12 r6, r7, lsl, lsr
	add	r11, \r6, \r7
	and	r4, r11, lr, lsl #1
	add	r4, r4, r11, lsl #1
	and	r11, r10, \r6, lsl #2
	orr	r11, r11, r11, \lsl #16
	and	r4, r10
	\lsr	r11, #16
	orr	r4, r4, r4, \lsr #16
	orr	r11, r11, r4, \lsl #16
.endm

CODE32_FN scr_update_3d2_asm
	push	{r4-r11,lr}
	mov	r12, #480
	ldr	lr, =0x00400802
	ldr	r10, =0xf81f07e0
1:	sub	r3, #320 << 16
2:
	READ2X2
	SCALE15X_12 r6, r7, lsl, lsr
	str	r11, [r0], #4

	and	r5, r10, r7, lsl #2
	orr	r5, r5, r5, lsr #16
	lsl	r5, #16

	add	r7, r9
	add	r11, r6, r8
	and	r4, r11, lr, lsl #1
	add	r4, r4, r11, lsl #1
	and	r4, r10
	orr	r4, r4, r4, lsl #16
	add	r11, r7
	and	r6, lr, r11, lsr #2
	add	r11, r6
	add	r11, lr
	and	r11, r10
	orr	r11, r11, r11, lsr #16
	lsl	r11, #16
	orr	r11, r11, r4, lsr #16
	str	r11, [r0, #480 * 2 - 4]

	and	r4, r7, lr, lsl #1
	add	r4, r4, r7, lsl #1
	and	r4, r10
	orr	r4, r4, r4, lsl #16
	orr	r5, r5, r4, lsr #16

	SCALE15X_12 r8, r9, lsl, lsr
	str	r11, [r0, #480 * 4 - 4]

	and	r4, r10, r9, lsl #2
	orr	r11, r4, r4, lsl #16

	READ2X2

	and	r4, r10, r8, lsl #2
	orr	r4, r4, r4, lsr #16
	lsl	r4, #16
	orr	r4, r4, r11, lsr #16
	str	r4, [r0, #480 * 4]

	and	r4, r10, r6, lsl #2
	orr	r4, r4, r4, lsr #16
	lsl	r4, #16
	orr	r4, r4, r5, lsr #16
	str	r4, [r0], #4
	lsl	r5, #16
	orr	r5, r5, r11, lsr #16

	SCALE15X_12 r7, r6, lsr, lsl
	str	r11, [r0], #4

	add	r11, r6, r8
	and	r4, r11, lr, lsl #1
	add	r4, r4, r11, lsl #1
	and	r4, r10
	orr	r4, r4, r4, lsr #16
	lsl	r4, #16
	orr	r4, r4, r5, lsr #16
	str	r4, [r0, #480 * 2 - 8]

	add	r7, r9
	add	r11, r7
	and	r4, lr, r11, lsr #2
	add	r11, r4
	add	r11, lr
	and	r11, r10
	orr	r11, r11, r11, lsl #16
	lsr	r11, #16
	and	r4, r7, lr, lsl #1
	add	r4, r4, r7, lsl #1
	and	r4, r10
	orr	r4, r4, r4, lsr #16
	orr	r11, r11, r4, lsl #16
	str	r11, [r0, #480 * 2 - 4]

	SCALE15X_12 r9, r8, lsr, lsl
	str	r11, [r0, #480 * 4 - 4]

	adds	r3, #4 << 16
	bmi	2b
	add	r1, #480 * 2 - 320
	add	r0, #480 * 4
	subs	r3, #2
	bhi	1b
	pop	{r4-r11,pc}

CODE32_FN scr_update_5x4d4_asm
	push	{r4-r11,lr}
	ldr	r9, =0x08210821
	ldr	r12, =0x1fe
	mvn	r8, r9, lsr #1
1:	sub	r3, #320 << 16
2:	ldr	r7, [r1], #4
	and	r4, r12, r7, lsl #1
	and	r5, r12, r7, lsr #7
	and	r6, r12, r7, lsr #15
	and	r7, r12, r7, lsr #23
	ldrh	r4, [r2, r4]
	ldrh	r5, [r2, r5]
	ldrh	r6, [r2, r6]
	ldrh	r7, [r2, r7]
	strh	r4, [r0], #10
	and	r10, r8, r5, lsr #1
	strh	r5, [r0, #-8]
	and	r4, r8, r6, lsr #1
	add	r10, r4
	orr	r11, r5, r6
	and	r5, r6
	and	r11, r10
	orr	r5, r11
	and	r5, r9
	add	r5, r10
	strh	r5, [r0, #-6]
	strh	r6, [r0, #-4]
	strh	r7, [r0, #-2]
	adds	r3, #4 << 16
	bmi	2b
	add	r1, #80
	subs	r3, #1
	bhi	1b
	pop	{r4-r11,pc}

