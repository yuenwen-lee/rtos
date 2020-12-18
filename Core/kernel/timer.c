/*
 * timer.c
 *
 *  Created on: Sep 19, 2013
 *      Author: Y.W. Lee
 */ 

#include <stdint.h>

#include "sys_core/sys_timer.h"
#include "kernel/task.h"
#include "kernel/timer.h"
#include "kernel/sleep.h"
#include "sys_device/dev_board.h"


static void timer_wait_short(timer_obj_t *timer_p);
static void timer_wait_long(timer_obj_t *timer_p);
static uint32_t timer_wait_fixed_short(timer_obj_t *timer_p);
static uint32_t timer_wait_fixed_long(timer_obj_t *timer_p);


void timer_init(timer_obj_t *timer_p, uint32_t msec)
{
    uint64_t period_ticks_64;

    timer_p->task_id = task_id_run;
    timer_p->period_msec = msec;
    timer_p->count = 0;
    period_ticks_64 = msec_to_cpu_tick_64(msec);
    if ((period_ticks_64 >> CPU_TICKS_SLEEP_SHIFT) == 0) {
        timer_p->period_ticks    = (uint32_t) period_ticks_64;
        timer_p->period_ticks_64 = 0;
    } else {
        timer_p->period_ticks    = 0;
        timer_p->period_ticks_64 = period_ticks_64;
    }
}


void timer_wait(timer_obj_t *timer_p)
{
    if (timer_p->period_ticks != 0) {
        timer_wait_short(timer_p);
    } else {
        timer_wait_long(timer_p);
    }
}


uint32_t timer_wait_fixed(timer_obj_t *timer_p)
{
    if (timer_p->period_ticks != 0) {
        return timer_wait_fixed_short(timer_p);
    } else {
        return timer_wait_fixed_long(timer_p);
    }
}


static void timer_wait_short(timer_obj_t *timer_p)
{
    uint32_t t_now;

    t_now = sys_timer_get_inline();
    if (timer_p->count == 0) {
        timer_p->time_wakeup = t_now;
    }
    timer_p->time_wakeup += timer_p->period_ticks;

    if ((int32_t) (timer_p->time_wakeup - t_now) > 0) {
        sleep_core(timer_p->time_wakeup);  // put to sleep
    }
    ++timer_p->count;
}


static void timer_wait_long(timer_obj_t *timer_p)
{
    if (timer_p->count == 0) {
        timer_p->time_wakeup_64 = sys_timer_64_get_inline();
    }
    timer_p->time_wakeup_64 += timer_p->period_ticks_64;

    sleep_core_64(timer_p->time_wakeup_64);
    ++timer_p->count;
}


static uint32_t timer_wait_fixed_short(timer_obj_t *timer_p)
{
    uint32_t t_now;

    t_now = sys_timer_get_inline();
    if (timer_p->count == 0) {
        timer_p->time_wakeup = t_now;
    }
    timer_p->time_wakeup += timer_p->period_ticks;

    uint32_t ov_count;
    int32_t t_dlt = (int32_t) (t_now - timer_p->time_wakeup);
    if (t_dlt == 0) {
        // on time ...
        ov_count = 0;

    } else if (t_dlt > 0) {
        // overdue ...
        ov_count = (t_dlt + (timer_p->period_ticks - 1)) / timer_p->period_ticks;
        timer_p->time_wakeup += (ov_count * timer_p->period_ticks);
        sleep_core(timer_p->time_wakeup);  // put to sleep

    } else {
        // expected ...
        ov_count = 0;
        sleep_core(timer_p->time_wakeup);  // put to sleep
    }

    ++timer_p->count;
    return ov_count;
}


static uint32_t timer_wait_fixed_long(timer_obj_t *timer_p)
{
    uint64_t t_now_64;

    t_now_64 = sys_timer_64_get_inline();
    if (timer_p->count == 0) {
        timer_p->time_wakeup_64 = t_now_64;
    }
    timer_p->time_wakeup_64 += timer_p->period_ticks_64;

    uint64_t ov_count;
    if (timer_p->time_wakeup_64 == t_now_64) {
        // on time ...
        ov_count = 0;

    } else if (timer_p->time_wakeup_64 > t_now_64) {
        // expected ...
        ov_count = 0;
        sleep_core_64(timer_p->time_wakeup_64);

    } else {
        // overdue ...
        uint64_t t_diff_64 = t_now_64 - timer_p->time_wakeup_64;
        ov_count = (t_diff_64 + (timer_p->period_ticks_64 - 1)) / timer_p->period_ticks_64;
        timer_p->time_wakeup_64 += (ov_count * timer_p->period_ticks_64);
        sleep_core_64(timer_p->time_wakeup_64);
    }

    ++timer_p->count;
    return (uint32_t) ov_count;
}
