/*
 * task_ctx.s
 *
 * Created: 10/9/2020 12:08:14 PM
 *  Author: Wayne Lee
 */

    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
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


    .align  2
    .global PendSV_Handler
    .thumb
    .thumb_func
    .type   PendSV_Handler, %function
PendSV_Handler:
    @ args = 0, pretend = 0, frame = 0
    @ frame_needed = 0, uses_anonymous_args = 0
    @ link register save eliminated.
    cpsid   I                       @ Disable core int

    push    {lr}                    @ save the LR
    bl      scheduler_core          @ jump to the scheduler_core
    pop     {lr}                    @ restore the LR
    cmp     r0, #0                  @ check the retrun flag from scheduler_core
    beq     PendSV_Handler_EXIT     @ return if return flag is 0

    ldr     r1, =task_sp_ptr_now    @ r1 = &task_sp_ptr_now
    ldr     r2, =task_sp_ptr_next   @ r2 = &task_sp_ptr_next
    ldr     r1, [r1, #0]            @ r1 = task_sp_ptr_now
    ldr     r2, [r2, #0]            @ r2 = task_sp_ptr_next

    mov     r0, sp                  @ r0 = sp
    stmdb   r0!, {r4-r11}           @ push r4~r11
    vstmdb  r0!, {s16-s31}          @ push s16_s31
    str     r0, [r1, #0]            @ *task_sp_ptr_now = r0 (sp)

    ldr     r0, [r2, #0]            @ r0 = task_sp_ptr_next
    vldmia  r0!, {s16-s31}          @ pop s16~s31
    ldmia   r0!, {r4-r11}           @ pop r4~r11
    mov     sp, r0                  @ swap the sp

@   orr     lr, lr, #0x04           @ Force to new process PSP

PendSV_Handler_EXIT:
    cpsie   I                       @ enable core int
    bx      lr
    .size   PendSV_Handler, .-PendSV_Handler
