@ -*- tab-width: 8 -*-
	.arch armv5te
	.syntax unified

.macro CODE16_FN name
	.section .text.\name, "ax", %progbits
	.p2align 1
	.code 16
	.type \name, %function
	.global \name
	.thumb_func
\name:
.endm

.macro CODE32_FN name
	.section .text.\name, "ax", %progbits
	.p2align 2
	.code 32
	.type \name, %function
	.global \name
\name:
.endm

CODE32_FN enable_mmu
	mcr	p15, #0, r0, c2, c0, #0
	mcr	p15, #0, r1, c3, c0, #0 // Domain Access Control Register
	mov	r0, #0
	mcr	p15, #0, r0, c7, c6, #0	// Invalidate DCache
	mrc	p15, #0, r0, c1, c0, #0 // Read Control Register
	orr	r0, #5
	mcr	p15, #0, r0, c1, c0, #0 // Write Control Register
	bx	lr

CODE32_FN clean_dcache
	// clean entire dcache using test and clean
	// apsr_nzcv == r15
1:	mrc	p15, #0, apsr_nzcv, c7, c10, #3
	bne	1b
	mov	r0, #0
	mcr	p15, #0, r0, c7, c10, #4 // drain write buffer
	bx	lr

CODE32_FN __gnu_thumb1_case_uqi
	bic	r12, lr, #1
	ldrb	r12, [r12, r0]
	add	lr, lr, r12, lsl #1
	bx	lr

CODE32_FN __gnu_thumb1_case_sqi
	bic	r12, lr, #1
	ldrsb	r12, [r12, r0]
	add	lr, lr, r12, lsl #1
	bx	lr

CODE32_FN __gnu_thumb1_case_uhi
	add	r12, r0, lr, lsr #1
	ldrh	r12, [r12, r12]
	add	lr, lr, r12, lsl #1
	bx	lr

CODE32_FN __gnu_thumb1_case_shi
	add	r12, r0, lr, lsr #1
	ldrsh	r12, [r12, r12]
	add	lr, lr, r12, lsl #1
	bx	lr

CODE32_FN sc6531da_init_smc_asm
	push	{r4-r7,lr}
	// sc6531da_init_smc_1
	adr	r6, 1f
	ldmia	r6!, {r0-r3,r5}

	mov	r7, #0x20000000
	mov	r4, #0
	str	r0, [r7]
	mov	r0, #0x10
	str	r4, [r7, #0x20]
	str	r4, [r7, #0x24]
	str	r0, [r7, #4]
	str	r1, [r7, #0x50]
	str	r2, [r7, #0x54]
	str	r3, [r7, #0x58]
	str	r4, [r7, #0x5c]

	// sc6531da_get_psram
	ldr	r5, [r5]
	and	r5, #1
	orr	r5, r5, r5, lsl #1
	lsl	r5, #28
	orr	r5, #0x04000000

	ldmia	r6!, {r1-r3}

	ldr	r0, [r7, #0x50]
	orr	r0, #0x20000
	str	r0, [r7, #0x50]
	strh	r4, [r5, r1]
	bl	2f
	strh	r4, [r5, r2]
	bl	2f
	ldr	r0, [r7, #0x50]
	bic	r0, #0x20000
	str	r0, [r7, #0x50]
	bl	2f

	mov	r1, #0x24
4:	ldr	r0, [r7, r1]
	orr	r0, r3
	str	r0, [r7, r1]
	add	r1, #4
	cmp	r1, #0x38
	bls	4b

	// smc08, smc00, smc20, smc24, smc04
	ldmia	r6!, {r0-r3,r5}
	str	r0, [r7, #8]
	ldr	r0, [r7, #8]
	bic	r0, #0x100
	str	r0, [r7, #8]
	str	r1, [r7]
	ldr	r0, [r7, #0x20]
	orr	r2, r0
	str	r2, [r7, #0x20]
	ldr	r0, [r7, #0x24]
	orr	r3, r0
	str	r3, [r7, #0x24]
	str	r5, [r7, #4]

	ldmia	r6!, {r0-r3}
	str	r0, [r7, #0x50]
	str	r1, [r7, #0x54]
	str	r2, [r7, #0x58]
	str	r3, [r7, #0x5c]

	ldr	r0, [r7]
	bic	r0, #0x100
	str	r0, [r7]
	mov	r0, #100
	bl	3f

	pop	{r4-r7,pc}

2:	mov	r0, #10
3:	subs	r0, #1
	bne	3b
	bx	lr

1:	.long	0x11110000, 0x00904ff0, 0x0151ffff, 0x00a0744f
	.long	0x205000e0

sc6531da_init_smc_asm_end:
	.global sc6531da_init_smc_asm_end

CODE32_FN abs
	tst	r0, r0
	rsbmi	r0, #0
	bx	lr

CODE32_FN memcpy
	mov	r12, r0
	orr	r3, r0, r1
	tst	r3, #3
	bne	2f
1:	subs	r2, #4
	ldrhs	r3, [r1],#4
	strhs	r3, [r0],#4
	bhi	1b
	and	r2, #3
2:	subs	r2, #1
	ldrbhs	r3, [r1],#1
	strbhs	r3, [r0],#1
	bhi	2b
	mov	r0, r12
	bx	lr

	.global __aeabi_memcpy
	.global __aeabi_memcpy4
	.set __aeabi_memcpy, memcpy
	.set __aeabi_memcpy4, memcpy

.if 0
CODE32_FN memmove
	subs	r3, r0, r1
	cmp	r3, r2
	bhs	memcpy	// dst - src >= len
	orr	r3, r0, r1
	tst	r3, #3
	bne	3f
	lsls	r3, r2, #31
	bic	r3, r2, #1
	bic	r2, #3
	ldrbmi	r3, [r1,r3]
	strbmi	r3, [r0,r3]
	ldrhcs	r3, [r1,r2] 
	strhcs	r3, [r0,r2]
2:	subs	r2, #4
	ldrhs	r3, [r1,r2]
	strhs	r3, [r0,r2]
	bhs	2b
	bx	lr

3:	subs	r2, #1
	ldrbhs	r3, [r1,r2]
	strbhs	r3, [r0,r2]
	bhs	3b
	bx	lr
.endif

.if 1
CODE32_FN memset
	mov	r3, r0
	tst	r0, #3
	bne	2f
	lsl	r1, #24
	orr	r1, r1, r1, lsr #8
	orr	r1, r1, r1, lsr #16
1:	subs	r2, #4
	strhs	r1, [r0], #4
	bhs	1b
	and	r2, #3
2:	subs	r2, #1
	strbhs	r1, [r0], #1
	bhs	2b
	mov	r0, r3
	bx	lr
.endif

CODE32_FN usb_send_asm
	push	{r4-r6,lr}
	tst	r2, r2
	and	r12, r1, #3
	ldrne	r3, [r1, -r12]!
	add	r2, r12
	lsl	r12, #3
	lsr	r3, r12
	mov	lr, #0xff0000
	subs	r2, #4
	orr	lr, #0xff
	bls	2f
	rsb	r5, r12, #32
1:	ldr	r4, [r1, #4]!
	orr	r3, r3, r4, lsl r5
	and	r6, lr, r3, ror #24
	and	r3, lr, r3
	orr	r6, r6, r3, ror #8
	lsr	r3, r4, r12
	str	r6, [r0]
	subs	r2, #4
	bhi	1b
2:	add	r2, #4
	lsr	r12, #3
	subs	r2, r2, r12
	and	r6, lr, r3, ror #24
	and	r3, lr, r3
	orr	r6, r6, r3, ror #8
	strhi	r6, [r0]
	mov	r0, r2
	pop	{r4-r6,pc}

// sum, ptr, size
CODE32_FN fastchk16
	tst	r2, r2
	and	r12, r1, #3
	ldrne	r3, [r1, -r12]!
	add	r2, r12
	lsl	r12, #3
	lsr	r3, r12
	subs	r2, #4
	lsl	r3, r12
	ror	r0, r12
	bls	2f
1:	adds	r0, r3
	ldr	r3, [r1, #4]!
	adc	r0, #0
	subs	r2, #4
	bhi	1b
2:	rsb	r2, #0
	lsl	r2, #3
	lsl	r3, r2
	adds	r0, r0, r3, lsr r2
	adc	r0, #0
	ror	r0, r12
	mov	r1, #0
	adds	r0, r0, r0, lsl #16
	// sum = (sum >> 16) + (sum & 0xffff)
	adc	r0, r1, r0, lsr #16
	// sum += sum >> 16
	add	r0, r0, r0, lsr #16
	// ~sum & 0xffff
	mov	r0, r0, lsl #16
	lsr	r0, #16
	bx	lr

CODE32_FN get_cpsr
	mrs	r0, cpsr
	bx	lr

CODE32_FN set_cpsr_c
	msr	cpsr_c, r0
	bx	lr

DIV_CHECK = 1
DIV_UNROLL = 1

// r0, r1 = r0 / r1, r0 % r1
CODE32_FN __aeabi_uidivmod
.if DIV_CHECK
	tst	r1, r1
	bleq	_sig_intdiv
.endif
	clz	r3, r1
	clz	r2, r0
	subs	r3, r2
.if DIV_UNROLL == 1
	mov	r2, #0x80000000
.else
	mov	r2, #0
.endif
	bls	9f
.if DIV_UNROLL == 1
	subs	r3, #1	// !mi
	bic	r3, #3
	lsl	r1, r3
	asr	r2, r3
1:	lsrmi	r1, #4
	rsbs	r12, r1, r0, lsr #4
	subhs	r0, r0, r1, lsl #4
	adc	r2, r2
	rsbs	r12, r1, r0, lsr #3
	subhs	r0, r0, r1, lsl #3
	adc	r2, r2
	rsbs	r12, r1, r0, lsr #2
	subhs	r0, r0, r1, lsl #2
	adc	r2, r2
	rsbs	r12, r1, r0, lsr #1
	subhs	r0, r0, r1, lsl #1
	adcs	r2, r2
	bcs	1b
.elseif DIV_UNROLL == 2
	adr	r12, 9f
	add	r3, r3, r3, lsl #1
	sub	pc, r12, r3, lsl #2
.set i, 31
.rept 31
	rsbs	r12, r1, r0, lsr #i
	subhs	r0, r0, r1, lsl #i
	adc	r2, r2
.set i, i - 1
.endr
.else // !DIV_UNROLL
1:	rsbs	r12, r1, r0, lsr r3
	subhs	r0, r0, r1, lsl r3
	adc	r2, r2
	subs	r3, #1
	bne	1b
.endif
9:	subs	r1, r0, r1
	movlo	r1, r0
	adc	r0, r2, r2
	bx	lr

// r0, r1 = r0 / r1, r0 % r1
CODE32_FN __aeabi_idivmod
	push	{r4,lr}
	lsrs	r4, r1, #31
	rsbne	r1, #0
	eors	r4, r4, r0, asr #31
	rsbmi	r0, #0
	bl	__aeabi_uidivmod
.if DIV_CHECK
	// r0 < 0 && (a0 ^ a1) >= 0
	bics	r2, r0, r4, lsl #31
	blmi	_sig_intovf
.endif
	asrs	r4, #1
	rsbcs	r0, #0	// -quo (a0 ^ a1 < 0)
	rsbmi	r1, #0	// -rem (a0 < 0)
	pop	{r4,pc}

	.global __aeabi_uidiv
	.global __aeabi_idiv
	.set __aeabi_uidiv, __aeabi_uidivmod
	.set __aeabi_idiv, __aeabi_idivmod

// r1:r0 = r1:r0 * r3:r2
CODE32_FN __aeabi_lmul
	mul	r3, r0, r3
	mla	r3, r2, r1, r3
	umull	r0, r1, r2, r0
	add	r1, r3
	bx	lr

