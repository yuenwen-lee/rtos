/*
 * task.c
 *
 * Created: 10/11/2020 4:59:56 PM
 *  Author: yuenw
 */

#include <stdint.h>
#include <stdio.h>

#include "sys_core/sys_ticks.h"
#include "sys_core/sys_timer.h"
#include "sys_core/sys_util.h"
#include "kernel/que.h"
#include "kernel/task.h"
#include "kernel/task_util.h"
#include "kernel/task_ctx.h"
#include "kernel/sleep.h"
#include "kernel/sched.h"
#include "kernel/run_stat.h"
#include "utility/bits_op_tool.h"


pri_act_map_t pri_act_map;

uint32_t      task_next_stack_start = ((uint32_t) &_estack);

task_fifo_t   task_fifo_pool[TASK_FIFO_NUMB];
task_info_t   task_info_pool[TASK_NUMB];

uint32_t      task_numb_total;    // active task number

uint32_t      task_id_run;
task_info_t  *task_info_run_p;

uint32_t      task_ctx_counter;   // maintained for the ctx
uint32_t    **task_sp_ptr_now;    // used for the ctx
uint32_t    **task_sp_ptr_next;   // used for the ctx

const char *task_state_name[TASK_STATE_N] = {
	"UNUSED",
	"TAKEN",
	"RUN",
	"READY",
	"W_SEM",
	"W_MUTEX",
	"W_SLEEP",
	"SUSPEND",
};


static uint32_t task_stack_base(uint32_t stack_size)
{
	uint32_t stack_ptr = task_next_stack_start;
	task_next_stack_start -= stack_size;
	return stack_ptr;
}


static void task_stack_fill_magic(uint32_t base, uint32_t size)
{
	uint32_t *ptr = (uint32_t *) (base - size);
	uint32_t  words = size / sizeof(uint32_t);
	for (uint32_t n = 0; n < words; ++n) {
		ptr[n] = TASK_STACK_RZ_MAGIC;
	}
}


uint32_t task_stack_check_magic(task_info_t *info_p, uint32_t words)
{
	uint32_t *ptr = (uint32_t *) (info_p->stack_base - info_p->stack_size);
	uint32_t  fail_count = 0;
	for (uint32_t n = 0; n < words; ++n) {
		if (ptr[n] != TASK_STACK_RZ_MAGIC) {
			fail_count++;
		}
	}
	return fail_count;
}


uint32_t task_stack_check_usage(task_info_t *info_p)
{
	uint32_t  *ptr, words, n;

	ptr = (uint32_t *) (info_p->stack_base - info_p->stack_size);
	words = info_p->stack_size / sizeof(uint32_t);
	for (n = 0; n < words; ++n) {
		if (ptr[n] != TASK_STACK_RZ_MAGIC) {
			break;
		}
	}
	return (info_p->stack_size - (n * sizeof(uint32_t)));
}


void task_stack_init(uint32_t *sp, void *func_ptr, uint32_t argv)
{
	sp[ 0] = 0x04040404L;                    // r4
	sp[ 1] = 0x05050505L;                    // r5
	sp[ 2] = 0x06060606L;                    // r6
	sp[ 3] = 0x07070707L;                    // r7
	sp[ 4] = 0x08080808L;                    // r8
	sp[ 5] = 0x09090909L;                    // r9
	sp[ 6] = 0x10101010L;                    // r10
	sp[ 7] = 0x11111111L;                    // r11

	sp[ 8] = argv;                           // r0
	sp[ 9] = 0x01010101L;                    // r1
	sp[10] = 0x02020202L;                    // r2
	sp[11] = 0x03030303L;                    // r3

	sp[12] = 0x12121212L;                    // r12
	sp[13] = 0x01000000L;                    // r14 (lr)
	sp[14] = ((unsigned int) func_ptr) | 1;  // pc
	sp[15] = 0x01000000L;                    // xPSR
}


void task_sys_init(uint32_t stack_size)
{
	uint32_t       task_id;
	task_fifo_t   *task_fifo_p;
	task_info_t   *task_info_p;
	uint32_t       stack_p;
	uint32_t       n;

	// pri_act_map init
	pri_act_map.act_b_map  = 0;
	pri_act_map.act_pri_num = 0;

	// task_fifo_pool init
	for (n = 0; n < TASK_FIFO_NUMB; ++n) {
		task_fifo_p = &task_fifo_pool[n];
		task_fifo_p->priority = n;
		que_init(&task_fifo_p->que_head_task_info);
		task_fifo_p->task_numb = 0;
	}

	// task_info_pool init
	task_numb_total = 0;
	for (n = 0; n < TASK_NUMB; ++n) {
		task_info_p = &task_info_pool[n];
		task_info_p->id    = n;
		task_info_p->state = TASK_STATE_UNUSED;
	}

	// init the system sleep queue ...
	sleep_init();

	// Init the CLI command for the task
	cli_task_init();

	// Setup the the very 1st task, the main (also the idle loop) task
	task_id = 0;
	task_numb_total++;

	task_info_p = &task_info_pool[task_id];
	task_info_p->id            = task_id;
	task_info_p->state         = TASK_STATE_RUN;
	task_info_p->priority      = PRIORITY_LOWEST;
	task_info_p->name          = "Idle_Loop";
	task_info_p->stack_base    = task_stack_base(stack_size);
	task_info_p->stack_size    = stack_size;
	task_info_p->sp            = (uint32_t *) task_info_p->stack_base;

	// fill the stack with magic pattern
	stack_p = get_stack_ptr();
	task_stack_fill_magic(stack_p, task_info_p->stack_size - (task_info_p->stack_base - stack_p));

	task_id_run = task_id;
	task_info_run_p = task_info_p;

	run_stat_init();
	run_stat_register_task(task_info_p->name, task_id, &task_info_p->run_stat);
	run_time_set_task(sys_timer_get_inline());
}


uint32_t task_create(void *func_ptr, const char *name, uint32_t priority, uint32_t stack_size, void *arg)
{
	task_info_t   *task_info_p;
	uint32_t       task_id;

	cpu_irq_enter_critical();    // __disable_irq();
	for (task_id = 0; task_id < TASK_NUMB; ++task_id) {
		task_info_p = &task_info_pool[task_id];
		if (task_info_p->state == TASK_STATE_UNUSED) {
			task_info_p->state = TASK_STATE_TAKEN;
			task_numb_total++;
			break;
		}
	}
	cpu_irq_leave_critical();    // __enable_irq();

	if (task_id == TASK_NUMB) {
		return (uint32_t) -1;
	}

	if (priority > PRIORITY_HIGHEST)
		priority = PRIORITY_HIGHEST;

	task_info_p = &task_info_pool[task_id];
	task_info_p->priority   = priority;
	task_info_p->name       = name;
	task_info_p->stack_base = task_stack_base(stack_size);
	task_info_p->stack_size = stack_size;
	task_info_p->sp         = (uint32_t *) (task_info_p->stack_base - (16 * sizeof(uint32_t)));

	// fill the stack with magic pattern
	task_stack_fill_magic(task_info_p->stack_base, task_info_p->stack_size);
	task_stack_init(task_info_p->sp, func_ptr, (uint32_t) arg);

	cpu_irq_enter_critical();    // __disable_irq();
	// put the task to ready tree
	task_info_p->state = TASK_STATE_READY;
	(void) task_enque_to_task_fifo(task_info_p);
	run_stat_register_task(task_info_p->name, task_id, &task_info_p->run_stat);
	cpu_irq_leave_critical();    // __enable_irq();

	return task_id;
}


/* ********************************************************* */
/* ********************************************************* */

void task_suspend(void)
{
	cpu_irq_enter_critical();    // __disable_irq();

	if (task_id_run == 0) {
		// never allow main/IDLE-loop task suspends itself
		cpu_irq_leave_critical();    // __enable_irq();
		return;
	}

	task_info_run_p->state = TASK_STATE_SUSPEND;
	cpu_irq_leave_critical();    // __enable_irq();

	scheduler();
}


void task_resume(uint32_t task_id)
{
	task_info_t   *task_info_p;

	cpu_irq_enter_critical();    // __disable_irq();

	if (task_id == task_id_run) {
		// resume yourself?
		cpu_irq_leave_critical();    // __enable_irq();
		return;
	}

	task_info_p = &task_info_pool[task_id];
	if (task_info_p->state != TASK_STATE_SUSPEND) {
		// can only put task in SUSPEND state back to READY
		cpu_irq_leave_critical();    // __enable_irq();
		return;
	}
	// put the task to ready tree
	task_info_p->state = TASK_STATE_READY;
	(void) task_enque_to_task_fifo(task_info_p);

	cpu_irq_leave_critical();    // __enable_irq();

	scheduler();
}


void task_terminate(void)
{
	cpu_irq_enter_critical();    // __disable_irq();

	if (task_id_run == 0) {
		// never allow IDLE loop task suspends itself
		cpu_irq_leave_critical();    // __enable_irq();
		return;
	}
	task_info_run_p->state = TASK_STATE_UNUSED;
	task_numb_total--;

	cpu_irq_leave_critical();    // __enable_irq();

	scheduler();
}


/* ********************************************************* */
/* ********************************************************* */

uint32_t task_get_priority_self(void)
{
	uint32_t priority;

	cpu_irq_enter_critical();    // __disable_irq();
	priority = task_info_run_p->priority;
	cpu_irq_leave_critical();    // __enable_irq();

	return(priority);
}


void task_set_priority_self(uint32_t priority)
{
	if (priority > PRIORITY_HIGHEST) {
		priority = PRIORITY_HIGHEST;
	}
	cpu_irq_enter_critical();    // __disable_irq();
	task_info_run_p->priority = priority;
	cpu_irq_leave_critical();    // __enable_irq();
}


void task_set_priority(uint32_t task_id, uint32_t pri_new)
{
	task_info_t  *task_info_p;

	task_info_p = &task_info_pool[task_id];
	if (pri_new > PRIORITY_HIGHEST)
		pri_new = PRIORITY_HIGHEST;

	cpu_irq_enter_critical();    // __disable_irq();

	if (task_info_p->state == TASK_STATE_READY) {
		task_remove_from_task_fifo(task_info_p);
		task_info_p->state = TASK_STATE_SUSPEND;
	}

	task_info_p->priority = pri_new;

	if (task_info_p->state == TASK_STATE_SUSPEND) {
		// put the task to ready tree
		task_info_p->state = TASK_STATE_READY;
		(void) task_enque_to_task_fifo(task_info_p);
	}

	cpu_irq_leave_critical();    // __enable_irq();
}


/* ********************************************************* */
/* ********************************************************* */
/* The following APIs assume that the caller already disable */
/* the IRQ                                                   */
/* ********************************************************* */
/* ********************************************************* */

/* ********************************************************* */
/* ********************************************************* */
#define BENCHMARK_PRI_CHAIN_OP  0    // control the scheduler benchmark

#if BENCHMARK_PRI_CHAIN_OP
uint32_t t_bench_insert_pri_chain_avrg = 0;             // 13
uint32_t t_bench_insert_pri_chain_max = 0;              // 13
uint32_t t_bench_insert_pri_chain_min = (uint32_t) -1;  // 13
#endif // BENCHMARK_PRI_CHAIN_OP

void task_insert_to_pri_chain(task_fifo_t *task_fifo_p)
{
#if BENCHMARK_PRI_CHAIN_OP
	volatile uint32_t  t_samp, t_diff;

	t_samp = sys_timer_get_inline();
#endif // BENCHMARK_PRI_CHAIN_OP
	uint32_t   bit_loc;

	bit_loc = (task_fifo_p->priority) & 0x1F;   // priority % 32
	pri_act_map.act_b_map |= (1 << bit_loc);
	pri_act_map.act_pri_num++;

#if BENCHMARK_PRI_CHAIN_OP
	t_diff = sys_timer_get_inline() - t_samp;
	if (t_diff > t_bench_insert_pri_chain_max)
		t_bench_insert_pri_chain_max = t_diff;
	if (t_diff < t_bench_insert_pri_chain_min)
		t_bench_insert_pri_chain_min = t_diff;
	t_bench_insert_pri_chain_avrg = (t_bench_insert_pri_chain_avrg * 31 + t_diff + 31) >> 5;
#endif // BENCHMARK_PRI_CHAIN_OP
}


/* ********************************************************* */
/* ********************************************************* */
#if BENCHMARK_PRI_CHAIN_OP
uint32_t t_bench_remove_pri_chain_avrg = 0;             // 13
uint32_t t_bench_remove_pri_chain_max = 0;              // 13
uint32_t t_bench_remove_pri_chain_min = (uint32_t) -1;  // 13
#endif // BENCHMARK_PRI_CHAIN_OP

void task_remove_from_pri_chain(task_fifo_t *task_fifo_p)
{
#if BENCHMARK_PRI_CHAIN_OP
	volatile uint32_t  t_samp, t_diff;

	t_samp = sys_timer_get_inline();
#endif // BENCHMARK_PRI_CHAIN_OP
	uint32_t   bit_loc;

	bit_loc = task_fifo_p->priority & 0x1F;   // priority % 32
	pri_act_map.act_b_map &= ~(1 << bit_loc);
	pri_act_map.act_pri_num--;

#if BENCHMARK_PRI_CHAIN_OP
	t_diff = sys_timer_get_inline() - t_samp;
	if (t_diff > t_bench_remove_pri_chain_max)
		t_bench_remove_pri_chain_max = t_diff;
	if (t_diff < t_bench_remove_pri_chain_min)
		t_bench_remove_pri_chain_min = t_diff;
	t_bench_remove_pri_chain_avrg = (t_bench_remove_pri_chain_avrg * 31 + t_diff + 31) >> 5;
#endif // BENCHMARK_PRI_CHAIN_OP
}


task_fifo_t *task_get_highest_from_pri_chain(void)
{
	uint32_t  map;
	uint32_t  priority;

	map = pri_act_map.act_b_map;
	if (map) {
		priority = find_msb_loc_fast(map);
		return &task_fifo_pool[priority];
	} else {
		return (task_fifo_t *) 0;
	}
}


/* ********************************************************* */
/* ********************************************************* */
#define BENCHMARK_TASK_CHAIN_OP  0    // control the scheduler benchmark

#if BENCHMARK_TASK_CHAIN_OP
uint32_t t_bench_enque_task_chain_avrg = 0;             // 57
uint32_t t_bench_enque_task_chain_max = 0;              // 86
uint32_t t_bench_enque_task_chain_min = (uint32_t) -1;  // 26
#endif // BENCHMARK_TASK_CHAIN_OP

task_fifo_t *task_enque_to_task_fifo(task_info_t *task_info_p)
{
	task_fifo_t   *task_fifo_p;
	uint32_t       priority;
#if BENCHMARK_TASK_CHAIN_OP
	volatile uint32_t  t_samp, t_diff;
#endif // BENCHMARK_TASK_CHAIN_OP

#if BENCHMARK_TASK_CHAIN_OP
	t_samp = sys_timer_get_inline();
#endif // BENCHMARK_TASK_CHAIN_OP

	priority = task_info_p->priority;
	task_fifo_p = &task_fifo_pool[priority];

	que_enque(&task_fifo_p->que_head_task_info, &task_info_p->que_task_info);
	task_fifo_p->task_numb++;

	if (task_fifo_p->task_numb == 1)
		task_insert_to_pri_chain(task_fifo_p);

#if BENCHMARK_TASK_CHAIN_OP
	t_diff = sys_timer_get_inline() - t_samp;
	if (t_diff > t_bench_enque_task_chain_max)
		t_bench_enque_task_chain_max = t_diff;
	if (t_diff < t_bench_enque_task_chain_min)
		t_bench_enque_task_chain_min = t_diff;
	t_bench_enque_task_chain_avrg = (t_bench_enque_task_chain_avrg * 31 + t_diff + 31) >> 5;
#endif // BENCHMARK_TASK_CHAIN_OP

	return task_fifo_p;
}


void task_remove_from_task_fifo(task_info_t *task_info_p)
{
	// NOTE: API assume the task is in READY queue
	task_fifo_t   *task_fifo_p;

	task_fifo_p = &task_fifo_pool[task_info_p->priority];
	que_remove(&task_info_p->que_task_info);

	task_fifo_p->task_numb--;
	if (task_fifo_p->task_numb == 0)
		task_remove_from_pri_chain(task_fifo_p);
}


/* ********************************************************* */
/* ********************************************************* */
#if BENCHMARK_TASK_CHAIN_OP
uint32_t t_bench_deque_task_chain_avrg = 0;              // 58
uint32_t t_bench_deque_task_chain_max = 0;               // 91
uint32_t t_bench_deque_task_chain_min = (uint32_t) -1;   // 27
#endif // BENCHMARK_TASK_CHAIN_OP

task_info_t *task_deque_from_task_fifo(task_fifo_t *task_fifo_p)
{
	que_t  *que_p;
#if BENCHMARK_TASK_CHAIN_OP
	volatile uint32_t  t_samp, t_diff;
#endif // BENCHMARK_TASK_CHAIN_OP

#if BENCHMARK_TASK_CHAIN_OP
	t_samp = sys_timer_get_inline();
#endif // BENCHMARK_TASK_CHAIN_OP

	task_fifo_p->task_numb--;
	que_p = que_deque(&task_fifo_p->que_head_task_info);

	if (task_fifo_p->task_numb == 0)
		task_remove_from_pri_chain(task_fifo_p);

#if BENCHMARK_TASK_CHAIN_OP
	t_diff = sys_timer_get_inline() - t_samp;
	if (t_diff > t_bench_deque_task_chain_max)
		t_bench_deque_task_chain_max = t_diff;
	if (t_diff < t_bench_deque_task_chain_min)
		t_bench_deque_task_chain_min = t_diff;
	t_bench_deque_task_chain_avrg = (t_bench_deque_task_chain_avrg * 31 + t_diff + 31) >> 5;
#endif // BENCHMARK_TASK_CHAIN_OP

	return (task_info_t *) que_p;
}


task_info_t *task_get_1st_from_task_fifo(task_fifo_t *task_fifo_p)
{
	que_t *q_head_p;

	q_head_p = &task_fifo_p->que_head_task_info;
	return (task_info_t *) q_head_p->next;
}


/* ********************************************************* */
/* ********************************************************* */

void task_switch(task_info_t *task_info_next_p)
{
	// NOTE: API is called by scheduler_core() which is encapsulated
	//       by cpsid/cpsie in the ctx6

	task_sp_ptr_now  = &task_info_run_p->sp;
	task_sp_ptr_next = &task_info_next_p->sp;

	// swap the run task
	task_id_run     = task_info_next_p->id;
	task_info_run_p = task_info_next_p;
	task_info_run_p->run_stat.time_ready = sys_clock_get_kernel();
	task_info_run_p->run_stat.run_counter++;
	task_ctx_counter++;

	return;
}


/* ********************************************************* */
/* ********************************************************* */

void task_info_dump (task_info_t *task_p, int event)
{
	printf("+++ Task Info (event %d) +++\r\n", event);
	printf("  <self>    : %p\r\n", task_p);
	printf("  que_prev  : %p\r\n", task_p->que_task_info.prev);
	printf("  que_next  : %p\r\n", task_p->que_task_info.next);
	printf("  id        : %d\r\n", task_p->id);
	printf("  name      : %s\r\n", task_p->name);
	printf("  prior     : %d\r\n", task_p->priority);
	printf("  state     : %s\r\n", task_state_name[task_p->state]);
	printf("  t_wakeup  : %lu\r\n", task_p->wake_up_time);
	printf("  stack_base: %p\r\n", (void *) task_p->stack_base);
	printf("  stack_size: %d\r\n", task_p->stack_size);
	printf("  stack_ptr : %p\r\n", (void *) task_p->sp);
	printf("  run_t_redy: %llu\r\n", task_p->run_stat.time_ready);
	printf("  run_t_ttl : %llu\r\n", task_p->run_stat.time_ttl);
	printf("  run_count : %lu\r\n", task_p->run_stat.run_counter);
}


void task_info_dump_all(void)
{
	task_info_t *task_p;
	uint32_t n;

	printf("=== All Task Info (total %lu tasks) ===\r\n", task_numb_total);
	for (n = 0; n < TASK_NUMB; n++) {
		task_p = &task_info_pool[n];
		if (task_p->state == TASK_STATE_UNUSED) {
			continue;
		}
		task_info_dump(task_p, 0);
	}
}


void task_info_summary (void)
{
	printf("Task number: %lu\r\n", task_numb_total);
	printf("System load: %lu.%02lu%%\r\n", sys_cpu_load/100, sys_cpu_load%100);
}


void task_fifo_dump(task_fifo_t *fifo)
{
	que_t *task;
	uint16_t count;

	printf("=== Task FIFO ===\r\n");
	printf("  <self>    : %p\r\n", fifo);
	printf("  head_next : %p\r\n", fifo->que_head_task_info.next);
	printf("  head_prev : %p\r\n", fifo->que_head_task_info.prev);
	printf("  priority  : %u\r\n", fifo->priority);
	printf("  task_num  : %u\r\n", fifo->task_numb);

	count = 0;
	task = fifo->que_head_task_info.next;
	while (task != &fifo->que_head_task_info) {
		task = task->next;
		count++;
	}
	if (count != fifo->task_numb) {
		printf("  ERROR: task_numb %u != count %u\r\n", fifo->task_numb, count);
	}

	task = fifo->que_head_task_info.next;
	while (task != &fifo->que_head_task_info) {
		task_info_dump((task_info_t *)task, 0);
		task = task->next;
	}
}


void task_fifo_dump_all(void)
{
	task_fifo_t *fifo;
	uint32_t n;

	printf("### All Task FIFO (%d FIFO) ###\r\n", TASK_FIFO_NUMB);
	for (n = 0; n < TASK_FIFO_NUMB; n++) {
		fifo = &task_fifo_pool[n];
		task_fifo_dump(fifo);
	}
}


void task_dump_pri_act_map(void)
{
	printf("### Priority Act Map ###\r\n");
	printf("  act_b_map  : 0x%08lx\r\n", pri_act_map.act_b_map);
	printf("  act_pri_num: %lu\r\n", pri_act_map.act_pri_num);
}
