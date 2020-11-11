/*
 * sem.c
 *
 *  Created on: Sep 24, 2013
 *      Author: wayne
 */

#include <stdint.h>

#include "sys_core/sys_timer.h"
#include "sys_core/sys_util.h"
#include "kernel/que.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "kernel/sem.h"
#include "utility/bits_op_tool.h"


#define BENCHMARK_SEM     0


void sem_bench_stat_collect(uint32_t val, uint32_t *max_p, uint32_t *min_p, uint32_t *avrg_p)
{
	if (val > *max_p)
		*max_p = val;
	if (val < *min_p)
		*min_p = val;
	*avrg_p = (*avrg_p * 31 + val + 31) >> 5;
}


void sem_init(sem_obj_t *sem_p)
{
	task_fifo_t   *task_fifo_p;

	task_fifo_p = &sem_p->task_fifo;

	cpu_irq_enter_critical();    // __disable_irq();

	sem_p->sem_count = 0;

	que_init(&task_fifo_p->que_head_task_info);
	task_fifo_p->task_numb = 0;
	task_fifo_p->priority = 0;

	cpu_irq_leave_critical();    // __enable_irq();
}


#if BENCHMARK_SEM
uint32_t t_bench_sem_post_avrg = 0;
uint32_t t_bench_sem_post_max = 0;
uint32_t t_bench_sem_post_min = (uint32_t) -1;
#endif // BENCHMARK_SEM

void sem_post(sem_obj_t *sem_p)
{
	task_info_t   *task_info_p;
	task_fifo_t   *task_fifo_p;
#if BENCHMARK_SEM
	volatile uint32_t  t_samp, t_diff;
#endif // BENCHMARK_SEM

	cpu_irq_enter_critical();    // __disable_irq();
#if BENCHMARK_SEM
	t_samp = sys_timer_get_inline();
#endif // BENCHMARK_SEM

	sem_p->sem_count++;

	task_fifo_p = &sem_p->task_fifo;
	if (task_fifo_p->task_numb == 0) {
#if BENCHMARK_SEM
		t_diff = sys_timer_get_inline() - t_samp;
		sem_bench_stat_collect(t_diff,
					&t_bench_sem_post_max, &t_bench_sem_post_min, &t_bench_sem_post_avrg);
#endif // BENCHMARK_SEM
		cpu_irq_leave_critical();    // __enable_irq();
		return;
	}

	// remove the 1st task in sema wait FIFO
	task_fifo_p->task_numb--;
	task_info_p = (task_info_t *) que_deque(&task_fifo_p->que_head_task_info);

	// put the task to ready tree
	task_info_p->state = TASK_STATE_READY;
	(void) task_enque_to_task_fifo(task_info_p);

#if BENCHMARK_SEM
	t_diff = sys_timer_get_inline() - t_samp;
	sem_bench_stat_collect(t_diff,
				&t_bench_sem_post_max, &t_bench_sem_post_min, &t_bench_sem_post_avrg);
#endif // BENCHMARK_SEM
	cpu_irq_leave_critical();    // __enable_irq();

	scheduler();
}


#if BENCHMARK_SEM
uint32_t t_bench_sem_wait_avrg = 0;
uint32_t t_bench_sem_wait_max = 0;
uint32_t t_bench_sem_wait_min = (uint32_t) -1;
uint32_t t_bench_sem_wait_pend_count = 0;
#endif // BENCHMARK_SEM

void sem_wait(sem_obj_t *sem_p)
{
	task_fifo_t   *task_fifo_p;
#if BENCHMARK_SEM
	volatile uint32_t  t_samp, t_diff;
#endif // BENCHMARK_SEM

	cpu_irq_enter_critical();    // __disable_irq();

	if (task_id_run == 0) {
		cpu_irq_leave_critical();    // __enable_irq();
		return;   // never allow main/IDLE-loop try semaphore
	}

#if BENCHMARK_SEM
	t_samp = sys_timer_get_inline();
#endif // BENCHMARK_SEM

	sem_p->sem_count--;
	if (sem_p->sem_count >= 0) {
#if BENCHMARK_SEM
		t_diff = sys_timer_get_inline() - t_samp;
		sem_bench_stat_collect(t_diff,
					&t_bench_sem_wait_max, &t_bench_sem_wait_min, &t_bench_sem_wait_avrg);
#endif // BENCHMARK_SEM
		cpu_irq_leave_critical();    // __enable_irq();

	} else {
		task_info_run_p->state = TASK_STATE_WAIT_SEM;

		// put task into sema wait FIFO
		task_fifo_p = &sem_p->task_fifo;
		que_enque(&task_fifo_p->que_head_task_info, &task_info_run_p->que_task_info);
		task_fifo_p->task_numb++;

#if BENCHMARK_SEM
		t_diff = sys_timer_get_inline() - t_samp;
		sem_bench_stat_collect(t_diff,
					&t_bench_sem_wait_max, &t_bench_sem_wait_min, &t_bench_sem_wait_avrg);
		t_bench_sem_wait_pend_count++;
#endif // BENCHMARK_SEM
		cpu_irq_leave_critical();    // __enable_irq();

		scheduler();
	}
}
