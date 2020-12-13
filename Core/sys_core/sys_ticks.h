/*
 * sys_ticks.h
 *
 *  Created on: Aug 10, 2013
 *      Author: Y.W. Lee
 */

#ifndef _SYS_TICKS_H_
#define _SYS_TICKS_H_


#include <stdbool.h>
#include "stm32f4xx/main.h"


extern volatile uint32_t sys_ticks;
extern uint32_t sys_ticks_per_sec;
extern uint32_t sys_tick_period;     // in CPU cycles


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


// ###########################################################################
// Testing/Verification API
// ###########################################################################

#define TIMER_STAMP_NUM   10
extern uint32_t timer_stamp[];
extern uint32_t timer_stamp_count;

void sysTickTestPush(void);
void sysTimerTest(void);
void sysTickTest(void);


#endif /* _SYS_TICKS_H_ */
