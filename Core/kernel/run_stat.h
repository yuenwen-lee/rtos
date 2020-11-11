/*
 * run_time_util.h
 *
 *  Created on: Feb 4, 2017
 *      Author: wayne
 */

#ifndef _SRC_KERNEL_RUN_STAT_H_
#define _SRC_KERNEL_RUN_STAT_H_

#include <stdint.h>
#include "kernel/que.h"


#define RUN_TIME_STACK_MAX_LEVEL   8    // maximum 8 (+ 1) level


typedef struct run_time_tag_s {
	uint32_t  t_stamp;
	uint32_t  t_accum;
} run_time_stack_t;

typedef struct run_stat_s {
	uint64_t  time_ready;      // last time when task state changes to "TASK_STATE_RUN"
	uint64_t  time_ttl;        // total run time (CPU cycles)
	uint32_t  run_counter;     // task -> in TASK_STATE_RUN, ISR -> number called
} run_stat_t;

typedef struct run_stat_que_s {
	que_t        link;
	uint32_t     id;
	const char  *name;
	run_stat_t  *stat_p;
	uint64_t     time_ttl_prev;   // total run time last snapshot
	uint32_t     time_dlt;
	uint32_t     time_avg;        // average run time
	uint32_t     run_counter_prev;
	uint32_t     run_counter_dlt;
	uint32_t     cpu_load;
} run_stat_que_t __attribute__((aligned(4)));


extern uint32_t sys_cpu_load;
extern que_t  run_stat_que_task;
extern que_t  run_stat_que_isr;
/* flag that control displaying task/isr cpu load in display task */
extern uint32_t cli_run_stat_root_load_display_enable;


void run_time_set_task(uint32_t t_now);
uint32_t run_time_get_task(uint32_t t_now);

void run_time_sche_start(void);
void run_time_sche_end(run_stat_t *task_stat, run_stat_t *sche_stat);

void run_time_stack_push_isr(int interrupt_disable);
uint32_t run_time_stack_pop_isr(int interrupt_disable);

void run_stat_register_task(const char *name, uint32_t task_id, run_stat_t *stat);
void run_stat_register_isr(const char *name, uint32_t isr_id, run_stat_t *stat, run_stat_que_t *stat_q);

run_stat_que_t *run_stat_get_que_task(uint32_t task_id);
run_stat_que_t *run_stat_get_que_isr(uint32_t isr_id);

void run_stat_init(void);
void run_stat_update(void);
void run_stat_display_new(int v_pos);
void run_stat_display(void);

void run_stat_reg_stck_ov(void);

void cli_run_stat_init(void);

static inline void cli_cb_run_stat_root_load_stop(void)
{
	/* stop displaying task/isr cpu load in display task */
	cli_run_stat_root_load_display_enable = 0;
}


#endif /* _SRC_KERNEL_RUN_STAT_H_ */
