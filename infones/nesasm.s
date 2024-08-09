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
1:	ldmia	r0!, {r6-r9}
	UPDATE_COPY4 r6, r7
	UPDATE_COPY4 r8, r9
	stmia	r1!, {r6-r9}
	subs	r2, #8
	bhi	1b
	pop	{r4-r9,pc}

CODE32_FN scr_update_1d2_asm
	push	{r4-r11,lr}
	ldr	r8, =0x7c1f7c1f
	ldr	r9, =0x03e003e0
	ldr	r10, =0x401
	ldr	r11, =0x7c1f
	lsl	r2, #1
1:	sub	r3, r3, r2, lsl #15
2:	ldr	r6, [r0, r2]
	ldr	r4, [r0], #4
	and	r7, r6, r8
	and	r5, r4, r8
	and	r6, r9
	and	r4, r9
	add	r5, r7
	add	r4, r6
	add	r5, r5, r5, lsr #16
	add	r4, r4, r4, lsr #16
	and	r7, r10, r5, lsr #2
	and	r6, r4, #0x20 << 2
	add	r5, r10
	add	r4, #0x20
	add	r5, r7
	add	r4, r4, r6, lsr #2
	and	r5, r11, r5, lsr #2
	and	r4, #0x03e0 << 2
	add	r4, r5, r4, lsr #2
	bic	r5, r4, #0x1f
	add	r4, r5
	strh	r4, [r1], #2
	adds	r3, #2 << 16
	bmi	2b
	add	r0, r2
	subs	r3, #2
	bhi	1b
	pop	{r4-r11,pc}

