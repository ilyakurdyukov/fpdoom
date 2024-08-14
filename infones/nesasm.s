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
	ldr	r8, =0x03e07c1f
	ldr	r9, =0x00200401
	lsl	r2, #1
1:	sub	r3, r3, r2, lsl #15
2:	ldr	r6, [r0, r2]
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
	adds	r3, #2 << 16
	bmi	2b
	add	r0, r2
	subs	r3, #2
	bhi	1b
	pop	{r4-r11,pc}

