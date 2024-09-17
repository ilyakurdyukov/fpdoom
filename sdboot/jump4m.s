@ -*- tab-width: 8 -*-
	.arch armv5te
	.syntax unified

	.section .text, "ax", %progbits
	.p2align 2
	.type _start, %function
	.global _start

// SC6530/SC6531 devices only have access to the first 4MB
// of flash memory when booting. This trampoline is required
// if the sdboot section is located after 4MB.

	.long	0x000a3d3d
2:	.long	0xffffffff
.if 1
	.code 32
_start:
	ldr	r0, 1f
	mov	r1, #16
	str	r1, [r0]
	ldr	pc, 2b
.else
	.code 16
_start:
	ldr	r0, 1f
	movs	r1, #16
	str	r1, [r0]
	ldr	r0, 2b
	mov	pc, r0
	.p2align 2
.endif

1:	.long	0x20a00200
