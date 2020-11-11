/*
 * task_ctx.s
 *
 * Created: 10/9/2020 12:08:14 PM
 *  Author: Wayne Lee
 */

    .syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .text

    /* ********************************************************
     * Get the stack pointer
     * ********************************************************/
    .align  2
    .global get_stack_ptr
    .thumb
    .thumb_func
    .type   get_stack_ptr, %function
get_stack_ptr:
    mov     r0, r13
    bx      lr
    .size   get_stack_ptr, .-get_stack_ptr


    .align  2
    .global get_sp_main
    .thumb
    .thumb_func
    .type   get_sp_main, %function
get_sp_main:
    mrs     r0, msp
    bx      lr
    .size   get_sp_main, .-get_sp_main


    .align  2
    .global get_sp_process
    .thumb
    .thumb_func
    .type   get_sp_process, %function
get_sp_process:
    mrs     r0, psp
    bx      lr
    .size   get_sp_process, .-get_sp_process


    .equ ICSR_ADDR,   0xE000ED04    @ Interrupt Control State Register Address
    .equ PENDSVSET,   0x10000000    @ pendSV bit in the Interrupt Control State Register

    .align  2
    .global gen_pend_sv_irq
    .thumb
    .thumb_func
    .type   gen_pend_sv_irq, %function
gen_pend_sv_irq:
    ldr     r1, =ICSR_ADDR            /*  Invoke PendSV exception  */
    ldr     r0, =PENDSVSET
    str     r0, [r1]
    bx      lr
    .size   gen_pend_sv_irq, .-gen_pend_sv_irq
