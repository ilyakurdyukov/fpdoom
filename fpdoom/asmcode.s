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

CODE32_FN sys_wait_clk
1:	subs	r0, #4	// 1
	bhi	1b	// 3
	bx	lr

CODE32_FN set_mode_sp
	msr	CPSR_c, r0
	mov	sp, r1
	msr	CPSR_c, #0xdf // SYS mode
	bx	lr

CODE32_FN int_vectors
1:	b	1b // reset
1:	b	1b // undefined
1:	b	1b // swi
1:	b	1b // prefetch
1:	b	4f // data
1:	b	1b // reserved
	b	2f // irq
1:	b	1b // fiq
3:	.long 0
5:	.long 0

2:	sub	lr, #4
	push	{r0-r4,r12,lr}
	ldr	lr, 3b
	blx	lr
	ldm	sp!, {r0-r4,r12,pc}^

4:	sub	lr, #8
	push	{r0-r4,r12,lr}
	ldr	lr, 5b
	blx	lr
	ldm	sp!, {r0-r4,r12,pc}^

	.global int_vectors_end
int_vectors_end:

CODE32_FN enable_mmu
	mcr	p15, #0, r0, c2, c0, #0
	mcr	p15, #0, r1, c3, c0, #0 // Domain Access Control Register
	mov	r0, #0
	mcr	p15, #0, r0, c7, c6, #0	// Invalidate DCache
	mrc	p15, #0, r0, c1, c0, #0 // Read Control Register
	orr	r0, #5
	mcr	p15, #0, r0, c1, c0, #0 // Write Control Register
	bx	lr

CODE32_FN invalidate_tlb
	mov	r0, #0
	mcr	p15, #0, r0, c8, c7, #0
	bx	lr

CODE32_FN invalidate_tlb_mva
	mcr	p15, #0, r0, c8, c7, #1
	bx	lr

CODE32_FN clean_icache
	mov	r0, #0
	mcr	p15, #0, r0, c7, c5, #0
	bx	lr

CODE32_FN clean_dcache
	// clean entire dcache using test and clean
	// apsr_nzcv == r15 (Clang doesn't understand r15)
1:	mrc	p15, #0, apsr_nzcv, c7, c10, #3
	bne	1b
	mov	r0, #0
	mcr	p15, #0, r0, c7, c10, #4 // drain write buffer
	bx	lr

CODE32_FN clean_invalidate_dcache
	// clean and invalidate entire dcache
1:	mrc	p15, #0, apsr_nzcv, c7, c14, #3
	bne	1b
	mov	r0, #0
	mcr	p15, #0, r0, c7, c10, #4 // drain write buffer
	bx	lr

CODE32_FN clean_invalidate_dcache_range
	bic	r0, #0x1f
	// clean and invalidate dcache entry (MVA)
1:	mcr	p15, #0, r0, c7, c14, #1
	add	r0, #0x20
	cmp	r0, r1
	blo	1b
	mov	r0, #0
	mcr	p15, #0, r0, c7, c10, #4 // drain write buffer
	bx	lr

CODE32_FN lcd_send_hx1230_asm
	mov	r12, #0x8a000000
	lsl	r1, #24
	ldr	r2, [r12, #0x20 << 3]!
	orr	r1, #1 << 23
	bic	r2, #0x700
1:	orr	r0, r2, r0, lsl #8
	str	r0, [r12]
	orr	r3, r0, #0x200
	lsr	r0, r1, #31
	lsls	r1, #1
	str	r3, [r12]
	bne	1b
	orr	r2, #0x700
	str	r2, [r12]
	bx	lr

CODE32_FN lcd_send_ssd1306_asm
	mov	r12, #0x8a000000
	ldr	r3, [r12, #0x40 << 3]
	lsls	r1, #25
	eor	r0, r3
	ands	r0, #1
	eor	r3, r0
	strne	r3, [r12, #0x40 << 3]
	ldr	r2, [r12, #0x10 << 3]!
	orr	r1, #1 << 24
1:	bic	r2, #0x1c00
	orrcs	r2, #0x400
	str	r2, [r12]
	orr	r2, #0x800
	lsls	r1, #1
	str	r2, [r12]
	bne	1b
	orr	r2, #0x1c00
	str	r2, [r12]
	bx	lr

CODE32_FN lcd_refresh_hx1230_asm
	push	{r4-r10,lr}
	add	r4, r0, #96
	ldr	r8, =0x80808080
	mov	r7, #9
1:	rsb	r1, r7, #0xb9
	and	r10, r1, #8
	mov	r0, #0
	bl	lcd_send_hx1230_asm
	mov	r1, #0x10
	mov	r0, #0
	bl	lcd_send_hx1230_asm
	mov	r1, #0
	mov	r0, #0
	bl	lcd_send_hx1230_asm

	lsr	r10, #1
	sub	r7, #96 << 16
2:	sub	r4, #4
	mov	r9, r4
	add	r5, r4, #96 * 68
	mov	r6, #0x3f
	lsr	r6, r10
3:	ldr	r0, [r9], #96
	ldr	r1, [r5]
	ldr	r2, [r9], #96
	ldr	r3, [r5, #96]
	add	r0, r1
	add	r2, r3
	bic	r1, r0, r8
	bic	r3, r2, r8
	str	r1, [r5], #96
	and	r0, r8
	str	r3, [r5], #96
	and	r2, r8
	orr	r0, r2, r0, lsr #1
	orrs	r6, r0, r6, lsr #2
	bcs	3b
	lsr	r6, r10
	mvn	r1, r6, lsr #24
	mov	r0, #1
	bl	lcd_send_hx1230_asm
	mvn	r1, r6, lsr #16
	mov	r0, #1
	bl	lcd_send_hx1230_asm
	mvn	r1, r6, lsr #8
	mov	r0, #1
	bl	lcd_send_hx1230_asm
	mvn	r1, r6
	mov	r0, #1
	bl	lcd_send_hx1230_asm
	adds	r7, #4 << 16
	bmi	2b
	add	r4, #96 * 9
	subs	r7, #1
	bhi	1b
	pop	{r4-r10,pc}

CODE32_FN lcd_refresh_ssd1306_asm
	push	{r4-r8,lr}
	mov	r4, r0
	ldr	r8, =0x80808080
	mov	r7, #6
1:	rsb	r1, r7, #0xb6
	mov	r0, #0
	bl	lcd_send_ssd1306_asm
	mov	r1, #0
	mov	r0, #0
	bl	lcd_send_ssd1306_asm
	mov	r1, #0x12
	mov	r0, #0
	bl	lcd_send_ssd1306_asm

	sub	r7, #64 << 16
2:	add	r5, r4, #64 * 48
	mov	r6, #0x3f
3:	ldr	r0, [r4], #64
	ldr	r1, [r5]
	ldr	r2, [r4], #64
	ldr	r3, [r5, #64]
	add	r0, r1
	add	r2, r3
	bic	r1, r0, r8
	bic	r3, r2, r8
	str	r1, [r5], #64
	and	r0, r8
	str	r3, [r5], #64
	and	r2, r8
	orr	r0, r2, r0, lsr #1
	orrs	r6, r0, r6, lsr #2
	bcs	3b
	mov	r1, r6
	mov	r0, #1
	bl	lcd_send_ssd1306_asm
	lsr	r1, r6, #8
	mov	r0, #1
	bl	lcd_send_ssd1306_asm
	lsr	r1, r6, #16
	mov	r0, #1
	bl	lcd_send_ssd1306_asm
	lsr	r1, r6, #24
	mov	r0, #1
	bl	lcd_send_ssd1306_asm
	sub	r4, #64 * 8 - 4
	adds	r7, #4 << 16
	bmi	2b
	add	r4, #64 * 7
	subs	r7, #1
	bhi	1b
	pop	{r4-r8,pc}

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

	// psram
	ldr	r5, [r5]
	ands	r5, #1
	movne	r5, #0x30000000
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

CODE32_FN sc6530_init_smc_asm
	push	{r4-r9,lr}
	adr	r6, 1f
	ldmia	r6!, {r0-r3}

	mov	r7, #0x20000000
	mov	r4, #0
	str	r0, [r7]
	str	r4, [r7, #0x04]
	str	r4, [r7, #0x20]
	str	r1, [r7, #0x24]
	str	r2, [r7, #0x28]
	str	r3, [r7, #0x2c]
	mov	r0, #100
	bl	3f

	ldmia	r6!, {r0-r3,r5}

	// psram
	ldr	r5, [r5]
	ands	r5, #1
	movne	r5, #0x30000000
	orr	r5, #0x04000000

	str	r0, [r7]
	str	r3, [r7, #0x04]
	eor	r8, r0, #0x100

	ldr	r0, [r7, #0x24]
	orr	r0, #0x20000
	str	r0, [r7, #0x24]
	strh	r4, [r5, r1]
	bl	2f
	strh	r4, [r5, r2]
	bl	2f
	ldr	r0, [r7, #0x24]
	bic	r0, #0x20000
	str	r0, [r7, #0x24]
	bl	2f

	ldmia	r6!, {r0-r2}
	str	r0, [r7, #0x24]
	str	r1, [r7, #0x28]
	str	r2, [r7, #0x2c]
	str	r8, [r7]
	str	r3, [r7, #0x04]
	mov	r0, #100
	bl	3f
	pop	{r4-r9,pc}

2:	mov	r0, #10
3:	subs	r0, #1
	bne	3b
	bx	lr

1:	.long	0x22220000, 0x00924ff0, 0x0151ffff, 0x00a0744f
	.long	0x222211e0, 0x10323e, 0x20, 0x8080, 0x205000e0
	.long	0x00ac1fff, 0x015115ff, 0x00501015

sc6530_init_smc_asm_end:
	.global sc6530_init_smc_asm_end

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
	.set __aeabi_memcpy, memcpy
	.set __aeabi_memcpy4, memcpy
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
	subs	r2, #4
	orr	r3, r3, r4, lsl r5
	// r6 = bswap32(r3)
	and	r6, lr, r3, ror #24
	and	r3, lr, r3
	orr	r6, r6, r3, ror #8
	lsr	r3, r4, r12
	str	r6, [r0]
	bhi	1b
2:	add	r2, #4
	subs	r2, r2, r12, lsr #3
	and	r6, lr, r3, ror #24
	and	r3, lr, r3
	orr	r6, r6, r3, ror #8
	strhi	r6, [r0]
	pop	{r4-r6,pc}

CODE32_FN usb_recv_asm
	tst	r2, r2
	bxeq	lr
	push	{r4-r6,lr}
	ands	r12, r0, #3
	ldrne	r3, [r0, -r12]!
	lsl	r12, #3
	rsb	r5, r12, #32
	mov	lr, #0xff0000
	lsl	r3, r5
	orr	lr, #0xff
	lsr	r3, r5
1:	ldr	r4, [r1]
	subs	r2, #4
	// r6 = bswap32(r4)
	and	r6, lr, r4, ror #24
	and	r4, lr, r4
	orr	r6, r6, r4, ror #8
	orr	r3, r3, r6, lsl r12
	str	r3, [r0], #4
	lsr	r3, r6, r5
	ldr	r4, [r1]	// 64-bit chunks
	bls	2f
	subs	r2, #4
	// r6 = bswap32(r4)
	and	r6, lr, r4, ror #24
	and	r4, lr, r4
	orr	r6, r6, r4, ror #8
	orr	r3, r3, r6, lsl r12
	str	r3, [r0], #4
	lsr	r3, r6, r5
	bhi	1b
2:	add	r2, #4
	subs	r2, r2, r5, lsr #3
	strhi	r3, [r0]
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

// r1:r0 = r1:r0 * r3:r2
CODE32_FN __aeabi_lmul
	mul	r3, r0, r3
	mla	r3, r2, r1, r3
	umull	r0, r1, r2, r0
	add	r1, r3
	bx	lr

CODE32_FN __clzsi2
	clz	r0, r0
	bx	lr

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

