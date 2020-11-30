/*
 * bench_core.S
 *
 * Created on: Nov 2. 2020
 *     Author: Y.W. Lee
 */ 

	.syntax unified
	.text

	.align  2
	.global bench_X
	.thumb
	.thumb_func
	.type   bench_X, %function
bench_X:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push    {r4, r5, r6, lr}
	mov     r6, r0
	ldr     r4, .L21
	movs    r5, #0
.L18:
	mov     r1, r6
	movs    r0, #1
	bl      api_Z
	subs    r4, r4, #1
	add     r5, r5, r0
	bne     .L18
	mov     r0, r5
	pop     {r4, r5, r6, pc}
.L22:
	.align  2
.L21:
	.word   500000
	.size   bench_X, .-bench_X


	.align  2
	.global api_Z
	.thumb
	.thumb_func
	.type   api_Z, %function
api_Z:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	add     r0, r0, r1
	bx      lr
	.size   api_Z, .-api_Z
