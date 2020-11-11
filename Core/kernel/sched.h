/*
 * schd.h
 *
 *  Created on: Aug 10, 2013
 *      Author: wayne
 */

#ifndef _SCHD_H_
#define _SCHD_H_

#include "sys_core/sys_ticks.h"
#include "sys_core/sys_timer.h"
#include "kernel/task_ctx.h"


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


#endif /* SCHD_H_ */
