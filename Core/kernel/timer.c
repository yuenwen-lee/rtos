/*
 * timer.c
 *
 *  Created on: Sep 19, 2013
 *      Author: Y.W. Lee
 */ 

#include <stdint.h>

#include "sys_core/sys_timer.h"
#include "sys_core/sys_ticks.h"
#include "kernel/task.h"
#include "kernel/timer.h"
#include "kernel/sleep.h"


void timer_init (timer_obj_t *timer_p, uint32_t msec)
{
    timer_p->task_id = task_id_run;
    timer_p->period_msec = msec;
    timer_p->period_tick = msec_to_cpu_tick(msec);
    timer_p->count       = 0;
}


void timer_wait (timer_obj_t *timer_p)
{
    if (timer_p->count == 0) {
        timer_p->time_wakeup = sys_timer_get_inline();
    }
    timer_p->time_wakeup += timer_p->period_tick;

    if ((int32_t) (sys_timer_get_inline() - timer_p->time_wakeup) > 0)
        return;  // already overdue ...

    sleep_core(timer_p->time_wakeup);  // put to sleep
    ++timer_p->count;
}


uint32_t timer_wait_fixed (timer_obj_t *timer_p)
{
    int32_t t_dlt;
    uint32_t ov_count;

    if (timer_p->count == 0) {
        timer_p->time_wakeup = sys_timer_get_inline();
    }
    timer_p->time_wakeup += timer_p->period_tick;

    ov_count = 0;
    t_dlt = (int32_t) (sys_timer_get_inline() - timer_p->time_wakeup);
    if (t_dlt > 0) {
        // already overdue ...
        ov_count = (t_dlt + (timer_p->period_tick -1)) / timer_p->period_tick;
        timer_p->time_wakeup += ov_count * timer_p->period_tick;
    }

    sleep_core(timer_p->time_wakeup);  // put to sleep
    ++timer_p->count;
    return ov_count;
}
