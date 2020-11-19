/*
 * task_monitor.c
 *
 * Created on: Nov 7, 2015
 *     Author: wayne
 */ 

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sys_core/sys_ticks.h"
#include "kernel/task.h"
#include "kernel/task_util.h"
#include "kernel/timer.h"
#include "kernel/run_stat.h"
#include "kernel/sem.h"
#include "application/task_monitor.h"
#include "sys_device/dev_board.h"
#include "sys_device/dev_uart.h"
#include "examples/task_example.h"

#define BENCHMARK_MONIT    0    // control the monitor benchmark


static void monitor_display_ctx_stat(void);
static void monitor_task_stack_usage(void);
static void monitor_display_task_stack(void);


#if BENCHMARK_MONIT
static uint32_t t_bench_mon_avrg = 0;              // ....  9 tasks + 3 ISR
static uint32_t t_bench_mon_max = 0;               // 2323
static uint32_t t_bench_mon_min = (uint32_t) -1;   // 2314
#endif // BENCHMARK_MONIT

uint32_t task_id_monitor;
uint32_t task_id_monitor_display;


void monitor_task(void *arg)
{
	timer_obj_t  timer_mon;
#if BENCHMARK_MONIT
	volatile uint32_t  t_samp, t_diff;
#endif // BENCHMARK_MONIT
	uint32_t task_id_mon_disp;
	run_stat_que_t *run_stat_que_mon_disp;

	task_id_mon_disp = (uint32_t) arg;
	run_stat_que_mon_disp = run_stat_get_que_task(task_id_mon_disp);

	timer_init(&timer_mon, 1000);
	while (1) {
		timer_wait(&timer_mon);

#if BENCHMARK_MONIT
		t_samp = sys_timer_get_inline();
#endif // BENCHMARK_SCHED
		board_toggle_led();

		run_stat_update();
		if (cli_task_root_stack_check_enable) {
			monitor_task_stack_usage();
		}

		if (run_stat_que_mon_disp->time_dlt == 0) {
			task_set_priority(task_id_mon_disp, (PRIORITY_HIGHEST - 1));
		} else {
			task_resume(task_id_mon_disp);
		}

#if BENCHMARK_MONIT
		t_diff = sys_timer_get_inline() - t_samp;
		if (t_diff > t_bench_mon_max)
			t_bench_mon_max = t_diff;
		if (t_diff < t_bench_mon_min)
			t_bench_mon_min = t_diff;
		t_bench_mon_avrg = (t_bench_mon_avrg * 31 + t_diff + 31) >> 5;
#endif // BENCHMARK_MONIT
	}
}


void monitor_display_task(void *arg)
{
	uint32_t my_priority;

	my_priority = task_get_priority_self();

	while (1) {
		task_suspend();

		if (cli_run_stat_root_load_display_enable) {
//			putchar(ASCII_FF);
            putchar('\033');
            putchar('[');
            putchar('2');
            putchar('J');
			monitor_display_ctx_stat();
			run_stat_display();
		}

		if (cli_task_root_stack_check_enable) {
			monitor_display_task_stack();
			cli_task_root_stack_check_enable = 0;
		}
		task_set_priority_self(my_priority);
	}
}


static void monitor_display_ctx_stat(void)
{
	printf("sys_clk: %llu\r\n", sys_clock);
	printf("sys_tck: %lu\r\n", sys_tick_counter);
	printf("tsk_ctx: %ld\r\n", task_ctx_counter);
	putchar('\n');
}


static void monitor_task_stack_usage(void)
{
	task_info_t *task_info;
	uint32_t n;

	for (n = 0; n < task_numb_total; ++n) {
		task_info = &task_info_pool[n];
		task_info->stack_usage = task_stack_check_usage(task_info);
	}
}


static void monitor_display_task_stack(void)
{
	task_info_t *info_p;
	char name_buf[13];
	uint32_t n;

	for (n = 0; n < TASK_NUMB; ++n) {
		info_p = &task_info_pool[n];
		if (info_p->state == TASK_STATE_UNUSED) {
			continue;
		}
		printf("%2d - %s  %4u  %4u\r\n", info_p->id,
		       string_fill_buf(name_buf, sizeof(name_buf), info_p->name),
		       info_p->stack_usage, info_p->stack_size);
	}	
	printf("\r\n");
}

