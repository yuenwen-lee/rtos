/*
 * schd.h
 *
 *  Created on: Aug 10, 2013
 *      Author: wayne
 */

#ifndef _SCHED_H_
#define _SCHED_H_

#include "sys_core/sys_ticks.h"
#include "sys_core/sys_timer.h"
#include "kernel/task_ctx.h"


// extern run_stat_t run_stat_sched;


void idle_loop(void *argv);

int scheduler_core(void);
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


#endif /* SCHED_H_ */
