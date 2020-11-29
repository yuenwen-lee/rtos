/*
 * sys_util.h
 *
 *  Created on: Oct 17, 2020
 *      Author: Y.W. Lee
 */

#ifndef _SYS_UTIL_H_
#define _SYS_UTIL_H_


extern int32_t sys_disable_irq_req;


static inline void enable_irq(void)
{
    // Enable IRQ Interrupts by clearing the I-bit in the CPSR.
    __asm volatile ("cpsie i" : : : "memory");
}

static inline void disable_irq(void)
{
    // Disable IRQ Interrupts by setting the I-bit in the CPSR.
    __asm volatile ("cpsid i" : : : "memory");
}

static inline void cpu_irq_enter_critical(void)
{
    disable_irq();
    ++sys_disable_irq_req;
}

static inline void cpu_irq_leave_critical(void)
{
    --sys_disable_irq_req;
    if (sys_disable_irq_req <= 0) {
        sys_disable_irq_req = 0;
        enable_irq();
    }
}


// ###########################################################################
// Information API
// ###########################################################################
void system_heap_update(uint8_t *heap_end);
void system_info_heap(void);
void system_info_linker(void);
void system_nvic_priority_dump(void);
void system_info_summary(void);

// ###########################################################################
// Testing/Verification API
// ###########################################################################
void mallocTest(void);
void system_bringup_test(void);
void system_math_test(void);


#endif /* _SYS_UTIL_H_ */
