/*
 * util.s
 *
 * Created: 11/20/2020 12:08:14 PM
 *  Author: Wayne Lee
 */

    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .text

    .align  2
    .global push_fp
    .thumb
    .thumb_func
    .type   push_fp, %function
push_fp:
    mov     r0, sp                  @ r0 = sp, save the sp
    vpush   {s16-s31}               @ push 16 FP register into stack
    mov     r1, sp
    mov     sp, r0                  @ restore the saved sp
    mov     r0, r1                  @ return the sp that after pushing the 16 FP register
    bx      lr
    .size   push_fp, .-push_fp


    .align  2
    .global gen_svc_irq
    .thumb
    .thumb_func
    .type   gen_svc_irq, %function
gen_svc_irq:
    svc     #0
    bx      lr
    .size   gen_svc_irq, .-gen_svc_irq
