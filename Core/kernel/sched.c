/*
 * scheduler.c
 *
 * Created: 10/9/2020 5:25:13 PM
 *  Author: Wayne Lee
 */ 

#include <stdint.h>
#include <stdio.h>

#include "sys_core/sys_ticks.h"
#include "sys_core/sys_timer.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "kernel/run_stat.h"


#define SCHED_STACK_CHECK  0    // control the scheduler benchmark
#define BENCHMARK_SCHED    0    // control the scheduler benchmark


run_stat_que_t run_stat_que_sched;
run_stat_t run_stat_sched;

#if BENCHMARK_SCHED
uint32_t t_bench_schd_avrg = 0;              // 518
uint32_t t_bench_schd_max = 0;               // 644
uint32_t t_bench_schd_min = (uint32_t) -1;   // 237
#endif // BENCHMARK_SCHED


uint32_t scheduler_core(uint32_t exc_turn)
{
	// NOTE: API is called by PendSV_Handler (task_ctx.S) and is encapsulated by cpsid/cpsie

	task_fifo_t  *task_fifo_next_p;
	task_info_t  *task_info_next_p;
	task_info_t  *task_info_p;
	uint32_t ctx_flag = 0;
    
#if BENCHMARK_SCHED
	volatile uint32_t  t_samp, t_diff;
#endif // BENCHMARK_SCHED

#if BENCHMARK_SCHED
	t_samp = sys_timer_get_inline();
#endif // BENCHMARK_SCHED
	run_time_sche_start();

#if SCHED_STACK_CHECK
	// check stack overflow
	uint32_t err_count;
	err_count = task_stack_check_magic(task_info_run_p,
	                                   TASK_STACK_RED_ZONE_SIZE);
	if (err_count != 0) {
		schedule_stack_ov(err_count);
	}
#endif // SCHED_STACK_CHECK
	task_info_p = task_info_run_p;

	// find the candidate task
	task_fifo_next_p = task_get_highest_from_pri_chain();
	if (task_fifo_next_p == (task_fifo_t *) 0) {
		// no task is in TASK_STATE_READY
		goto SCHEDULER_EXIT;
	}
	task_info_next_p = task_get_1st_from_task_fifo(task_fifo_next_p);

	if (task_info_run_p->state == TASK_STATE_RUN) {
		if (task_info_next_p->priority < task_info_run_p->priority) {
			goto SCHEDULER_EXIT;
		}
		// put the present running task back to ready tree
		task_info_run_p->state = TASK_STATE_READY;
		(void) task_enque_to_task_fifo(task_info_run_p);
	}

	// remove the candidate task from ready tree
	(void) task_deque_from_task_fifo(task_fifo_next_p);
	task_info_next_p->state = TASK_STATE_RUN;
    ctx_flag = 1;

SCHEDULER_EXIT:
	if (ctx_flag) {
        uint32_t both_exc_b4;
        task_info_p->exc_rtn_b4 = get_exc_return_b4(exc_turn);
        both_exc_b4 = ((task_info_p->exc_rtn_b4) << 1) | task_info_next_p->exc_rtn_b4;
        ctx_flag |= both_exc_b4 << 16;
		task_switch(task_info_next_p);
	}
	run_stat_sched.run_counter++;
	run_time_sche_end(&task_info_p->run_stat, &run_stat_sched);

#if BENCHMARK_SCHED
	t_diff = sys_timer_get_inline() - t_samp;
	if (t_diff > t_bench_schd_max)
		t_bench_schd_max = t_diff;
	if (t_diff < t_bench_schd_min)
		t_bench_schd_min = t_diff;
	t_bench_schd_avrg = (t_bench_schd_avrg * 31 + t_diff + 31) >> 5;
#endif // BENCHMARK_SCHED

	return ctx_flag;
}


void schedule_stack_ov(uint32_t err_count)
{
	schedule_halt();

	printf("!!!! Stack Overflow (%lu) !!!\r\n", err_count);
	task_info_dump(task_info_run_p, 0);
	while (1)
		;
}


void run_stat_reg_isr_sched(void)
{
	run_stat_register_isr("sched", 0, &run_stat_sched, &run_stat_que_sched);
}
