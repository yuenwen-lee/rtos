/*
 * sleep.h
 *
 *  Created on: Aug 27, 2013
 *      Author: Y.W. Lee
 */

#ifndef _SLEEP_H_
#define _SLEEP_H_

#include "kernel/que.h"


#define CPU_TICKS_SLEEP_SHIFT  30
#define CPU_TICKS_SLEEP_STEP   (1 << CPU_TICKS_SLEEP_SHIFT)
#define CPU_TICKS_SLEEP_MASK   (((uint32_t) -1) >> (32 - CPU_TICKS_SLEEP_SHIFT))


typedef struct sleep_que_ {
    uint32_t   latest_wake_up_time;
    que_t      que_head_task_info;
    uint32_t   numb;
} sleep_que_t __attribute__((aligned(4)));


void sleep_init(void);
void sleep_core(uint32_t tick);
void sleep_core_64(uint64_t wake_up_time_64);
void sleep_msec(uint32_t msec);
void delay_usec(uint32_t usec);

uint32_t check_sleep_que(uint32_t time_now);


#endif /* _SLEEP_H_ */
