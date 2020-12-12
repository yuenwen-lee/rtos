/*
 * timer.h
 *
 *  Created on: Sep 19, 2013
 *      Author: Y.W. Lee
 */

#ifndef _TIMER_H_
#define _TIMER_H_


#define CPU_TICKS_SLEEP_SHIFT  31
#define CPU_TICKS_SLEEP_STEP   (1 << CPU_TICKS_SLEEP_SHIFT)
#define CPU_TICKS_SLEEP_MASK   (((uint32_t) -1) >> (32 - CPU_TICKS_SLEEP_SHIFT))


typedef struct timer_obj_ {
    uint32_t  task_id;
    uint32_t  period_msec;
    uint32_t  period_ticks;        // period_ticks = period_ent_ticks, if period_ent_count == 0
    uint32_t  period_ent_ticks;    // period = ((CPU_TICKS_SLEEP_STEP * period_ent_count) + 
    uint32_t  period_ent_count;    //           period_ent_ticks)
    uint32_t  time_wakeup;   // next wake up time
    uint64_t  count;
} timer_obj_t;


void timer_init(timer_obj_t *timer_p, uint32_t msec);
void timer_wait(timer_obj_t *timer_p);
uint32_t timer_wait_fixed(timer_obj_t *timer_p);


#endif /* _TIMER_H_ */
