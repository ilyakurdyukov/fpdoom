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

NO_SA1 = 1

CODE32_FN S9xMainLoop_asm
	push	{r4-r10, lr}
	ldr	r4, =CPU
	mov	r7, #0
	ldr	r5, =ICPU
	mov	r10, #1
.if !NO_SA1
	ldr	r6, =SA1
.endif
	ldr	r8, =Settings
	ldr	r9, =Registers
	ldr	r0, [r4]
	cmp	r0, #0
	beq	15f

10:	tst	r0, #128 // NMI_FLAG
	beq	11f
	ldr	r1, [r4, #72] // NMICycleCount
	bic	r0, #128
	subs	r1, #1
	str	r1, [r4, #72]
	bne	11f
	ldrb	r1, [r4, #7] // WaitingForInterrupt
	str	r0, [r4]
	cmp	r1, #0
	ldrne	r1, [r4, #12] // PC
	strbne	r7, [r4, #7]
	addne	r1, #1
	strne	r1, [r4, #12]
	bl	_Z13S9xOpcode_NMIv
	ldr	r0, [r4]

11:	tst	r0, #2048 // IRQ_PENDING_FLAG
	beq	14f
	ldr	r1, [r4, #76] // IRQCycleCount
	subs	r1, #1
	blo	12f
	str	r1, [r4, #76]
	bne	14f
	ldrb	r1, [r9, #2] // PL
	tst	r1, #4 // IRQ
	strne	r10, [r4, #76]
	b	14f

12:	ldrb	r1, [r4, #7] // WaitingForInterrupt
	cmp	r1, #0
	ldrne	r1, [r4, #12] // PC
	strbne	r7, [r4, #7]
	addne	r1, #1
	strne	r1, [r4, #12]
	ldrb	r1, [r4, #6] // IRQActive
	ldrb	r3, [r8, #16] // DisableIRQ
	cmp	r1, #0
	beq	13f
	ldrb	r1, [r9, #2] // PL
	cmp	r3, #0
	bne	13f
	tst	r1, #4 // IRQ
	bne	14f
	bl	_Z13S9xOpcode_IRQv
	ldr	r0, [r4]
	b	14f

13:	bic	r0, #2048 // IRQ_PENDING_FLAG
	str	r0, [r4]
14:	tst	r0, #16 // SCAN_KEYS_FLAG
	bne	9f

15:	ldr	r0, [r4, #12] // PC
	ldr	r1, [r4, #32] // Cycles
	ldr	r2, [r4, #44] // MemSpeed
	str	r0, [r4, #20] // PCAtOpcodeStart
	add	r1, r2
	str	r1, [r4, #32]
	ldrb	r3, [r0], #1
	ldr	r1, [r5, #4] // S9xOpcodes
	str	r0, [r4, #12] // PC
	ldr	r3, [r1, r3, lsl #2]
	blx	r3
	bl	S9xUpdateAPUTimer
.if !NO_SA1
	ldrb	r0, [r6, #24] // Executing
	cmp	r0, #0
	beq	16f
	bl	S9xSA1MainLoop
16:
.endif
	ldr	r0, [r4, #32]
	ldr	r1, [r4, #36]
	cmp	r0, r1
	blt	17f
	bl	S9xDoHBlankProcessing
17:	ldr	r0, [r4]
	cmp	r0, #0
	beq	15b
	b	10b
9:	pop	{r4-r10,pc}

CODE32_FN S9xUpdateAPUTimer
1:	ldr	r3, =CPU
	//mov	r1, #10000 - 8192
	ldr	r0, =IAPU
	ldr	r3, [r3, #32] // Cycles
	//orr	r1, #8192
	//mul	r2, r3, r1
	ldr	r1, [r0, #48] // NextAPUTimerPos
	// (Cycles << 14) >= NextAPUTimerPos
	cmp	r1, r3, lsl #14
	bxgt	lr
	ldr	r2, =5498202 // old 3355836
	add	r1, r2
	str	r1, [r0, #48]
	ldr	r1, =APU

	ldrb	r2, [r1, #218] // TimerEnabled[2]
	cmp	r2, #0
	beq	2f
	ldrh	r3, [r1, #208] // Timer[2]
	ldrh	r2, [r1, #214] // TimerTarget[2]
	add	r3, #1
	strh	r3, [r1, #208]
	lsl	r3, #16
	cmp	r3, r2, lsl #16
	blo	2f
	mov	r2, #0
	strh	r2, [r1, #208]
	ldr	r2, [r0, #4]	// RAM
	ldrb	r3, [r2, #255]
	add	r3, #1
	and	r3, #15
	strb	r3, [r2, #255]

2:	ldr	r2, [r0, #52] // APUTimerCounter
	add	r2, #1
	ands	r2, #7
	str	r2, [r0, #52]
	bne	1b

	ldrb	r2, [r1, #216] // TimerEnabled[0]
	cmp	r2, #0
	beq	3f
	ldrh	r2, [r1, #204] // Timer[0]
	ldrh	r3, [r1, #210] // TimerTarget[0]
	add	r2, #1
	strh	r2, [r1, #204]
	lsl	r2, #16
	cmp	r2, r3, lsl #16
	blo	3f
	mov	r2, #0
	strh	r2, [r1, #204]
	ldr	r2, [r0, #4]	// RAM
	ldrb	r3, [r2, #253]
	add	r3, #1
	and	r3, #15
	strb	r3, [r2, #253]

3:	ldrb	r2, [r1, #217] // TimerEnabled[1]
	cmp	r2, #0
	beq	1b
	ldrh	r2, [r1, #206] // Timer[1]
	ldrh	r3, [r1, #212] // TimerTarget[1]
	add	r2, #1
	strh	r2, [r1, #206]
	lsl	r2, #16
	cmp	r2, r3, lsl #16
	blo	1b
	ldr	r0, [r0, #4]	// RAM
	mov	r2, #0
	strh	r2, [r1, #206]
	ldrb	r1, [r0, #254]
	add	r1, #1
	and	r1, #15
	strb	r1, [r0, #254]
	b	1b


.macro ConvertTile_PART r5, i, j
.if (\i & 1) == 0
	ldrb	r5, [r1, #\i - 2]
	ldrb	r2, [r1, #\i - 1]
.endif
	orrs	r5, \r5, \r5, lsl #9
	beq	1f
	orr	r5, r5, r5, lsl #18
	and	r4, r12, r5, lsr #7
	and	r5, r12, r5, lsr #3
	orr	r6, r6, r4, lsl #\j
	orr	r7, r7, r5, lsl #\j
1:
.endm

CODE32_FN ConvertTile_asm
	push	{r4-r7,lr}
	ldr	lr, =BG
	ldr	r4, =Memory
	ldr	lr, [lr, #4] // BitShift
	mov	r3, #0
	cmp	lr, #0
	beq	9f
	ldr	r4, [r4, #8] // VRAM
	orr	lr, #7 << 29
	add	r1, r4
	ldr	r12, =0x1010101
10:	ldrb	r5, [r1], #2
	ldrb	r2, [r1, #-1]
	orr	r5, r5, r5, lsl #9
	orr	r5, r5, r5, lsl #18
	and	r6, r12, r5, lsr #7
	and	r7, r12, r5, lsr #3
	ConvertTile_PART r2, 1, 1
	tst	lr, #2
	bne	2f
	ConvertTile_PART r5, 16, 2
	ConvertTile_PART r2, 17, 3
	tst	lr, #4
	bne	2f
	ConvertTile_PART r5, 32, 4
	ConvertTile_PART r2, 33, 5
	ConvertTile_PART r5, 48, 6
	ConvertTile_PART r2, 49, 7
2:	orr	r4, r6, r7
	stmia	r0!, {r6,r7}
	orr	r3, r4
	subs	lr, #1 << 29
	bhs	10b
9:	mov	r0, #1 // TRUE
	cmp	r3, #0
	moveq	r0, #2 // BLANK_TILE
	pop	{r4-r7,pc}


.macro TILE_CLIP_PREAMBLE
	tst	r0, 0x4000 // H_FLIP
	add	r1, r2, r3
	rsbne	r2, r1, #8
	lsl	r2, #3
	add	r3, r2, r3, lsl #3
	mvn	r11, #0
	lsl	r10, r11, r2
	subs	r2, #32
	rsb	r3, #64
	lslhi	r11, r2
	subs	r2, r3, #32
	and	r11, r11, r11, lsr r3
	andhi	r10, r10, r10, lsr r2
.endm
.macro TILE_PREAMBLE r11
	mov	r6, r0
	tst	r0, #256
	ldr	r8, =BG
	lsl	r0, #22
	lsr	r0, #22
	ldr	r2, [r8, #8] // TileShift
	ldr	r1, [r8, #12] // TileAddress
	ldrne	r3, [r8, #16] // NameSelect
	add	r1, r1, r0, lsl r2
	addne	r1, r3
	// TileAddr &= 0xffff
	lsl	r1, #16
	lsr	r1, #16

	ldr	r7, [r8, #40] // Buffered
	ldr	r5, [r8, #36] // Buffer
	lsr	r3, r1, r2
	ldrb	r0, [r7, r3]!
	add	r5, r5, r3, lsl #6
	cmp	r0, #0
	bne	1f
	mov	r0, r5
	bl	ConvertTile_asm
	strb	r0, [r7]
1:	cmp	r0, #2
	popeq	{r1-\r11,pc}
	ldr	r1, [r8, #32] // PaletteMask
	ldrb	r0, [r8, #44] // DirectColourMode
	ldr	r7, =IPPU
	and	r3, r1, r6, lsr #10
	cmp	r0, #0
	beq	2f
	ldr	r8, =DirectColourMaps
	ldrb	r0, [r7, #7] // DirectColourMapsNeedRebuild
	add	r8, r8, r3, lsl #9
	cmp	r0, #0
	beq	3f
	bl	S9xBuildDirectColourMaps
	b	3f

2:	ldr	r0, [r8, #24] // StartPalette
	ldr	r1, [r8, #28] // PaletteShift
	add	r7, #72 // ScreenColors
	add	r0, r0, r3, lsl r1
	add	r8, r7, r0, lsl #1
3:
.endm

.macro TILE_AssignPixel b, src, dst
	ldrb	r6, [r7, #\dst]
.if \src == 0
	ands	r0, r9, lr, lsl #1
.else
	ands	r0, r9, lr, lsr #\src * 8 - 1
.endif
	cmpne	r1, r6
	ble	1f
.if \b == 2
	ldrh	r0, [r8, r0]
	strb	r4, [r7, #\dst]
	strh	r0, [r12, #\dst * 2]
.else
	ldrb	r0, [r8, r0]
	strb	r4, [r7, #\dst]
	strb	r0, [r12, #\dst]
.endif
1:
.endm
.macro DRAWTILE_CLIP0 x
	cmp	lr, #0
.endm
.macro DRAWTILE_CLIP1 x
	ands	lr, \x
.endm
.macro DRAWTILE_CLIP2 x
	ldr	r6, [sp, #4 + \x]
	ands	lr, r6
.endm
.macro DRAWTILE_TEMP b, clip
.if \clip == 0
	cmp	r3, #0
	bxeq	lr
	push	{r1-r9,lr}
	TILE_PREAMBLE r9
	pop	{r0,r2,r3}
.else
	push	{r1-r11,lr}
	TILE_CLIP_PREAMBLE
	TILE_PREAMBLE r11
	ldr	r2, [sp, #48]
	ldr	r3, [sp, #48 + 4]
	ldr	r0, [sp], #12
	cmp	r3, #0
	popeq	{r4-r11,pc}
.endif
	tst	r6, 0x8000 // V_FLIP
	rsbne	r2, #56
	add	r5, r2
	mov	r2, #8
	movne	r2, #-8

	ldr	r4, =GFX
	ldr	r9, =0x1fe
	ldr	r12, [r4, #60] // S
	ldr	r7, [r4, #64] // DB
.if \b == 2
	add	r12, r12, r0, lsl #1
.else
	add	r12, r0
.endif
	add	r7, r0
	ldrb	r1, [r4, #76] // Z1
	ldrb	r4, [r4, #77] // Z2
	tst	r6, 0x4000 // H_FLIP
	bne	4f

3:	ldr	lr, [r5] // Pixels
	DRAWTILE_CLIP\clip r10
	beq	2f
	TILE_AssignPixel \b, 0, 0
	TILE_AssignPixel \b, 1, 1
	TILE_AssignPixel \b, 2, 2
	TILE_AssignPixel \b, 3, 3
2:	ldr	lr, [r5, #4]
	DRAWTILE_CLIP\clip r11
	beq	2f
	TILE_AssignPixel \b, 0, 4
	TILE_AssignPixel \b, 1, 5
	TILE_AssignPixel \b, 2, 6
	TILE_AssignPixel \b, 3, 7
2:	add	r5, r2
	add	r7, #256 * \b
	add	r12, #256 * \b * \b
	subs	r3, #1
	bhi	3b
.if \clip == 0
	pop	{r4-r9,pc}
.else
	pop	{r4-r11,pc}
.endif

4:	ldr	lr, [r5, #4] // Pixels
	DRAWTILE_CLIP\clip r11
	beq	2f
	TILE_AssignPixel \b, 3, 0
	TILE_AssignPixel \b, 2, 1
	TILE_AssignPixel \b, 1, 2
	TILE_AssignPixel \b, 0, 3
2:	ldr	lr, [r5]
	DRAWTILE_CLIP\clip r10
	beq	2f
	TILE_AssignPixel \b, 3, 4
	TILE_AssignPixel \b, 2, 5
	TILE_AssignPixel \b, 1, 6
	TILE_AssignPixel \b, 0, 7
2:	add	r5, r2
	add	r7, #256 * \b
	add	r12, #256 * \b * \b
	subs	r3, #1
	bhi	4b
.if \clip == 0
	pop	{r4-r9,pc}
.else
	pop	{r4-r11,pc}
.endif
.endm
CODE32_FN _Z8DrawTilejjjj
	DRAWTILE_TEMP 1, 0
CODE32_FN _Z15DrawClippedTilejjjjjj
	DRAWTILE_TEMP 1, 1
CODE32_FN _Z10DrawTile16jjjj
	DRAWTILE_TEMP 2, 0
CODE32_FN _Z17DrawClippedTile16jjjjjj
	DRAWTILE_TEMP 2, 1

.macro TILE_AddPixel16
	orr	r0, r0, r0, lsl #16
	orr	r6, r6, r6, lsl #16
	and	r0, r10
	and	r6, r10
	add	r0, r6
	add	r6, r10, r0, lsr #5
	bic	r6, r0
	bic	r0, r10, r6
	orr	r0, r0, r0, lsr #16
.endm
.macro TILE_AddPixel16Half
	orr	r0, r0, r0, lsl #16
	orr	r6, r6, r6, lsl #16
	and	r0, r10
	and	r6, r10
	add	r0, r6
	addeq	r6, r10, r0, lsr #5
	biceq	r6, r0
	andne	r0, r10, r0, lsr #1
	biceq	r0, r10, r6
	orr	r0, r0, r0, lsr #16
.endm
.macro TILE_AddFPixel16Half
	orr	r0, r0, r0, lsl #16
	and	r0, r10
	add	r0, r11
	and	r0, r10, r0, lsr #1
	orr	r0, r0, r0, lsr #16
.endm
.macro TILE_SubPixel16
	orr	r0, r0, r0, lsl #16
	orr	r6, r6, r6, lsl #16
	orr	r0, r0, r10, lsl #5
	and	r6, r10
	sub	r0, r6
	orr	r6, r0, r10
	and	r0, r10
	sub	r6, r6, r10, lsl #5
	bic	r0, r0, r6, lsr #5
	orr	r0, r0, r0, lsr #16
.endm
.macro TILE_SubPixel16Half
	orr	r0, r0, r0, lsl #16
	orr	r6, r6, r6, lsl #16
	orr	r0, r0, r10, lsl #5
	and	r6, r10
	sub	r0, r6
	orr	r6, r0, r10
	and	r0, r10
	sub	r6, r6, r10, lsl #5
	bic	r0, r0, r6, lsr #5
	andne	r0, r10, r0, lsr #1
	orr	r0, r0, r0, lsr #16
.endm
.macro TILE_SubFPixel16Half
	orr	r0, r0, r0, lsl #16
	orr	r0, r0, r10, lsl #5
	sub	r0, r11
	orr	r6, r0, r10
	and	r0, r10
	sub	r6, r6, r10, lsl #5
	bic	r0, r0, r6, lsr #5
	and	r0, r10, r0, lsr #1
	orr	r0, r0, r0, lsr #16
.endm

.macro TILE_Select16 sel, name, src, dst
	ldrb	r6, [r7, #\dst]
.if \src == 0
	ands	r0, r9, lr, lsl #1
.else
	ands	r0, r9, lr, lsr #\src * 8 - 1
.endif
	cmpne	r1, r6
	ble	1f
	ldrb	r6, [r7, #256 + \dst]
	ldrh	r0, [r8, r0]
	cmp	r6, 1
.if \sel == 3
	blo	11f
	addne	r6, r12, #512
	moveq	r6, r11
	ldrhne	r6, [r6, #\dst * 2]
.else
	bne	11f
.endif
	TILE_\name
11:	strb	r4, [r7, #\dst]
	strh	r0, [r12, #\dst * 2]
1:
.endm

.macro DRAWTILE16_TEMP num, title, sel, name, clip
.if \clip == 0
CODE32_FN _Z\num\()DrawTile16\title\()jjjj
.else
CODE32_FN _Z\num\()DrawClippedTile16\title\()jjjjjj
.endif
	push	{r1-r11,lr}
.if \clip == 0
	add	r9, sp, #4
.else
	add	r9, sp, #12 * 4
	TILE_CLIP_PREAMBLE
	str	r10, [sp, #4]
	str	r11, [sp, #8]
.endif
	TILE_PREAMBLE r11
	ldm	r9, {r2,r3}
	ldr	r0, [sp]
	cmp	r3, #0
	popeq	{r1-r11,pc}

	tst	r6, 0x8000 // V_FLIP
	rsbne	r2, #56
	add	r5, r2
	mov	r2, #8
	movne	r2, #-8

	ldr	r4, =GFX
	ldr	r9, =0x1fe
	ldr	r12, [r4, #60] // S
	ldr	r7, [r4, #64] // DB
	add	r12, r12, r0, lsl #1
	add	r7, r0
	ldrh	r11, [r4, #80] // FixedColour
	ldr	r10, =0x07c0f81f
	ldrb	r1, [r4, #76] // Z1
	ldrb	r4, [r4, #77] // Z2
.if \sel == 2
	orr	r11, r11, r11, lsl #16
	and	r11, r10
.endif
	tst	r6, 0x4000 // H_FLIP
	bne	4f

3:	ldr	lr, [r5] // Pixels
	DRAWTILE_CLIP\clip 0
	beq	2f
	TILE_Select16 \sel, \name, 0, 0
	TILE_Select16 \sel, \name, 1, 1
	TILE_Select16 \sel, \name, 2, 2
	TILE_Select16 \sel, \name, 3, 3
2:	ldr	lr, [r5, #4]
	DRAWTILE_CLIP\clip 4
	beq	2f
	TILE_Select16 \sel, \name, 0, 4
	TILE_Select16 \sel, \name, 1, 5
	TILE_Select16 \sel, \name, 2, 6
	TILE_Select16 \sel, \name, 3, 7
2:	add	r5, r2
	add	r7, #256 * 2
	add	r12, #256 * 4
	subs	r3, #1
	bhi	3b
	pop	{r1-r11,pc}

4:	ldr	lr, [r5, #4] // Pixels
	DRAWTILE_CLIP\clip 4
	beq	2f
	TILE_Select16 \sel, \name, 3, 0
	TILE_Select16 \sel, \name, 2, 1
	TILE_Select16 \sel, \name, 1, 2
	TILE_Select16 \sel, \name, 0, 3
2:	ldr	lr, [r5]
	DRAWTILE_CLIP\clip 0
	beq	2f
	TILE_Select16 \sel, \name, 3, 4
	TILE_Select16 \sel, \name, 2, 5
	TILE_Select16 \sel, \name, 1, 6
	TILE_Select16 \sel, \name, 0, 7
2:	add	r5, r2
	add	r7, #256 * 2
	add	r12, #256 * 4
	subs	r3, #1
	bhi	4b
	pop	{r1-r11,pc}
.endm
	DRAWTILE16_TEMP 13, Add, 3, AddPixel16, 0
	DRAWTILE16_TEMP 20, Add, 3, AddPixel16, 2
	DRAWTILE16_TEMP 16, Add1_2, 3, AddPixel16Half, 0
	DRAWTILE16_TEMP 23, Add1_2, 3, AddPixel16Half, 2
	DRAWTILE16_TEMP 13, Sub, 3, SubPixel16, 0
	DRAWTILE16_TEMP 20, Sub, 3, SubPixel16, 2
	DRAWTILE16_TEMP 16, Sub1_2, 3, SubPixel16Half, 0
	DRAWTILE16_TEMP 23, Sub1_2, 3, SubPixel16Half, 2
	DRAWTILE16_TEMP 21, FixedAdd1_2, 2, AddFPixel16Half, 0
	DRAWTILE16_TEMP 28, FixedAdd1_2, 2, AddFPixel16Half, 2
	DRAWTILE16_TEMP 21, FixedSub1_2, 2, SubFPixel16Half, 0
	DRAWTILE16_TEMP 28, FixedSub1_2, 2, SubFPixel16Half, 2

CODE32_FN scr_clear16_asm
	subs	r2, r1
	bxmi	lr
	push	{r4-r5,lr}
	add	r0, r0, r1, lsl #10
	mov	r1, r3
	mov	r4, r3
	mov	r5, r3
1:	sub	r2, #256 << 16
2:	adds	r2, #8 << 16
	stmia	r0!, {r1,r3-r5}
	bmi	2b
	add	r1, #512
	subs	r2, #1
	bpl	1b
	pop	{r4-r5,pc}

CODE32_FN scr_update16_1d1_asm
	push	{r4-r8,lr}
1:	sub	r2, #256 << 16
2:	ldmia	r1!, {r3-r8,r12,lr}
	adds	r2, #16 << 16
	stmia	r0!, {r3-r8,r12,lr}
	bmi	2b
	add	r1, #256 * 2
	subs	r2, #1
	bne	1b
	pop	{r4-r8,pc}

// more complicated version than C reference
CODE32_FN scr_update16_5x4d4_asm
	push	{r4-r10,lr}
	ldr	lr, =0x7fffffff - (0x08210821 >> 1)
1:	sub	r2, #256 << 16
2:	ldmia	r1!, {r3-r5,r9}
	adds	r2, #8 << 16

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

	stmia	r0!, {r3-r6,r9}
	bmi	2b
	add	r1, #256 * 2
	subs	r2, #1
	bne	1b
	pop	{r4-r10,pc}

CODE32_FN scr_update16_1d2_asm
	ldr	r12, =0x07e0f81f
	push	{r4-r6,lr}
	bic	lr, r12, r12, lsl #1
	sub	r2, #1
1:	sub	r2, #256 << 16
2:	ldr	r6, [r1, #512 * 2]
	ldr	r4, [r1], #4
	and	r3, r6, r12
	and	r5, r4, r12
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
	strh	r4, [r0], #2
	adds	r2, #2 << 16
	bmi	2b
	add	r1, #512 * 3
	subs	r2, #2
	bhi	1b
	popne	{r4-r6,pc}
	sub	r2, #256 << 16
2:	ldr	r4, [r1], #4
	subs	r2, #2
	and	r5, r12, r4
	and	r4, r12, r4, ror #16
	add	r4, r5
	// rounding half to even
	and	r5, lr, r4, lsr #2
	add	r4, lr
	add	r4, r5
	and	r4, r12, r4, lsr #2
	orr	r4, r4, r4, lsr #16
	strh	r4, [r0], #2
	bne	2b
	pop	{r4-r6,pc}

CODE32_FN scr_update16_5x4d8_asm
	ldr	r12, =0x07e0f81f
	push	{r4-r11,lr}
	bic	lr, r12, r12, lsl #1
	sub	r2, #1
1:	sub	r2, #256 << 16
2:	ldr	r11, [r1, #512 * 2]
	ldr	r10, [r1], #4	// 01

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
	strh	r6, [r0], #2	// 0011

	lsr	r9, r11, #16
	lsr	r7, r10, #16
	orr	r7, r9, r7, lsl #16
	and	r9, r12, r7
	and	r7, r12, r7, ror #16
	add	r5, r7, r9

	ldr	r11, [r1, #512 * 2]
	ldr	r10, [r1], #4	// 23

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
	strh	r6, [r0], #2	// 1223

	ldr	r11, [r1, #512 * 2]
	ldr	r10, [r1], #4	// 45

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
	strh	r6, [r0], #2	// 3344

	ldr	r11, [r1, #512 * 2]
	ldr	r10, [r1], #4	// 67

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
	strh	r6, [r0], #2	// 4556

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
	strh	r6, [r0], #2	// 6677

	adds	r2, #8 << 16
	bmi	2b
	add	r1, #512 * 3
	subs	r2, #2
	bhi	1b
	popne	{r4-r11,pc}
	mov	r2, #256
2:	ldmia	r1!, {r10,r11}	// 0123
	subs	r2, #8
	and	r7, r12, r10
	and	r6, r12, r10, ror #16
	add	r6, r7

	and	r9, lr, r6, lsr #2
	add	r6, lr
	add	r6, r9
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 0011

	lsr	r5, r10, #16
	orr	r5, r5, r5, lsl #16
	and	r5, r12

	lsl	r6, r11, #16
	lsr	r7, r11, #16
	orr	r6, r6, r6, lsr #16
	orr	r7, r7, r7, lsl #16
	and	r6, r12
	ldmia	r1!, {r10,r11}	// 4567

	add	r6, r5, r6, lsl #1
	and	r5, r7, r12
	add	r6, r5
	and	r9, lr, r6, lsr #3
	add	r6, lr
	add	r6, r6, lr, lsl #1
	add	r6, r9
	and	r6, r12, r6, lsr #3
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 1223

	lsl	r6, r10, #16
	lsr	r7, r10, #16
	orr	r6, r6, r6, lsr #16
	orr	r7, r7, r7, lsl #16
	and	r6, r12
	and	r7, r12

	add	r9, r5, r6
	add	r5, r6, r7, lsl #1
	and	r6, lr, r9, lsr #2
	add	r6, lr
	add	r6, r9
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 3344

	lsl	r6, r11, #16
	orr	r6, r6, r6, lsr #16
	and	r6, r12

	add	r6, r5
	and	r9, lr, r6, lsr #3
	add	r6, lr
	add	r6, r6, lr, lsl #1
	add	r6, r9
	and	r6, r12, r6, lsr #3
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 4556

	and	r7, r12, r11
	and	r6, r12, r11, ror #16
	add	r6, r7

	and	r9, lr, r6, lsr #2
	add	r6, lr
	add	r6, r9
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 6677
	bne	2b
	pop	{r4-r11,pc}


CODE32_FN pal_update16_asm
	push	{r4-r5,lr}
	mov	r3, #256
	sub	r1, #256 * 2
	ldr	r12, =0x1f001f
1:	ldr	r2, [r0], #4
	and	r4, r12, r2
	and	r5, r12, r2, lsr #5
	and	r2, r12, r2, lsr #10
	orr	r2, r2, r4, lsl #11
	orr	r2, r2, r5, lsl #6
	str	r2, [r1], #4
	subs	r3, #2
	bne	1b
	pop	{r4-r5,pc}

CODE32_FN pal_update32_asm
	push	{r4-r6,lr}
	mov	r3, #256
	sub	r1, #256 * 4
	ldr	r12, =0x1f001f
	ldr	r6, =0x07c0f81f
1:	ldr	r2, [r0], #4
	and	r4, r12, r2
	and	r5, r12, r2, lsr #5
	and	r2, r12, r2, lsr #10
	orr	r2, r2, r4, lsl #11
	orr	r2, r2, r5, lsl #6
	lsl	r4, r2, #16
	lsr	r5, r2, #16
	orr	r4, r4, r4, lsr #16
	orr	r5, r5, r5, lsl #16
	and	r4, r6
	and	r5, r6
	stmia	r1!, {r4,r5}
	subs	r3, #2
	bne	1b
	pop	{r4-r6,pc}


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
	push	{r6-r10,lr}
	ldr	r12, =0x1fe
	lsl	r3, #8
1:	ldmia	r1!, {r9,lr}
	UPDATE_COPY4 r6, r7, r8, r9
	UPDATE_COPY4 r8, r9, r10, lr
	stmia	r0!, {r6-r9}
	subs	r3, #8
	bne	1b
	pop	{r6-r10,pc}

CODE32_FN scr_update_5x4d4_asm
	push	{r4-r11,lr}
	ldr	r10, =0x07c0f81f
	ldr	lr, =0x1fe
	bic	r12, r10, r10, lsl #1 // 0x400801
	lsl	r3, #8
1:	ldmia	r1!, {r9,r11}
	and	r6, lr, r9, lsl #1
	and	r7, lr, r9, lsr #7
	and	r8, lr, r9, lsr #15
	and	r9, lr, r9, lsr #23
	ldrh	r6, [r2, r6]
	ldrh	r7, [r2, r7]
	ldrh	r8, [r2, r8]
	ldrh	r9, [r2, r9]
	orr	r4, r6, r7, lsl #16	// a1, b1
	orr	r5, r7, r8, lsl #16	// b1, c1
	and	r6, lr, r11, lsl #1
	and	r7, lr, r11, lsr #7
	ldrh	r6, [r2, r6]	// a2
	ldrh	r7, [r2, r7]	// b2
	orr	r6, r9, r6, lsl #16	// d1, a2

	and	r9, r10, r5
	and	r5, r10, r5, ror #16
	add	r5, r9
	and	r9, r12, r5, lsr #1
	add	r5, r9
	and	r5, r10, r5, lsr #1
	orr	r5, r5, r5, lsl #16
	lsr	r5, #16
	orr	r5, r5, r8, lsl #16	// bc1, c1

	and	r8, lr, r11, lsr #15
	and	r9, lr, r11, lsr #23
	ldrh	r8, [r2, r8]
	ldrh	r9, [r2, r9]
	orr	r11, r7, r8, lsl #16	// b2, c2
	orr	r8, r8, r9, lsl #16	// c2, d2

	and	r9, r10, r11
	and	r11, r10, r11, ror #16
	add	r11, r9
	and	r9, r12, r11, lsr #1
	add	r11, r9
	and	r11, r10, r11, lsr #1
	orr	r11, r11, r11, lsr #16

	orr	r7, r7, r11, lsl #16	// b2, bc2
	stmia	r0!, {r4-r8}
	subs	r3, #8
	bne	1b
	pop	{r4-r11,pc}

CODE32_FN scr_update_1d2_asm
	push	{r4-r11,lr}
	sub	r2, #256 * 2
	ldr	r5, =0x07c0f81f
	mov	r4, #0x3fc
	bic	lr, r5, r5, lsl #1 // 0x400801
	sub	r3, #1
1:	sub	r3, #256 << 16
2:	ldr	r11, [r1, #256]
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
	and	r6, r5, r6, lsr #2
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
	and	r6, r5, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	orr	r6, r11, r6, lsl #16
	str	r6, [r0], #4
	adds	r3, #4 << 16
	bmi	2b
	add	r1, #256
	subs	r3, #2
	bhi	1b
	popne	{r4-r11,pc}
	mov	r3, #256
2:	ldr	r10, [r1], #4
	subs	r3, #4
	and	r6, r4, r10, lsl #2
	and	r7, r4, r10, lsr #6
	and	r8, r4, r10, lsr #14
	and	r9, r4, r10, lsr #22
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r6]
	ldr	r9, [r2, r7]
	add	r6, r7
	and	r7, lr, r6, lsr #2
	add	r6, lr
	add	r6, r7
	and	r6, r5, r6, lsr #2
	orr	r11, r6, r6, lsl #16
	lsr	r11, #16
	add	r6, r8, r9
	and	r7, lr, r6, lsr #2
	add	r6, lr
	add	r6, r7
	and	r6, r5, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	orr	r6, r11, r6, lsl #16
	str	r6, [r0], #4
	bne	2b
	pop	{r4-r11,pc}

CODE32_FN scr_update_5x4d8_asm
	push	{r4-r11,lr}
	sub	r2, #256 * 2
	ldr	r12, =0x07c0f81f
	mov	r4, #0x3fc
	bic	lr, r12, r12, lsl #1 // 0x400801
	sub	r3, #1
1:	sub	r3, #256 << 16
2:	ldr	r11, [r1, #256]
	ldr	r10, [r1], #4
	and	r8, r4, r11, lsl #2
	and	r9, r4, r11, lsr #6
	and	r6, r4, r10, lsl #2
	and	r7, r4, r10, lsr #6
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	ldr	r9, [r2, r9]
	add	r6, r8
	add	r5, r7, r9
	add	r6, r5
	and	r9, lr, r6, lsr #2
	add	r6, lr
	add	r6, r9
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 0011

	and	r8, r4, r11, lsr #14
	and	r9, r4, r11, lsr #22
	and	r6, r4, r10, lsr #14
	and	r7, r4, r10, lsr #22
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	ldr	r9, [r2, r9]
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
	strh	r6, [r0], #2	// 1223

	ldr	r11, [r1, #256]
	ldr	r10, [r1], #4
	and	r8, r4, r11, lsl #2
	and	r9, r4, r11, lsr #6
	and	r6, r4, r10, lsl #2
	and	r7, r4, r10, lsr #6
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	ldr	r9, [r2, r9]
	add	r6, r8
	add	r7, r9
	add	r9, r5, r6
	add	r5, r6, r7, lsl #1
	and	r6, lr, r9, lsr #2
	add	r6, lr
	add	r6, r9
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 3344

	and	r8, r4, r11, lsr #14
	and	r9, r4, r11, lsr #22
	and	r6, r4, r10, lsr #14
	and	r7, r4, r10, lsr #22
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	ldr	r9, [r2, r9]
	add	r6, r8
	add	r7, r9
	add	r7, r6
	add	r6, r5

	and	r9, lr, r6, lsr #3
	add	r6, lr
	add	r6, r6, lr, lsl #1
	add	r6, r9
	and	r6, r12, r6, lsr #3
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 4556

	and	r6, lr, r7, lsr #2
	add	r6, lr
	add	r6, r7
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 6677

	adds	r3, #8 << 16
	bmi	2b
	add	r1, #256
	subs	r3, #2
	bhi	1b
	popne	{r4-r11,pc}
	mov	r3, #256
2:	ldmia	r1!, {r10,r11}
	subs	r3, #8
	and	r6, r4, r10, lsl #2
	and	r7, r4, r10, lsr #6
	and	r8, r4, r10, lsr #14
	and	r9, r4, r10, lsr #22
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	ldr	r9, [r2, r9]

	add	r6, r7
	and	r5, lr, r6, lsr #2
	add	r6, lr
	add	r6, r5
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 0011

	add	r6, r7, r8, lsl #1
	add	r6, r9
	and	r5, lr, r6, lsr #3
	add	r6, lr
	add	r6, r6, lr, lsl #1
	add	r6, r5
	and	r6, r12, r6, lsr #3
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 1223

	and	r6, r4, r11, lsl #2
	and	r7, r4, r11, lsr #6
	and	r8, r4, r11, lsr #14
	ldr	r6, [r2, r6]
	ldr	r7, [r2, r7]
	ldr	r8, [r2, r8]
	add	r7, r6, r7, lsl #1
	add	r6, r9
	and	r9, r4, r11, lsr #22
	and	r5, lr, r6, lsr #2
	ldr	r9, [r2, r9]
	add	r6, lr
	add	r6, r5
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 3344

	add	r6, r7, r8
	and	r5, lr, r6, lsr #3
	add	r6, lr
	add	r6, r6, lr, lsl #1
	add	r6, r5
	and	r6, r12, r6, lsr #3
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 4556

	add	r6, r8, r9
	and	r5, lr, r6, lsr #2
	add	r6, lr
	add	r6, r5
	and	r6, r12, r6, lsr #2
	orr	r6, r6, r6, lsr #16
	strh	r6, [r0], #2	// 6677
	bne	2b
	pop	{r4-r11,pc}

