/*
 * task.h
 *
 *  Created on: Jul 16, 2013
 *      Author: wayne
 */

#ifndef _TASK_H_
#define _TASK_H_


#include <kernel/run_stat.h>
#include "kernel/que.h"


extern uint32_t _estack;


#define PRIORITY_NUMB             16   // can NOT exceed 32 (for NOW)
#define PRIORITY_HIGHEST          (PRIORITY_NUMB - 1)    // highest priority
#define PRIORITY_LOWEST           0                      // lowest priority
#define TASK_FIFO_NUMB            PRIORITY_NUMB

#define TASK_NUMB                 16
#define TASK_ID_MAX               (TASK_NUMB - 1)

#define TASK_STACK_RED_ZONE_SIZE  8  // 8 x 4 byte
#define TASK_STACK_RZ_MAGIC       0xdeadbeef
#define TASK_STACK_SIZE_0_5K       512  // 0.5 KB
#define TASK_STACK_SIZE_1_0K      1024  // 1.0 KB
#define TASK_STACK_SIZE_1_5K      1536  // 1.5 KB
#define TASK_STACK_SIZE_2_0K      2048  // 2.0 KB
#define TASK_STACK_SIZE_2_5K      2560  // 2.5 BB
#define TASK_STACK_SIZE_3_0K      3072  // 3.0 KB
#define TASK_STACK_SIZE_3_5K      3584  // 3.5 KB
#define TASK_STACK_SIZE_4_0K      4096  // 4.0 KB


typedef enum task_state_ {
	TASK_STATE_UNUSED = 0,
	TASK_STATE_TAKEN,
	TASK_STATE_RUN,
	TASK_STATE_READY,
	TASK_STATE_WAIT_SEM,
	TASK_STATE_WAIT_MUTEX,
	TASK_STATE_WAIT_SLEEP,
	TASK_STATE_SUSPEND,
	TASK_STATE_N
} task_state_t;

typedef struct task_info_ {
	que_t         que_task_info;
	uint16_t      id;
	uint16_t      priority;
	const char   *name;
	task_state_t  state;
	uint32_t      wake_up_time;
	uint32_t      stack_base;
	uint16_t      stack_size;
	uint16_t      stack_usage;
    uint32_t      exc_rtn_b4;   // EXC_RETURN[4]
	uint32_t     *sp;
	run_stat_t    run_stat;
} task_info_t __attribute__((aligned(4)));

typedef struct task_fifo_ {
	que_t         que_head_task_info;   // queue head for linking task_info_t
	uint16_t      task_numb;
	uint16_t      priority;
} task_fifo_t __attribute__((aligned(4)));

typedef struct pri_act_map_ {
	uint32_t      act_b_map;
	uint32_t      act_pri_num;
} pri_act_map_t;

typedef struct ready_tree_ {
	pri_act_map_t pri_act_map;
	task_fifo_t   task_fifo[TASK_FIFO_NUMB];
} ready_que_t;


extern task_fifo_t   task_fifo_pool[];
extern task_info_t   task_info_pool[];

extern uint32_t      task_numb_total;

extern uint32_t      task_id_run;
extern task_info_t  *task_info_run_p;

extern uint32_t      task_ctx_counter;

extern const char *task_state_name[TASK_STATE_N];


void task_stack_init(uint32_t *sp, void *func_ptr, uint32_t argv);
void task_stack_init_debug(uint32_t *sp, void *func_ptr, uint32_t argv, uint32_t pfx);
void task_sys_init(uint32_t stack_size);

uint32_t task_create(void *func_ptr, const char *name, uint32_t priority,
                     uint32_t stack_size, void *arg);
void task_suspend(void);
void task_resume(uint32_t task_id);
void task_terminate(void);

uint32_t task_get_priority_self(void);
void task_set_priority_self(uint32_t priority);
void task_set_priority(uint32_t task_id, uint32_t priority);

void task_insert_to_pri_chain(task_fifo_t *task_chain_p);
void task_remove_from_pri_chain(task_fifo_t *task_chain_p);
task_fifo_t *task_get_highest_from_pri_chain(void);

task_fifo_t *task_enque_to_task_fifo(task_info_t *task_info_p);
void task_remove_from_task_fifo(task_info_t *task_info_p);
task_info_t *task_deque_from_task_fifo(task_fifo_t *task_chain_p);
task_info_t *task_get_1st_from_task_fifo(task_fifo_t *task_chain_p);

void task_switch(task_info_t *task_info_next_p);

// void task_cpu_load_request(void);
// void task_cpu_load_stat(void);

uint32_t task_stack_check_magic(task_info_t *info_p, uint32_t words);
uint32_t task_stack_check_usage(task_info_t *info_p);

void task_fifo_dump(task_fifo_t *fifo);
void task_fifo_dump_all(void);
void task_info_dump(task_info_t *task_p, int event);
void task_info_dump_all(void);
void task_info_summary(void);
void task_dump_pri_act_map(void);


#endif /* _TASK_H_ */
