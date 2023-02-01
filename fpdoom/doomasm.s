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

.macro palette16 name
	lsr	r2, 2
	lsr	r3, 3
	lsr	r1, 3
	orr	r3, r3, r2, lsl #5
	orr	r3, r3, r1, lsl #11
	strh	r3, [lr], #2
	subs	r12, #1
.endm

CODE32_FN I_SetPalette16Asm
	push	{r4,lr}
	ldr	r1, =usegamma
	ldr	lr, =colors
	ldr	r1, [r1]
	mov	r12, #256
	cmp	r1, #0
	bne	2f
1:	ldrb	r2, [r0, #1]
	ldrb	r3, [r0, #2]
	ldrb	r1, [r0], #3
	palette16
	bhi	1b
	pop	{r4,pc}

2:	ldr	r4, =gammatable
	add	r4, r4, r1, lsl #8
3:	ldrb	r2, [r0, #1]
	ldrb	r3, [r0, #2]
	ldrb	r1, [r0], #3
	ldrb	r2, [r4, r2]
	ldrb	r3, [r4, r3]
	ldrb	r1, [r4, r1]
	palette16
	bhi	3b
	pop	{r4,pc}

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

CODE32_FN I_FinishUpdateCopyAsm
	push	{r4-r9,lr}
	ldr	r3, =screens
	ldr	r4, =colors
	ldr	r5, =imagedata
	ldr	r3, [r3]
	ldr	r5, [r5]
	mov	r2, #SCREENHEIGHT
	ldr	r6, =0x1fe
1:	mov	r1, #SCREENWIDTH
2:	ldr	r9, [r3], #4
	and	r0, r6, r9, lsl #1
	and	r7, r6, r9, lsr #7
	and	r8, r6, r9, lsr #15
	and	r9, r6, r9, lsr #23
	ldrh	r0, [r4, r0]
	ldrh	r7, [r4, r7]
	ldrh	r8, [r4, r8]
	ldrh	r9, [r4, r9]
	orr	r0, r0, r7, lsl #16
	orr	r8, r8, r9, lsl #16
	stmia	r5!, {r0,r8}
	subs	r1, #4
	bhi	2b
	subs	r2, #1
	bhi	1b
	pop	{r4-r9,pc}

CODE32_FN I_FinishUpdateHalfAsm
	push	{r4-r9,lr}
	ldr	r3, =screens
	ldr	r4, =colors
	ldr	r5, =imagedata
	ldr	r3, [r3]
	ldr	r5, [r5]
	mov	r12, #SCREENWIDTH
	mov	r2, #SCREENHEIGHT
	ldr	lr, =0x00400802
	ldr	r6, =0xf81f07e0
1:	mov	r1, r12
2:	ldrh	r8, [r3, r12]
	ldrh	r0, [r3], #2
	lsr	r9, r8, #8
	lsr	r7, r0, #8
	and	r0, #0xff
	and	r8, #0xff
	ldr	r0, [r4, r0, lsl #2]
	ldr	r7, [r4, r7, lsl #2]
	ldr	r8, [r4, r8, lsl #2]
	ldr	r9, [r4, r9, lsl #2]
	add	r0, r7
	add	r8, r9
	add	r0, r8
	and	r7, lr, r0, lsr #2
	add	r0, lr
	add	r0, r7
	and	r0, r6
	orr	r0, r0, r0, lsr #16
	strh	r0, [r5], #2
	subs	r1, #2
	bhi	2b
	add	r3, r12
	subs	r2, #2
	bhi	1b
	pop	{r4-r9,pc}

.macro READ2X2
	ldrh	r8, [r3, r12]
	ldrh	r0, [r3], #2
	lsr	r9, r8, #8
	lsr	r7, r0, #8
	and	r0, #0xff
	and	r8, #0xff
	ldr	r0, [r4, r0, lsl #2]
	ldr	r7, [r4, r7, lsl #2]
	ldr	r8, [r4, r8, lsl #2]
	ldr	r9, [r4, r9, lsl #2]
.endm

.macro SCALE15X_12 r0, r1, lsl, lsr
	add	r11, \r0, \r1
	and	r6, r11, lr, lsl #1
	add	r6, r6, r11, lsl #1
	and	r11, r10, \r0, lsl #2
	orr	r11, r11, r11, \lsl #16
	and	r6, r10
	\lsr	r11, #16
	orr	r6, r6, r6, \lsr #16
	orr	r11, r11, r6, \lsl #16
.endm

CODE32_FN I_FinishUpdate15Asm
	push	{r4-r11,lr}
	ldr	r3, =screens
	ldr	r4, =colors
	ldr	r5, =imagedata
	ldr	r3, [r3]
	ldr	r5, [r5]
	mov	r12, #SCREENWIDTH
	mov	r2, #SCREENHEIGHT
	ldr	lr, =0x00400802
	ldr	r10, =0xf81f07e0
1:	mov	r1, r12
2:
	READ2X2
	SCALE15X_12 r0, r7, lsl, lsr
	str	r11, [r5], #4

	and	r6, r10, r7, lsl #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r5], #2

	add	r7, r9
	add	r11, r0, r8
	and	r6, r11, lr, lsl #1
	add	r6, r6, r11, lsl #1
	and	r6, r10
	orr	r6, r6, r6, lsl #16
	add	r11, r7
	and	r0, lr, r11, lsr #2
	add	r11, r0
	add	r11, lr
	and	r11, r10
	orr	r11, r11, r11, lsr #16
	lsl	r11, #16
	orr	r11, r11, r6, lsr #16
	add	r0, r5, #480 * 2
	str	r11, [r0, #-6]

	and	r6, r7, lr, lsl #1
	add	r6, r6, r7, lsl #1
	and	r6, r10
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0, #-2]
	add	r0, #480 * 2

	SCALE15X_12 r8, r9, lsl, lsr
	str	r11, [r0, #-6]

	and	r6, r10, r9, lsl #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0, #-2]

	READ2X2

	and	r6, r10, r0, lsl #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r5], #2

	SCALE15X_12 r7, r0, lsr, lsl
	str	r11, [r5], #4

	add	r11, r0, r8
	and	r6, r11, lr, lsl #1
	add	r6, r6, r11, lsl #1
	and	r6, r10
	orr	r6, r6, r6, lsr #16
	add	r0, r5, #480 * 2
	strh	r6, [r0, #-6]

	add	r7, r9
	add	r11, r7
	and	r6, lr, r11, lsr #2
	add	r11, r6
	add	r11, lr
	and	r11, r10
	orr	r11, r11, r11, lsl #16
	lsr	r11, #16
	and	r6, r7, lr, lsl #1
	add	r6, r6, r7, lsl #1
	and	r6, r10
	orr	r6, r6, r6, lsr #16
	orr	r11, r11, r6, lsl #16
	str	r11, [r0, #-4]
	add	r0, #480 * 2

	and	r6, r10, r8, lsl #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0, #-6]

	SCALE15X_12 r9, r8, lsr, lsl
	str	r11, [r0, #-4]

	subs	r1, #4
	bhi	2b
	add	r3, r12
	mov	r5, r0
	subs	r2, #2
	bhi	1b
	pop	{r4-r11,pc}

