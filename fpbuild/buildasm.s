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

CODE32_FN krecipasm
	movs	r2, r0
	rsbmi	r2, r0, #0
	clz	r3, r2	// 0 -> 32
	lsl	r2, r3
	ldr	r1, 1f
	lsr	r2, #20
	eor	r3, #31
	ldrne	r1, [r1, r2, lsl #2]
	asr	r0, #31
	eorne	r0, r0, r1, asr r3
	bx	lr

1:	.long	reciptable-2048*4

.macro CODE32_LOC name
	.p2align 2
	.code 32
	.type \name, %function
	.global \name
\name:
.endm
.macro DATA_DEF name, type=long, val=0
	.global \name
\name:	.\type	\val
.endm

	.section .text.buildasm, "ax", %progbits

CODE32_LOC hlineasm4
	cmp	r0, #0
	bxmi	lr
	push	{r4-r10,lr}
	cmp	r1, #0
	ldr	r4, [sp, #8*4]
	ldr	r5, [sp, #8*4+4]
	ldr	r6, glogx
	ldr	r7, glogy
	ldr	r8, gbuf
	ldr	lr, ghlinepal
	ldrne	r9, gbxinc
	ldrne	r10, gbyinc
	ldreq	r9, asm1
	ldreq	r10, asm2
	rsb	r6, #32
	rsb	r1, r7, #32
	add	r2, lr
1:	lsr	lr, r4, r6
	sub	r4, r9
	lsl	lr, r7
	add	lr, lr, r3, lsr r1
	ldrb	lr, [r8, lr]
	sub	r3, r10
	ldrb	lr, [r2, lr]
	subs	r0, #1
	strb	lr, [r5], #-1
	bpl	1b
	pop	{r4-r10,pc}


CODE32_LOC slopevlin
	cmp	r3, #0
	bxle	lr
	push	{r4-r11,lr}
	ldr	r1, asm3
	ldr	r4, glogx
	ldr	r5, glogy
	ldr	r6, gbuf
	ldr	r9, asm1
	ldr	r10, globalx3
	ldr	r11, globaly3
	ldr	r12, gpinc
	rsb	r4, #32

1:	// lr = krecip(r1 >> 6)
	asrs	lr, r1, #6
	rsbmi	lr, #0
	clz	r7, lr
	lsl	lr, r7
	ldr	r8, 2f
	lsr	lr, #20
	eor	r7, #31
	ldrne	r8, [r8, lr, lsl #2]	// 2 cycles
	asr	lr, r1, #31
	eorne	lr, lr, r8, asr r7

	ldr	r7, [sp, #9*4]
	ldr	r8, [sp, #9*4+4]
	mla	r7, lr, r10, r7	// 2 cycles
	mla	r8, lr, r11, r8	// 2 cycles
	rsb	lr, r5, #32
	lsr	r7, r4
	lsr	r8, lr
	add	lr, r8, r7, lsl r5

	ldrb	lr, [r6, lr]
	ldr	r7, [r2], #-4
	add	r1, r1, r9, asr #3
	ldrb	lr, [r7, lr]
	subs	r3, #1
	strb	lr, [r0], r12
	bne	1b
	pop	{r4-r11,pc}

2:	.long	reciptable-2048*4


.macro vline_head r6, n
	cmp	r2, #0
	bxmi	lr
	push	{r4-\r6,lr}
	ldr	r4, [sp, #\n*4]
	ldr	r5, [sp, #\n*4+4]
	ldr	r6, glogy
	ldr	r12, bpl
.endm

CODE32_LOC vlineasm1
	vline_head r6, 4
1:	lsr	lr, r3, r6
	ldrb	lr, [r4, lr]
	add	r3, r0
	ldrb	lr, [r1, lr]
	subs	r2, #1
	strb	lr, [r5], r12
	bpl	1b
	pop	{r4-r6,pc}


CODE32_LOC mvlineasm1
	vline_head r6, 4
1:	lsr	lr, r3, r6
	ldrb	lr, [r4, lr]
	add	r3, r0
	cmp	lr, #255
	ldrbne	lr, [r1, lr]
	addeq	r5, r12
	strbne	lr, [r5], r12
	subs	r2, #1
	bpl	1b
	pop	{r4-r6,pc}


CODE32_LOC tvlineasm1
	vline_head r8, 6
	ldr	lr, transmode
	ldr	r7, gtrans
	cmp	lr, #0
	beq	11f
	b	10f

.macro tvlineasm1_part r8, lr, label
2:	ldrb	lr, [r1, lr]
	ldrb	r8, [r5]
	orr	lr, \r8, \lr, lsl #8
	ldrb	lr, [r7, lr]
	subs	r2, #1
	strb	lr, [r5], r12
	bmi	9f
\label:
1:	lsr	lr, r3, r6
	ldrb	lr, [r4, lr]
	add	r3, r0
	cmp	lr, #255
	bne	2b
	subs	r2, #1
	add	r5, r12
	bpl	1b
9:	pop	{r4-r8,pc}
.endm
	tvlineasm1_part r8, lr, 10
	tvlineasm1_part lr, r8, 11


.macro hline_head r9, n
	subs	r2, #65536
	bxlt	lr
	push	{r4-\r9,lr}
	ldr	r4, [sp, #\n*4]
	ldr	r5, [sp, #\n*4+4]
	ldr	r6, glogx
	ldr	r7, glogy
	ldr	r8, asm1
	ldr	r9, asm2
	ldr	r12, asm3
	rsb	r6, #32
	rsb	r3, r7, #32
.endm

CODE32_LOC mhline
	hline_head r9, 7
1:	lsr	lr, r1, r6
	lsl	lr, r7
	add	lr, lr, r4, lsr r3
	ldrb	lr, [r0, lr]
	add	r1, r8
	cmp	lr, #255
	ldrbne	lr, [r12, lr]
	add	r4, r9
	strbne	lr, [r5]
	subs	r2, #65536
	add	r5, #1
	bhs	1b
	pop	{r4-r9,pc}


CODE32_LOC thline
	hline_head r11, 9
	ldr	lr, transmode
	ldr	r10, gtrans
	cmp	lr, #0
	beq	11f
	b	10f

.macro thline_part r11, lr, label
2:	ldrb	lr, [r12, lr]
	ldrb	r11, [r5]
	add	r4, r9
	orr	lr, \r11, \lr, lsl #8
	ldrb	lr, [r10, lr]
	subs	r2, #65536
	strb	lr, [r5], #1
	blo	9f
\label:
1:	lsr	lr, r1, r6
	lsl	lr, r7
	add	lr, lr, r4, lsr r3
	ldrb	lr, [r0, lr]
	add	r1, r8
	cmp	lr, #255
	bne	2b
	subs	r2, #65536
	add	r5, #1
	add	r4, r9
	bhs	1b
9:	pop	{r4-r11,pc}
.endm
	thline_part r11, lr, 10
	thline_part lr, r11, 11


.macro spritevline_head r8, n
	subs	r2, #2
	bxlt	lr
	push	{r4-\r8,lr}
	ldr	r4, [sp, #\n*4]
	ldr	r5, glogy
	ldr	r6, gpal
	ldr	r7, gbxinc
	ldr	r8, gbyinc
	ldr	r12, bpl
.endm

CODE32_LOC spritevline
	spritevline_head r8, 6
1:	asr	lr, r0, #16
	mul	lr, r5, lr	// 2 cycles
	add	r0, r7
	add	lr, lr, r1, asr #16
	ldrb	lr, [r3, lr]
	add	r1, r8
	ldrb	lr, [r6, lr]
	subs	r2, #1
	strb	lr, [r4], r12
	bpl	1b
	pop	{r4-r8,pc}


CODE32_LOC mspritevline
	spritevline_head r8, 6
1:	asr	lr, r0, #16
	mul	lr, r5, lr	// 2 cycles
	add	r0, r7
	add	lr, lr, r1, asr #16
	ldrb	lr, [r3, lr]
	add	r1, r8
	cmp	lr, #255
	ldrbne	lr, [r6, lr]
	addeq	r4, r12
	strbne	lr, [r4], r12
	subs	r2, #1
	bpl	1b
	pop	{r4-r8,pc}


CODE32_LOC tspritevline
	spritevline_head r10, 8
	ldr	lr, transmode
	ldr	r9, gtrans
	cmp	lr, #0
	beq	11f
	b	10f

.macro tspritevline_part r10, lr, label
2:	ldrb	lr, [r6, lr]
	ldrb	r10, [r4]
	// extra cycle
	orr	lr, \r10, \lr, lsl #8
	ldrb	lr, [r9, lr]
	subs	r2, #1
	strb	lr, [r4], r12
	bmi	9f
\label:
1:	asr	lr, r0, #16
	mul	lr, r5, lr	// 2 cycles
	add	r0, r7
	add	lr, lr, r1, asr #16
	ldrb	lr, [r3, lr]
	add	r1, r8
	cmp	lr, #255
	bne	2b
	subs	r2, #1
	add	r4, r12
	bpl	1b
9:	pop	{r4-r10,pc}
.endm
	tspritevline_part r10, lr, 10
	tspritevline_part lr, r10, 11


CODE32_LOC drawslab
	cmp	r2, #0
	bxle	lr
	cmp	r0, #0
	bxle	lr
	push	{r4-r7,lr}
	ldr	r4, [sp, #5*4]
	ldr	r5, [sp, #5*4+4]
	ldr	r12, bpl
	ldr	r6, gpal
	sub	r12, r0
1:	ldrb	lr, [r4, r1, asr #16]
	add	r1, r3
	ldrb	lr, [r6, lr]
	mov	r7, r0
2:	subs	r7, #1
	strb	lr, [r5], #1
	bne	2b
	subs	r2, #1
	add	r5, r12
	bne	1b
	pop	{r4-r7,pc}

DATA_DEF asm1
DATA_DEF asm2
DATA_DEF asm3
DATA_DEF globalx3
DATA_DEF globaly3

	.global buildasm_data
buildasm_data:
bpl:	.long 0
transmode:	.long 0
glogx:	.long 0
glogy:	.long 0
gbxinc:	.long 0
gbyinc:	.long 0
gpinc:	.long 0
gbuf:	.long 0
gpal:	.long 0
ghlinepal:	.long 0
gtrans:	.long 0

.macro palette16
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
1:	ldrb	r2, [r0, #1]
	ldrb	r3, [r0, #2]
	ldrb	r4, [r0], #4
	palette16
	bhi	1b
	pop	{r4,pc}

.macro palette32
	lsl	r2, 1
	orr	r2, r2, r3, lsl #11
	orr	r2, r2, r4, lsl #22
	str	r2, [r1], #4
	subs	r12, #1
.endm

CODE32_FN pal_update32_asm
	push	{r4,lr}
	mov	r12, #256
	sub	r1, #256 * 4
1:	ldrb	r2, [r0, #1]
	ldrb	r3, [r0, #2]
	ldrb	r4, [r0], #4
	palette32
	bhi	1b
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
	ldr	r12, scr_update_data
	ldr	r5, scr_update_data + 4
	ldr	r3, =0x1fe
1:	sub	r5, r5, r12, lsl #16
2:	ldmia	r0!, {r9,r11}
	UPDATE_COPY4 r6, r7, r8, r9
	UPDATE_COPY4 r8, r9, r10, r11
	stmia	r1!, {r6-r9}
	adds	r5, #8 << 16
	bmi	2b
	subs	r5, #1
	bhi	1b
	pop	{r4-r11,pc}

	.global scr_update_data
scr_update_data:
	.long	0 // w
	.long	0 // h

	.type scr_update_1d2_asm, %function
	.global scr_update_1d2_asm
scr_update_1d2_asm:
	push	{r4-r11,lr}
	sub	r2, r1, #256 * 4
	ldr	r12, scr_update_data
	ldr	r3, scr_update_data + 4
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


