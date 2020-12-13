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


static void timer_period_setup(timer_obj_t *timer_p);
static uint32_t timer_period_update(timer_obj_t *timer_p);
static uint32_t timer_overdue_adjust(timer_obj_t *timer_p, uint32_t t_now);


void timer_init(timer_obj_t *timer_p, uint32_t msec)
{
    timer_p->task_id = task_id_run;
    timer_p->period_msec = msec;
    timer_p->count       = 0;
}


void timer_wait (timer_obj_t *timer_p)
{
    if (timer_p->count == 0) {
        timer_p->time_wakeup = sys_timer_get_inline();
    }
    timer_period_setup(timer_p);

    do {
        if ((int32_t) (sys_timer_get_inline() - timer_p->time_wakeup) < 0) 
            sleep_core(timer_p->time_wakeup);  // put to sleep
    } while (timer_period_update(timer_p));

    ++timer_p->count;
}


uint32_t timer_wait_fixed (timer_obj_t *timer_p)
{
    uint32_t t_now = sys_timer_get_inline();
    uint32_t ov_count;

    if (timer_p->count == 0) {
        timer_p->time_wakeup = t_now;
    }
    timer_period_setup(timer_p);
    ov_count = timer_overdue_adjust(timer_p, t_now);

    do {
        if ((int32_t) (sys_timer_get_inline() - timer_p->time_wakeup) < 0) 
            sleep_core(timer_p->time_wakeup);  // put to sleep
    } while (timer_period_update(timer_p));

    ++timer_p->count;
    return ov_count;
}


static void timer_period_setup(timer_obj_t *timer_p)
{
    uint64_t  ticks_dlt = msec_to_cpu_tick_64(timer_p->period_msec);
    uint32_t  count = (uint32_t) (ticks_dlt >> CPU_TICKS_SLEEP_SHIFT);
    uint32_t  ticks = (uint32_t) (ticks_dlt &  CPU_TICKS_SLEEP_MASK);

    if (count) {
        timer_p->time_wakeup += CPU_TICKS_SLEEP_STEP;
        timer_p->period_ent_count = count - 1;
        timer_p->period_ent_ticks = ticks;
    } else {
        timer_p->time_wakeup += ticks;
        timer_p->period_ent_count = 0;
        timer_p->period_ent_ticks = 0;
        timer_p->period_ticks     = ticks;
    }
}


static uint32_t timer_period_update(timer_obj_t *timer_p)
{
    if (timer_p->period_ent_count) {
        timer_p->time_wakeup += CPU_TICKS_SLEEP_STEP;
        timer_p->period_ent_count--;
        return 1;

    } else if (timer_p->period_ent_ticks) {
        timer_p->time_wakeup += timer_p->period_ent_ticks;
        timer_p->period_ent_ticks = 0;
        return 1;

    } else {
        return 0;
    }
}


static uint32_t timer_overdue_adjust(timer_obj_t *timer_p, uint32_t t_now)
{
    if (timer_p->period_ent_ticks) {
        return 0;
    }

    int32_t t_dlt = (int32_t) (t_now - timer_p->time_wakeup);
    if (t_dlt > 0) {
        // already overdue ...
        uint32_t ovd = (t_dlt + (timer_p->period_ticks -1)) / timer_p->period_ticks;
        timer_p->time_wakeup += ovd * timer_p->period_ticks;
        return ovd;

    } else {
        return 0;
    }
}


#if 0    // ........ Original Version ........

void timer_init_ORG(timer_obj_t *timer_p, uint32_t msec)
{
    timer_p->task_id = task_id_run;
    timer_p->period_msec  = msec;
    timer_p->period_ticks = msec_to_cpu_tick(msec);
    timer_p->count        = 0;
}


void timer_wait_ORG(timer_obj_t *timer_p)
{
    if (timer_p->count == 0) {
        timer_p->time_wakeup = sys_timer_get_inline();
    }
    timer_p->time_wakeup += timer_p->period_ticks;

    if ((int32_t) (sys_timer_get_inline() - timer_p->time_wakeup) < 0)
        return;  // already overdue ...

    sleep_core(timer_p->time_wakeup);  // put to sleep
    ++timer_p->count;
}


uint32_t timer_wait_fixed_ORG(timer_obj_t *timer_p)
{
    int32_t t_dlt;
    uint32_t ov_count;

    if (timer_p->count == 0) {
        timer_p->time_wakeup = sys_timer_get_inline();
    }
    timer_p->time_wakeup += timer_p->period_ticks;

    ov_count = 0;
    t_dlt = (int32_t) (sys_timer_get_inline() - timer_p->time_wakeup);
    if (t_dlt > 0) {
        // already overdue ...
        ov_count = (t_dlt + (timer_p->period_ticks -1)) / timer_p->period_ticks;
        timer_p->time_wakeup += ov_count * timer_p->period_ticks;
    }

    sleep_core(timer_p->time_wakeup);  // put to sleep
    ++timer_p->count;
    return ov_count;
}

#endif   // ........ Original Version ........
