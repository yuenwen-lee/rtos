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
    mov     r0, lr                  @ pass lr to schduler
    bl      scheduler_core          @ jump to the scheduler_core
    pop     {lr}                    @ restore the LR
    lsr     r3, r0, #16             @ get the 'both_exc_b4' by right shifting 16 bits
    lsl     r0, #31

    cmp     r0, #0                  @ check the retrun flag from scheduler_core
    beq     PendSV_Handler_No_CTX   @ return if return flag is 0

    ldr     r1, =task_sp_ptr_now    @ r1 = &task_sp_ptr_now
    ldr     r2, =task_sp_ptr_next   @ r2 = &task_sp_ptr_next
    ldr     r1, [r1, #0]            @ r1 = task_sp_ptr_now
    ldr     r2, [r2, #0]            @ r2 = task_sp_ptr_next

    cmp     r3, #0
    beq     PendSV_Handler_00       @ Jump to Wt_F -> Wt_F
    cmp     r3, #1
    beq     PendSV_Handler_01       @ Jump to Wt_F -> No_F
    cmp     r3, #2
    beq     PendSV_Handler_10       @ Jump to No_F -> Wt_F
    cmp     r3, #3
    beq     PendSV_Handler_11       @ Jump to No_F -> No_F

PendSV_Handler_00:                  @ Wt_F -> Wt_F
    mov     r0, sp                  @ r0 = sp
    stmdb   r0!, {r4-r11}           @ push r4~r11
    vstmdb  r0!, {s16-s31}          @ push s16_s31
    str     r0, [r1, #0]            @ *task_sp_ptr_now = r0 (sp)

    ldr     r0, [r2, #0]            @ r0 = task_sp_ptr_next
    vldmia  r0!, {s16-s31}          @ pop s16~s31
    ldmia   r0!, {r4-r11}           @ pop r4~r11
    mov     sp, r0                  @ swap the sp

@   orr     lr, lr, #0x04           @ Force to new process PSP
    cpsie   I                       @ enable core int
    bx      lr

PendSV_Handler_01:                  @ Wt_F -> No_F
    mov     r0, sp                  @ r0 = sp
    stmdb   r0!, {r4-r11}           @ push r4~r11
    vstmdb  r0!, {s16-s31}          @ push s16_s31
    str     r0, [r1, #0]            @ *task_sp_ptr_now = r0 (sp)

    and     r0, #0                  @ r0 = 0
    vmsr    fpscr, r0               @ clear the FPSCR
    
    ldr     r0, [r2, #0]            @ r0 = task_sp_ptr_next
    ldmia   r0!, {r4-r11}           @ pop r4~r11
    mov     sp, r0                  @ swap the sp

    orr     lr, lr, #0x10           @ EXC_RETURN[4] = 1 --> Thread does NOT use floating unit
    cpsie   I                       @ enable core int
    bx      lr

PendSV_Handler_10:                  @ No_F -> Wt_F
    mov     r0, sp                  @ r0 = sp
    stmdb   r0!, {r4-r11}           @ push r4~r11
    str     r0, [r1, #0]            @ *task_sp_ptr_now = r0 (sp)

    ldr     r0, [r2, #0]            @ r0 = task_sp_ptr_next
    vldmia  r0!, {s16-s31}          @ pop s16~s31
    ldmia   r0!, {r4-r11}           @ pop r4~r11
    mov     sp, r0                  @ swap the sp

    mov     r3, #0xFFEF
    movt    r3, #0xFFFF             @ set r3 = 0xFFFFFFEF (BIT4 = 0)
    and     lr, lr, r3              @ EXC_RETURN[4] = 0 --> Thread use floating unit
    cpsie   I                       @ enable core int
    bx      lr

PendSV_Handler_11:                  @ No_F -> No_F
    mov     r0, sp                  @ r0 = sp
    stmdb   r0!, {r4-r11}           @ push r4~r11
    str     r0, [r1, #0]            @ *task_sp_ptr_now = r0 (sp)

    ldr     r0, [r2, #0]            @ r0 = task_sp_ptr_next
    ldmia   r0!, {r4-r11}           @ pop r4~r11
    mov     sp, r0                  @ swap the sp

@   orr     lr, lr, #0x04           @ Force to new process PSP
    cpsie   I                       @ enable core int
    bx      lr

PendSV_Handler_No_CTX:
    cpsie   I                       @ enable core int
    bx      lr
    .size   PendSV_Handler, .-PendSV_Handler
