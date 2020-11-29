/*
 * sys_ticks.c
 * 
 *  Created on: Aug 10, 2013
 *      Author: Y.W. Lee
 */

#include <stdio.h>
#include "sys_ticks.h"
#include "sys_timer.h"
#include "kernel/run_stat.h"
#include "kernel/sched.h"
#include "kernel/sleep.h"


uint32_t  sys_tick_per_sec;
uint32_t  sys_tick_period;
uint32_t  sys_slot_ratio;

uint32_t  sys_slot_counter;

volatile uint64_t  sys_clock;
volatile uint32_t  sys_tick_counter;

run_stat_que_t run_stat_que_sys_tick;
run_stat_t run_stat_sys_tick;


static bool sysTickConfig(uint32_t ticks, uint8_t priority)
{
    if ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk) {
        /* Reload value impossible */
        return false;
    }

    SysTick->LOAD  = (uint32_t)(ticks - 1UL);          /* set reload register */
    SysTick->VAL   = 0UL;                              /* Load the SysTick Counter Value */
    __NVIC_SetPriority(SysTick_IRQn, priority);        /* set Priority for Systick Interrupt */
    return true;                                       /* Function successful */
}


void sysTickStart(void)
{
    SysTick->CTRL = (SysTick_CTRL_CLKSOURCE_Msk |
                     SysTick_CTRL_TICKINT_Msk   |
                     SysTick_CTRL_ENABLE_Msk);         /* Enable SysTick IRQ and SysTick Timer */
}

void sysTickStop(void)
{
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk;
}

void sysTickHandlerCnfg(uint32_t usec, uint32_t ratio)
{
    if (cpu_ticks_per_sec == 0) {
        sys_timer_cpu_ticks_update();
    }
    sys_tick_per_sec = 1000000 / usec;
    sys_slot_ratio = ratio;

    run_stat_register_isr("sys_tick", 0, &run_stat_sys_tick, &run_stat_que_sys_tick);

    // Configure SysTick to periodically interrupt.
    if (sysTickConfig(usec * cpu_ticks_per_usec, 0) == false) {
            printf("SysTick config failed .....\r\n");
        while (1) ;
    }
    sys_tick_period = sysTickGetPeriod();

    run_stat_reg_isr_sched();
    set_pendsv_irq_priority(15);
//  sysTickStart();
}

void SysTick_Handler()
{
    uint32_t t_now;
    uint32_t ready, flag;

    run_time_stack_push_isr(1);

    t_now = sys_timer_get_inline();

    sys_clock += sys_tick_period;
    sys_tick_counter++;

    sys_slot_counter++;
    if (sys_slot_counter == sys_slot_ratio) {
        sys_slot_counter = 0;
        flag = 1;
    } else {
        flag = 0;
    }

    ready = check_sleep_que(t_now);

    if (ready || flag) {
        scheduler();
    }

    run_stat_sys_tick.run_counter++;
    run_stat_sys_tick.time_ttl += run_time_stack_pop_isr(1);
}


// ###########################################################################
// Testing/Verification API
// ###########################################################################

uint32_t timer_stamp[TIMER_STAMP_NUM];
uint32_t timer_stamp_count = 0;

void sysTickTestPush(void)
{
    if (timer_stamp_count < TIMER_STAMP_NUM) {
        timer_stamp[timer_stamp_count] = sys_timer_get_inline();
        timer_stamp_count++;
    }
}

void sysTimerTest(void)
{
    uint32_t time;

    time = sys_timer_get_inline();
    printf("-- the time is %lu\r\n", time);

    printf("Test Timer -- %lu\r\n", timer_stamp_count);
    uint32_t n, diff;

    for (n = 0; n < timer_stamp_count; ++n) {
        if (n == 0) {
            diff = 0;
            } else {
            diff = timer_stamp[n] - timer_stamp[n - 1];
        }
        printf("... %lu - %lu (%lu)\r\n", n, timer_stamp[n], diff);
    }

    volatile uint32_t time_A, time_B;
    time_A = sys_timer_get_inline();
    time_B = sys_timer_get_inline();
    printf("... time_A: %lu, time_B: %lu\r\n", time_A, time_B);
}


static void sysTickTestDummyDelay(uint32_t count)
{
    volatile uint32_t n = 0;
    while (n++ < count);
}


#define TEST_SAMPLE_COUNT   10

void sysTickTest(void)
{
    uint32_t n, cnt;
    uint32_t tick[TEST_SAMPLE_COUNT];
    uint32_t curt[TEST_SAMPLE_COUNT];
    uint32_t delay;

    delay = 10000;
    for (n = cnt = 0; n < TEST_SAMPLE_COUNT; ++n) {
        sysTickTestDummyDelay(delay);
        tick[n] = sys_tick_counter;
        curt[n] = SysTick->VAL;
    }

    printf("Test SysTick: %lu (%lu)\r\n", n, cnt);
    printf("-- CTRL: 0x%08lx, LOAD: %lu, CALIB: %lu\r\n",
    SysTick->CTRL, SysTick->LOAD, SysTick->CALIB);
    for (n = 0; n < TEST_SAMPLE_COUNT; n++) {
        printf("... %lu - tick %lu, current %lu\r\n", n, tick[n], curt[n]);
    }
}
