/*
 * sys_ticks.h
 *
 *  Created on: Aug 10, 2013
 *      Author: wayne
 */

#ifndef _SYSTICK_HANDLER_H_
#define _SYSTICK_HANDLER_H_


#include <stdbool.h>
#include "stm32f4xx/main.h"


extern uint32_t sys_tick_per_sec;
extern uint32_t sys_tick_period;

extern volatile uint64_t sys_clock;
extern volatile uint32_t sys_tick_counter;


void sysTimeCalibrate(void);

void sysTickStart(void);
void sysTickStop(void);
void sysTickHandlerCnfg(uint32_t usec, uint32_t ratio);
void sysTickIntHandler(void);
uint64_t sys_clock_get(void);


static inline uint32_t sysTickGetPeriod(void)
{
    return (SysTick->LOAD + 1);
}

static inline uint32_t get_sys_tick_counter(void)
{
    return sys_tick_counter;
}

static inline uint64_t sys_clock_get_kernel(void)
{
    return sys_clock + (uint64_t) (sys_tick_period - SysTick->VAL);
}

static inline uint64_t sys_clock_get_inline(void)
{
    volatile uint32_t  sys_tick_cntr_samp;
    volatile uint32_t  sys_tick_cur;
    uint64_t  sys_clk;

    do {
        sys_tick_cntr_samp = sys_tick_counter;
        sys_tick_cur = SysTick->VAL;
    } while (sys_tick_counter != sys_tick_cntr_samp);

    sys_clk = (uint64_t) sys_tick_cntr_samp * (uint64_t) sys_tick_period;
    return sys_clk + (uint64_t) (sys_tick_period - sys_tick_cur);
}


// ###########################################################################
// Testing/Verification API
// ###########################################################################

#define TIMER_STAMP_NUM   10
extern uint32_t timer_stamp[];
extern uint32_t timer_stamp_count;

void sysTickTestPush(void);
void sysTimerTest(void);
void sysTickTest(void);


#endif /* _SYSTICK_HANDLER_H_ */
