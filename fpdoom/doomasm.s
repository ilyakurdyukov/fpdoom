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

CODE32_FN FixedMulAsm
	smull	r2, r3, r0, r1
	lsr	r0, r2, #16
	orr	r0, r0, r3, lsl #16
	bx	lr

CODE32_FN FixedDivAsm
	eor	r12, r0, r1	// -quo
	eor	r2, r0, r0, asr #31
	eor	r3, r1, r1, asr #31
	sub	r2, r2, r0, asr #31
	sub	r3, r3, r1, asr #31
	mov	r0, #0x7fffffff
	lsr	r1, r2, #15
	eor	r0, r0, r12, asr #31
	lsl	r2, #17
	cmp	r1, r3
	bxhs	lr
	mov	r0, #2
1:	lsls	r2, #1
	adc	r1, r1
	cmp	r1, r3
	subhs	r1, r3
	adcs	r0, r0
	bcc	1b
	tst	r12, r12
	rsbmi	r0, #0	// quo
	bx	lr

SCREENWIDTH = 320
SCREENHEIGHT = 200

.macro pixaddress x, y, m=1
	ldr	r3, =firstpix
	mov	r1, #SCREENWIDTH
	ldr	r3, [r3]
.if \m == 2
	mla	r1, \y, r1, r3
	add	r1, r1, \x, lsl #1
.else
	mla	r1, \y, r1, \x
	add	r1, r3
.endif
.endm

CODE32_FN R_DrawColumnAsm
	push	{r4-r5,lr}
	ldr	r0, =dc_yl
	ldr	r1, =dc_yh
	ldr	r0, [r0]
	ldr	r1, [r1]
	subs	r2, r1, r0
	blt	9f
	ldr	r4, =dc_x
	ldr	r4, [r4]
	pixaddress r4, r0
	ldr	r4, =centery
	ldr	lr, =dc_iscale
	ldr	r12, =dc_texturemid	// frac
	ldr	r4, [r4]
	ldr	lr, [lr]
	ldr	r12, [r12]
	sub	r3, r0, r4
	ldr	r4, =dc_iscale
	mla	r3, lr, r3, r12
	ldr	r4, [r4]
	lsl	r3, #9
	lsl	r4, #9

	ldr	lr, =dc_source
	ldr	r5, =dc_colormap
	ldr	lr, [lr]
	ldr	r5, [r5]

	mov	r12, #SCREENWIDTH
1:	lsr	r0, r3, #16+9
	ldrb	r0, [lr, r0]
	ldrb	r0, [r5, r0]
	strb	r0, [r1], r12
	add	r3, r4
	subs	r2, #1
	bge	1b
9:	pop	{r4-r5,pc}

CODE32_FN R_DrawColumnLowAsm
	push	{r4-r5,lr}
	ldr	r0, =dc_yl
	ldr	r1, =dc_yh
	ldr	r0, [r0]
	ldr	r1, [r1]
	subs	r2, r1, r0
	blt	9f
	ldr	r4, =dc_x
	ldr	r4, [r4]
	pixaddress r4, r0, 2
	ldr	r4, =centery
	ldr	lr, =dc_iscale
	ldr	r12, =dc_texturemid	// frac
	ldr	r4, [r4]
	ldr	lr, [lr]
	ldr	r12, [r12]
	sub	r3, r0, r4
	ldr	r4, =dc_iscale
	mla	r3, lr, r3, r12
	ldr	r4, [r4]
	lsl	r3, #9
	lsl	r4, #9

	ldr	lr, =dc_source
	ldr	r5, =dc_colormap
	ldr	lr, [lr]
	ldr	r5, [r5]

	mov	r12, #SCREENWIDTH
1:	lsr	r0, r3, #16+9
	ldrb	r0, [lr, r0]
	ldrb	r0, [r5, r0]
	strb	r0, [r1, #1]
	strb	r0, [r1], r12
	add	r3, r4
	subs	r2, #1
	bge	1b
9:	pop	{r4-r5,pc}

FUZZTABLE = 50

CODE32_FN R_DrawFuzzColumnAsm
	push	{r4-r5,lr}
	ldr	r0, =dc_yl
	ldr	r1, =dc_yh
	ldr	r2, =viewheight
	ldr	r0, [r0]
	ldr	r1, [r1]
	ldr	r2, [r2]
	cmp	r0, #1
	movlt	r0, #1
	sub	r2, #2
	cmp	r1, r2
	movgt	r1, r2

	subs	r2, r1, r0
	blt	9f
	ldr	r4, =dc_x
	ldr	r4, [r4]
	pixaddress r4, r0
	ldr	r3, =colormaps
	ldr	r5, =fuzzpos
	ldr	r3, [r3]
	ldr	r4, [r5]
	add	r3, #256 * 6

	ldr	lr, =fuzzoffset
	mov	r12, #SCREENWIDTH
1:	ldr	r0, [lr, r4, lsl #2]
	ldrb	r0, [r1, r0]
	ldrb	r0, [r3, r0]
	strb	r0, [r1], r12
	add	r4, #1
	cmp	r4, #FUZZTABLE
	moveq	r4, #0
	subs	r2, #1
	bge	1b
	str	r4, [r5]
9:	pop	{r4-r5,pc}

CODE32_FN R_DrawFuzzColumnLowAsm
	push	{r4-r5,lr}
	ldr	r0, =dc_yl
	ldr	r1, =dc_yh
	ldr	r2, =viewheight
	ldr	r0, [r0]
	ldr	r1, [r1]
	ldr	r2, [r2]
	cmp	r0, #1
	movlt	r0, #1
	sub	r2, #2
	cmp	r1, r2
	movgt	r1, r2

	subs	r2, r1, r0
	blt	9f
	ldr	r4, =dc_x
	ldr	r4, [r4]
	pixaddress r4, r0, 2
	ldr	r3, =colormaps
	ldr	r5, =fuzzpos
	ldr	r3, [r3]
	ldr	r4, [r5]
	add	r3, #256 * 6

	ldr	lr, =fuzzoffset
	mov	r12, #SCREENWIDTH
1:	ldr	r0, [lr, r4, lsl #2]
	ldrb	r0, [r1, r0]
	ldrb	r0, [r3, r0]
	strb	r0, [r1, #1]
	strb	r0, [r1], r12
	add	r4, #1
	cmp	r4, #FUZZTABLE
	moveq	r4, #0
	subs	r2, #1
	bge	1b
	str	r4, [r5]
9:	pop	{r4-r5,pc}

CODE32_FN R_DrawSpanAsm
	push	{r4-r7,lr}
	ldr	r0, =ds_x1
	ldr	r1, =ds_x2
	ldr	r0, [r0]
	ldr	r1, [r1]
	subs	r2, r1, r0
	blt	9f
	ldr	r4, =ds_y
	ldr	r4, [r4]
	pixaddress r0, r4
	ldr	r5, =ds_yfrac
	ldr	r3, =ds_xfrac
	ldr	r5, [r5]
	ldr	r3, [r3]
	lsl	r5, #10
	lsl	r3, #10

	ldr	r6, =ds_ystep
	ldr	r4, =ds_xstep
	ldr	r6, [r6]
	ldr	r4, [r4]
	lsl	r6, #10
	lsl	r4, #10

	ldr	r12, =ds_source
	ldr	lr, =ds_colormap
	ldr	r12, [r12]
	ldr	lr, [lr]
1:	lsr	r0, r3, #26
	lsr	r7, r5, #26
	add	r3, r4
	add	r5, r6
	orr	r0, r0, r7, lsl #6
	ldrb	r0, [r12, r0]
	ldrb	r0, [lr, r0]
	strb	r0, [r1], #1
	subs	r2, #1
	bhs	1b
9:	pop	{r4-r7,pc}

CODE32_FN R_DrawSpanLowAsm
	push	{r4-r7,lr}
	ldr	r0, =ds_x1
	ldr	r1, =ds_x2
	ldr	r0, [r0]
	ldr	r1, [r1]
	subs	r2, r1, r0
	blt	9f
	ldr	r4, =ds_y
	ldr	r4, [r4]
	pixaddress r0, r4, 2
	ldr	r5, =ds_yfrac
	ldr	r3, =ds_xfrac
	ldr	r5, [r5]
	ldr	r3, [r3]
	lsl	r5, #10
	lsl	r3, #10

	ldr	r6, =ds_ystep
	ldr	r4, =ds_xstep
	ldr	r6, [r6]
	ldr	r4, [r4]
	lsl	r6, #10
	lsl	r4, #10

	ldr	r12, =ds_source
	ldr	lr, =ds_colormap
	ldr	r12, [r12]
	ldr	lr, [lr]
1:	lsr	r0, r3, #26
	lsr	r7, r5, #26
	add	r3, r4
	add	r5, r6
	orr	r0, r0, r7, lsl #6
	ldrb	r0, [r12, r0]
	ldrb	r0, [lr, r0]
	orr	r0, r0, r0, lsl #8
	strh	r0, [r1], #2
	subs	r2, #1
	bhs	1b
9:	pop	{r4-r7,pc}

.macro palette16 name
	lsr	r2, 2
	lsr	r3, 3
	lsr	r4, 3
	orr	r3, r3, r2, lsl #5
	orr	r3, r3, r4, lsl #11
	strh	r3, [r1], #2
	subs	r12, #1
.endm

CODE32_FN pal_update16_asm
	push	{r4,lr}
	mov	r12, #256
	sub	r1, #256 * 2
	movs	lr, r2
	bne	2f
1:	ldrb	r2, [r0, #1]
	ldrb	r3, [r0, #2]
	ldrb	r4, [r0], #3
	palette16
	bhi	1b
	pop	{r4,pc}

2:	ldrb	r2, [r0, #1]
	ldrb	r3, [r0, #2]
	ldrb	r4, [r0], #3
	ldrb	r2, [lr, r2]
	ldrb	r3, [lr, r3]
	ldrb	r4, [lr, r4]
	palette16
	bhi	2b
	pop	{r4,pc}

.macro UPDATE_COPY4 r6, r7, r8, r9
	and	\r6, r3, \r9, lsl #1
	and	\r7, r3, \r9, lsr #7
	and	\r8, r3, \r9, lsr #15
	and	\r9, r3, \r9, lsr #23
	ldrh	\r6, [r2, \r6]
	ldrh	\r7, [r2, \r7]
	ldrh	\r8, [r2, \r8]
	ldrh	\r9, [r2, \r9]
	orr	\r6, \r6, \r7, lsl #16
	orr	\r7, \r8, \r9, lsl #16
.endm

CODE32_FN scr_update_1d1_asm
	push	{r4-r11,lr}
	sub	r2, r1, #256 * 2
	mov	r5, #SCREENHEIGHT
	ldr	r3, =0x1fe
1:	mov	r4, #SCREENWIDTH
2:	ldmia	r0!, {r9,r11}
	UPDATE_COPY4 r6, r7, r8, r9
	UPDATE_COPY4 r8, r9, r10, r11
	stmia	r1!, {r6-r9}
	subs	r4, #8
	bhi	2b
	subs	r5, #1
	bhi	1b
	pop	{r4-r11,pc}

CODE32_FN scr_update_1d2_asm
	push	{r4-r11,lr}
	sub	r2, r1, #256 * 4
	mov	r12, #SCREENWIDTH
	mov	r3, #SCREENHEIGHT
	ldr	lr, =0x00400802
	ldr	r5, =0xf81f07e0
	mov	r4, #0x3fc
1:	sub	r3, r3, r12, lsl #16
2:	ldr	r11, [r0, r12]
	ldr	r10, [r0], #4
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
	str	r6, [r1], #4
	adds	r3, #4 << 16
	bmi	2b
	add	r0, r12
	subs	r3, #2
	bhi	1b
	pop	{r4-r11,pc}


.macro READ2X2
	mov	r4, #0x3fc
	ldrh	r8, [r0, r12]
	ldrh	r6, [r0], #2
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
	sub	r2, r1, #256 * 4
	mov	r12, #SCREENWIDTH
	mov	r3, #SCREENHEIGHT
	ldr	lr, =0x00400802
	ldr	r10, =0xf81f07e0
1:	sub	r3, r3, r12, lsl #16
2:
	READ2X2
	SCALE15X_12 r6, r7, lsl, lsr
	str	r11, [r1], #4

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
	str	r11, [r1, #480 * 2 - 4]

	and	r4, r7, lr, lsl #1
	add	r4, r4, r7, lsl #1
	and	r4, r10
	orr	r4, r4, r4, lsl #16
	orr	r5, r5, r4, lsr #16

	SCALE15X_12 r8, r9, lsl, lsr
	str	r11, [r1, #480 * 4 - 4]

	and	r4, r10, r9, lsl #2
	orr	r11, r4, r4, lsl #16

	READ2X2

	and	r4, r10, r8, lsl #2
	orr	r4, r4, r4, lsr #16
	lsl	r4, #16
	orr	r4, r4, r11, lsr #16
	str	r4, [r1, #480 * 4]

	and	r4, r10, r6, lsl #2
	orr	r4, r4, r4, lsr #16
	lsl	r4, #16
	orr	r4, r4, r5, lsr #16
	str	r4, [r1], #4
	lsl	r5, #16
	orr	r5, r5, r11, lsr #16

	SCALE15X_12 r7, r6, lsr, lsl
	str	r11, [r1], #4

	add	r11, r6, r8
	and	r4, r11, lr, lsl #1
	add	r4, r4, r11, lsl #1
	and	r4, r10
	orr	r4, r4, r4, lsr #16
	lsl	r4, #16
	orr	r4, r4, r5, lsr #16
	str	r4, [r1, #480 * 2 - 8]

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
	str	r11, [r1, #480 * 2 - 4]

	SCALE15X_12 r9, r8, lsr, lsl
	str	r11, [r1, #480 * 4 - 4]

	adds	r3, #4 << 16
	bmi	2b
	add	r0, r12
	add	r1, #480 * 4
	subs	r3, #2
	bhi	1b
	pop	{r4-r11,pc}

