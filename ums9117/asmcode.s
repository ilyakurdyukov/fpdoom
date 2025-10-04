@ -*- tab-width: 8 -*-
	.arch armv7-a
	.arch_extension idiv
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

CODE32_FN sys_wait_clk
1:	subs	r0, #4	// 1
	bhi	1b	// 3
	bx	lr

CODE32_FN set_mode_sp
	msr	CPSR_c, r0
	mov	sp, r1
	msr	CPSR_c, #0xdf // SYS mode
	bx	lr

CODE32_FN enable_mmu
	mcr	p15, #0, r0, c2, c0, #0 // TTBR0
	mcr	p15, #0, r1, c3, c0, #0 // DACR
	mrc	p15, #0, r0, c1, c0, #0 // SCTLR
	orr	r0, #5
	mcr	p15, #0, r0, c1, c0, #0 // SCTLR
	bx	lr

CODE32_FN clean_icache
	mov	r0, #0
	mcr	p15, #0, r0, c7, c5, #0
	bx	lr

CODE32_FN clean_dcache
	push	{r4-r8,lr}
	dmb
	mrc	p15, #1, r0, c0, c0, #1 // CLIDR
	mov	r3, r0, lsr #23
	ands	r3, #7 << 1 // LoC * 2
	beq	9f
	mov	r8, #0
0:	tst	r0, #6
	beq	8f
	lsr	r0, #3
	mcr	p15, #2, r8, c0, c0, #0 // CCSR
	isb
	mrc	p15, #1, r1, c0, c0, #0 // CCSIDR
	and	r2, r1, #7 // line size
	ubfx	r4, r1, #2, #10 // ways - 1
	ubfx	r7, r1, #13, #15 // sets - 1
	clz	r5, r4
1:	lsl	r6, r7, #4
2:	orr	r1, r8, r4, lsl r5 // level + way
	orr	r1, r1, r6, lsl r2 // set
	mcr	p15, #0, r1, c7, c10, #2 // DCCSW
	subs	r6, #1 << 4
	bpl	2b
	subs	r4, #1
	bpl	1b
8:	add	r8, #2
	cmp	r8, r3
	blo	0b
9:	mov	r8, #0
	mcr	p15, #2, r8, c0, c0, #0 // CCSR
	dsb	st
	isb
	pop	{r4-r8,pc}

CODE32_FN clean_dcache_range
	mrc	p15, #0, r3, c0, c0, #1 // CTR
	mov	r2, #4
	ubfx	r3, r3, #16, #4
	lsl	r2, r3
	sub	r3, r2, #1
	bic	r0, r3
1:	mcr	p15, #0, r0, c7, c10, #1 // clean
	add	r0, r2
	cmp	r0, r1
	blo	1b
	dsb	sy
	bx	lr

CODE32_FN invalidate_dcache_range
	mrc	p15, #0, r3, c0, c0, #1 // CTR
	mov	r2, #4
	ubfx	r3, r3, #16, #4
	lsl	r2, r3
	sub	r3, r2, #1
	tst	r0, r3
	bic	r0, r3
	mcrne	p15, #0, r0, c7, c14, #1 // clean and invalidate
	tst	r1, r3
	bic	r1, r3
	mcrne	p15, #0, r1, c7, c14, #1 // clean and invalidate
1:	mcr	p15, #0, r0, c7, c6, #1 // invalidate
	add	r0, r2
	cmp	r0, r1
	blo	1b
	dsb	sy
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

CODE32_FN __gnu_thumb1_case_si
	add	r12, lr, #2
	and	r12, #-4
	ldr	lr, [r12, r0, lsl #2]
	add	r12, #1
	add	lr, r12
	bx	lr

.if 1
CODE32_FN memcpy
	orr	r3, r0, r1
	mov	r12, r0
	tst	r3, #3
	bne	3f
	subs	r2, #4
	blo	2f
1:	ldr	r3, [r1], #4
	subs	r2, #4
	str	r3, [r12], #4
	bhs	1b
2:	lsls	r2, #31
	ldrhcs	r3, [r1], #2
	ldrbmi	r1, [r1]
	strhcs	r3, [r12], #2
	strbmi	r1, [r12]
	bx	lr

3:	tst	r2, r2
	bxeq	lr
4:	ldrb	r3, [r1], #1
	subs	r2, #1
	strb	r3, [r12], #1
	bhi	4b
	bx	lr

	.global __aeabi_memcpy
	.global __aeabi_memcpy4
	.global __aeabi_memcpy8
	.set __aeabi_memcpy, memcpy
	.set __aeabi_memcpy4, memcpy
	.set __aeabi_memcpy8, memcpy
.endif

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
	strhs	r1, [r3], #4
	bhs	1b
	and	r2, #3
2:	subs	r2, #1
	strbhs	r1, [r3], #1
	bhs	2b
	bx	lr

CODE32_FN __aeabi_memclr
	mov	r2, r1
	mov	r1, #0
	b	memset

	.global __aeabi_memclr4
	.global __aeabi_memclr8
	.set __aeabi_memclr4, __aeabi_memclr
	.set __aeabi_memclr8, __aeabi_memclr
.endif

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
	mrs	r0, CPSR
	bx	lr

CODE32_FN set_cpsr_c
	msr	CPSR_c, r0
	bx	lr

.if 0
// r0, r1 = r0 / r1, r0 % r1
CODE32_FN __aeabi_uidivmod
	udiv	r2, r0, r1
	mls	r1, r2, r1, r0
	mov	r0, r2
	bx	lr

CODE32_FN __aeabi_uidiv
	udiv	r0, r0, r1
	bx	lr
.endif

DIV_CHECK = 1

// r1:r0, r3:r2 = r1:r0 / r3:r2, r1:r0 % r3:r2
CODE32_FN __aeabi_uldivmod
.if DIV_CHECK
	orrs	r12, r2, r3
	bleq	_sig_intdiv
.endif
	push	{r4-r6,lr}
	clz	r6, r2
	cmp	r3, #0
	add	r6, #32
	clzne	r6, r3

	clz	r4, r0
	cmp	r1, 0
	add	r4, #32
	clzne	r4, r1

	subs	r6, r4
	mov	r4, #0
	mov	r5, #0
	bls	9f

	# llsl(r3:r2, r6)
	rsbs	r12, r6, #32
	lsl	r3, r6
	orr	r3, r3, r2, lsr r12
	sub	r12, r6, #32
	movmi	r3, r2, lsl r12
	lsl	r2, r6

1:	subs	r12, r0, r2
	sbcs	lr, r1, r3
	movhs	r0, r12
	movhs	r1, lr
	adcs	r4, r4
	adc	r5, r5
	lsrs	r3, #1
	rrx	r2, r2
	subs	r6, #1
	bne	1b
9:	subs	r2, r0, r2
	sbcs	r3, r1, r3
	movlo	r2, r0
	movlo	r3, r1
	adcs	r0, r4, r4
	adc	r1, r5, r5
	pop	{r4-r6,pc}

// r0, r2 = r0 / r2, r0 % r2
CODE32_FN __aeabi_ldivmod
	push	{r4,lr}
	lsr	r4, r3, #31
	asr	r12, r3, #31
	eors	r4, r4, r1, asr #31
	eor	r2, r12
	eor	r3, r12
	subs	r2, r12
	sbc	r3, r12
	eor	r0, r0, r4, asr #31
	eor	r1, r1, r4, asr #31
	subs	r0, r0, r4, asr #31
	sbc	r1, r1, r4, asr #31
	bl	__aeabi_uldivmod
.if DIV_CHECK
	// r1 < 0 && (a1 ^ a3) >= 0
	bics	r12, r1, r4, lsl #31
	blmi	_sig_intovf
.endif
	// -rem (a1 < 0)
	eor	r2, r2, r4, asr #31
	eor	r3, r3, r4, asr #31
	subs	r2, r2, r4, asr #31
	sbc	r3, r3, r4, asr #31
	// -quo (a1 ^ a3 < 0)
	ror	r4, #1
	eor	r0, r0, r4, asr #31
	eor	r1, r1, r4, asr #31
	subs	r0, r0, r4, asr #31
	sbc	r1, r1, r4, asr #31
	pop	{r4,pc}

.macro SHIFT64 a, b, asr, lsr, lsl
	rsbs	r3, r2, #32
	\lsr	\a, r2
	orr	\a, \a, \b, \lsl r3
	sub	r3, r2, #32
	movmi	\a, \b, \asr r3
	\asr	\b, r2
	bx	lr
.endm

CODE32_FN __aeabi_lasr
	SHIFT64 r0, r1, asr, lsr, lsl

CODE32_FN __aeabi_llsr
	SHIFT64 r0, r1, lsr, lsr, lsl

CODE32_FN __aeabi_llsl
	SHIFT64 r1, r0, lsl, lsl, lsr

