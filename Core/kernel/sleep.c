/*
 * sleep.c
 *
 *  Created on: Aug 27, 2013
 *      Author: wayne
 */ 

#include "stdint.h"
#include <stdio.h>

#include "sys_core/sys_timer.h"
#include "sys_core/sys_ticks.h"
#include "sys_core/sys_util.h"
#include "kernel/que.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "kernel/sleep.h"


sleep_que_t sleep_que;


void sleep_init(void)
{
	sleep_que.latest_wake_up_time = 0;
	sleep_que.numb = 0;
	que_init(&sleep_que.que_head_task_info);
}


void sleep_core (uint32_t wake_up_time)
{
	que_t     *que_head_p;
	que_t     *que_p;

	if (task_id_run == 0)
		return;   // never allow main/IDLE-loop task sleep

	cpu_irq_enter_critical();    // __disable_irq();

	task_info_run_p->wake_up_time = wake_up_time;
	task_info_run_p->state = TASK_STATE_WAIT_SLEEP;

	que_head_p = &sleep_que.que_head_task_info;
	que_p = que_head_p->next;
	while (que_p != que_head_p) {
		if ((int32_t) (wake_up_time - ((task_info_t *) que_p)->wake_up_time) < 0)
			break;
		que_p = que_p->next;
	}

	que_insert_before(que_p, &task_info_run_p->que_task_info);
	if (que_head_p->next == (que_t *) task_info_run_p)
		sleep_que.latest_wake_up_time = wake_up_time;
	sleep_que.numb++;

	cpu_irq_leave_critical();    // __enable_irq();

	scheduler();
}


void sleep_msec (uint32_t msec)
{
	uint32_t   wake_up_time;

	wake_up_time = sys_timer_get_inline() + msec_to_cpu_tick(msec);
	sleep_core(wake_up_time);
}


void delay_usec (uint32_t usec)
{
    volatile uint32_t timeup, time;

    timeup = sys_timer_get_inline() + usec_to_cpu_tick(usec);
    do {
        time = sys_timer_get_inline();
    } while ((int32_t) (timeup - time) > 0);
}


/* ********************************************************* */
/* ********************************************************* */
#define BENCHMARK_SLEEP  0    // control the sleep benchmark


#if BENCHMARK_SLEEP
uint32_t t_bench_sleep_avrg = 0;              //  45
uint32_t t_bench_sleep_max = 0;               // 198
uint32_t t_bench_sleep_min = (uint32_t) -1;   //  14
#endif // BENCHMARK_SLEEP


uint32_t check_sleep_que(uint32_t time_now)
{
	que_t       *que_head_p;
	que_t       *que_p;
	task_info_t *task_info_p;
	uint32_t     ready;
#if BENCHMARK_SLEEP
	volatile uint32_t  t_samp, t_diff;
#endif // BENCHMARK_SLEEP

	cpu_irq_enter_critical();    // __disable_irq();

#if BENCHMARK_SLEEP
	t_samp = sys_timer_get_inline();
#endif // BENCHMARK_SLEEP

	ready = 0;
	if (sleep_que.numb == 0 ||
		((int32_t) (sleep_que.latest_wake_up_time - time_now) > 0)) {
		goto CHECK_SLEEP_QUE_EXIT;
	}

	que_head_p = &sleep_que.que_head_task_info;
	que_p = que_head_p->next;
	while (que_p != que_head_p) {

		if ((int32_t) (((task_info_t *) que_p)->wake_up_time - time_now) > 0)
			break;
		sleep_que.numb--;
		task_info_p = (task_info_t *) que_deque(que_head_p);

		// put the task to ready tree
		task_info_p->state = TASK_STATE_READY;
		(void) task_enque_to_task_fifo(task_info_p);
		ready++;

		que_p = que_head_p->next;
	}

	if (sleep_que.numb) {
		sleep_que.latest_wake_up_time = ((task_info_t *) que_p)->wake_up_time;
	}

CHECK_SLEEP_QUE_EXIT:
#if BENCHMARK_SLEEP
	t_diff = sys_timer_get_inline() - t_samp;
	if (t_diff > t_bench_sleep_max)
		t_bench_sleep_max = t_diff;
	if (t_diff < t_bench_sleep_min)
		t_bench_sleep_min = t_diff;
	t_bench_sleep_avrg = (t_bench_sleep_avrg * 31 + t_diff + 31) >> 5;
#endif // BENCHMARK_SLEEP

	cpu_irq_leave_critical();    // __enable_irq();
	return ready;
}


void sleep_que_dump(void)
{
	printf("=== Sleep Que Info ===\r\n");
	printf("  <self ptr>     : %p\r\n", &sleep_que);
	printf("  earliest wakeup: %lu\r\n", sleep_que.latest_wake_up_time);
	printf("  que_head       : %p\r\n", &sleep_que.que_head_task_info);
	printf("  task numb      : %lu\r\n", sleep_que.numb);

	que_t *task_info;
	task_info = sleep_que.que_head_task_info.next;
	while (task_info != &sleep_que.que_head_task_info) {
		task_info_dump((task_info_t *) task_info, 0);
		task_info = task_info->next;
	}
}
