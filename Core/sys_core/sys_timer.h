/*
 * sys_timer.h
 *
 *  Created on: Sep 6, 2013
 *      Author: wayne
 */

#ifndef _SYS_TIMER_H_
#define _SYS_TIMER_H_

#include <stdint.h>
#include "stm32f4xx/main.h"


extern TIM_HandleTypeDef sys_timer_hndl;

extern uint32_t cpu_ticks_per_sec;
extern uint32_t cpu_ticks_per_msec;
extern uint32_t cpu_ticks_per_usec;


void sys_timer_init(void);
void sys_timer_start(void);
void sys_timer_stop(void);
void sys_timer_cpu_ticks_update(void);


static inline uint32_t usec_to_cpu_tick(uint32_t usec)
{
    return (usec * cpu_ticks_per_usec);
}

static inline uint32_t msec_to_cpu_tick(uint32_t msec)
{
    return (msec * cpu_ticks_per_msec);
}

static inline uint32_t sys_timer_get_inline(void)
{
    return (volatile uint32_t) sys_timer_hndl.Instance->CNT;
}


#endif /* _SYS_TIMER_H_ */

