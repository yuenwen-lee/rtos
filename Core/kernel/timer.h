/*
 * timer.h
 *
 *  Created on: Sep 19, 2013
 *      Author: Y.W. Lee
 */

#ifndef _TIMER_H_
#define _TIMER_H_


typedef struct timer_obj_ {
    uint32_t  task_id;
    uint32_t  period_msec;
    uint32_t  period_ticks;
    uint64_t  period_ticks_64;
    uint32_t  time_wakeup;
    uint64_t  time_wakeup_64;
    uint64_t  count;
} timer_obj_t;


void timer_init(timer_obj_t *timer_p, uint32_t msec);
void timer_wait(timer_obj_t *timer_p);
uint32_t timer_wait_fixed(timer_obj_t *timer_p);


#endif /* _TIMER_H_ */
