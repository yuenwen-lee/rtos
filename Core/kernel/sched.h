/*
 * schd.h
 *
 *  Created on: Aug 10, 2013
 *      Author: Y.W. Lee
 */

#ifndef _SCHED_H_
#define _SCHED_H_

#include "sys_core/sys_ticks.h"
#include "sys_core/sys_timer.h"
#include "kernel/task_ctx.h"


#define EXC_RETURN_BIT4_SHIFT    4
#define EXC_RETURN_BIT4_MASK     (0x1 << EXC_RETURN_BIT4_SHIFT)


void idle_loop(void *argv);

uint32_t scheduler_core(uint32_t exc_return);
void schedule_stack_ov(uint32_t err_count);
void run_stat_reg_isr_sched(void);


static inline void scheduler(void)
{
    gen_pend_sv_irq();
}

static inline void schedule_halt(void)
{
    sysTickStop();
    sys_timer_stop();
}

static inline uint32_t get_exc_return_b4(uint32_t exc_return)
{
    return ((exc_return >> EXC_RETURN_BIT4_SHIFT) & 0x1);
}


#endif /* SCHED_H_ */
