/*
 * run_stat.c
 *
 *  Created on: Feb 4, 2017
 *      Author: Y.W. Lee
 */ 

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sys_core/sys_timer.h"
#include "sys_core/sys_util.h"
#include "kernel/que.h"
#include "kernel/task.h"
#include "kernel/task_util.h"
#include "kernel/run_stat.h"
#include "kernel/sched.h"
#include "application/cmd_util.h"
#include "application/cli_util.h"


#define SCALE_2_DIGIT  10000  // to get 2 digits accuracy afer floating point


static run_time_stack_t  run_time_stack[RUN_TIME_STACK_MAX_LEVEL + 1];
static uint32_t  run_time_stack_wr_indx;
static uint32_t  run_time_stack_ov_count;
static uint32_t  run_time_stack_uf_count;
static uint32_t  run_time_stack_err;

run_stat_que_t run_stat_que_stck_ov;
run_stat_t run_stat_stck_ov;
uint32_t sys_cpu_load;


static void run_stat_failure(const char *msg);


void run_time_set_task (uint32_t t_now)
{
    if (run_time_stack_wr_indx != 0) {
        run_time_stack_err++;
    } else {
        run_time_stack[0].t_stamp = t_now;
        run_time_stack[0].t_accum = 0;
    }
}


uint32_t run_time_get_task (uint32_t t_now)
{
    if (run_time_stack_wr_indx != 0) {
        run_time_stack_err++;
        return 0;
    } else {
        return (run_time_stack[0].t_accum + (t_now - run_time_stack[0].t_stamp));
    }
}


void run_time_stack_push_isr (int interrupt_disable)
{
    uint32_t t_now;
    uint32_t wr_indx;

    if (interrupt_disable) {
        cpu_irq_enter_critical();    // __disable_irq();
    }

    t_now = sys_timer_get_inline();
    wr_indx = run_time_stack_wr_indx;

    if (wr_indx < RUN_TIME_STACK_MAX_LEVEL) {
        run_time_stack[wr_indx].t_accum += (t_now - run_time_stack[wr_indx].t_stamp);

        wr_indx++;
        run_time_stack[wr_indx].t_stamp = t_now;
        run_time_stack[wr_indx].t_accum = 0;

    } else {
        run_time_stack_ov_count++;
    }

    run_time_stack_wr_indx++;

    if (interrupt_disable) {
        cpu_irq_leave_critical();    // __enable_irq();
    }
}


uint32_t run_time_stack_pop_isr (int interrupt_disable)
{
    uint32_t t_accum = 0;
    uint32_t indx;

    if (interrupt_disable) {
        cpu_irq_enter_critical();    // __disable_irq();
    }

    indx = run_time_stack_wr_indx;
    if (indx > RUN_TIME_STACK_MAX_LEVEL) {
        run_time_stack_wr_indx--;

    } else if (indx == RUN_TIME_STACK_MAX_LEVEL) {
        uint32_t t_now = sys_timer_get_inline();
        run_stat_stck_ov.time_ttl += run_time_stack[indx].t_accum + (t_now - run_time_stack[indx].t_stamp);
        run_stat_stck_ov.run_counter++;

        indx--;
        run_time_stack[indx].t_stamp = t_now;
        run_time_stack_wr_indx--;

    } else if (indx) {
        uint32_t t_now = sys_timer_get_inline();
        t_accum = run_time_stack[indx].t_accum + (t_now - run_time_stack[indx].t_stamp);

        indx--;
        run_time_stack[indx].t_stamp = t_now;
        run_time_stack_wr_indx--;

    } else {
        run_time_stack_uf_count++;
    }

    if (interrupt_disable) {
        cpu_irq_leave_critical();    // __enable_irq();
    }

    return t_accum;
}


void run_time_sche_start (void)
{
    uint32_t t_now;

    t_now = sys_timer_get_inline();
    if (run_time_stack_wr_indx != 0) {
        // Should never happen, PendSV is the lowest IRQ ...
        run_stat_failure("start");
    }

    run_time_stack[0].t_accum += (t_now - run_time_stack[0].t_stamp);
    run_time_stack[1].t_stamp = t_now;
    run_time_stack[1].t_accum = 0;
    run_time_stack_wr_indx = 1;
}


void run_time_sche_end (run_stat_t *task_stat, run_stat_t *sche_stat)
{
    if (run_time_stack_wr_indx != 1) {
        // Should never happen, PendSV is the lowest IRQ ...
        run_stat_failure("end");
    }
    run_time_stack_wr_indx = 0;

    task_stat->time_ttl += run_time_stack[0].t_accum;
    run_time_stack[0].t_accum = 0;
    volatile uint32_t ttl_tmp = (run_time_stack[1].t_accum - run_time_stack[1].t_stamp);

    volatile uint32_t t_now = sys_timer_get_inline();
    run_time_stack[0].t_stamp = t_now;
    sche_stat->time_ttl += (ttl_tmp + t_now);
}


/* ********************************************************* */
/* ********************************************************* */

run_stat_que_t  run_stat_task_pool[TASK_NUMB];
que_t run_stat_que_task;
que_t run_stat_que_isr;


void run_stat_init(void)
{
    que_init(&run_stat_que_task);
    que_init(&run_stat_que_isr);
    run_stat_reg_stck_ov();

    cli_run_stat_init();
}


void run_stat_register_task (const char *name, uint32_t task_id, run_stat_t *stat)
{
    run_stat_que_t *stat_q;

    stat_q = &run_stat_task_pool[task_id];
    que_init(&stat_q->link);

    cpu_irq_enter_critical();    // __disable_irq();
    que_enque(&run_stat_que_task, &stat_q->link);
    cpu_irq_leave_critical();    // __enable_irq();

    stat->run_counter = 0;
    stat->time_ttl = 0;

    stat_q->id = task_id;
    stat_q->name = name;
    stat_q->stat_p = stat;
    stat_q->time_ttl_prev = 0;
}

void run_stat_register_isr (const char *name, uint32_t isr_id, run_stat_t *stat, run_stat_que_t *stat_q)
{
    que_init(&stat_q->link);

    cpu_irq_enter_critical();    // __disable_irq();
    que_enque(&run_stat_que_isr, &stat_q->link);
    cpu_irq_leave_critical();    // __enable_irq();

    stat->run_counter = 0;
    stat->time_ttl = 0;

    stat_q->id = isr_id;
    stat_q->name = name;
    stat_q->stat_p = stat;
    stat_q->time_ttl_prev = 0;
    stat_q->run_counter_prev = 0;
}


void run_stat_reg_stck_ov(void)
{
    run_stat_register_isr("stck_ov", 0, &run_stat_stck_ov, &run_stat_que_stck_ov);
}


/* ********************************************************* */
/* ********************************************************* */

run_stat_que_t *run_stat_get_que_task(uint32_t task_id)
{
    que_t *que;
    run_stat_que_t *stat_q;

    que = run_stat_que_task.next;
    while (que != &run_stat_que_task) {

        stat_q = (run_stat_que_t *) que;
        if (stat_q->id == task_id) {
            return stat_q;
        }

        que = que->next;
    }

    return NULL;
}

run_stat_que_t *run_stat_get_que_isr(uint32_t isr_id)
{
    que_t *que;
    run_stat_que_t *stat_q;

    que = run_stat_que_isr.next;
    while (que != &run_stat_que_isr) {

        stat_q = (run_stat_que_t *) que;
        if (stat_q->id == isr_id) {
            return stat_q;
        }

        que = que->next;
    }

    return NULL;
}


/* ********************************************************* */
/* ********************************************************* */

static uint32_t run_stat_update_core (que_t *que_head)
{
    que_t *que;
    run_stat_que_t *stat_q;
    run_stat_t *stat;
    uint32_t cycle = 0;

    que = que_head->next;
    while (que != que_head) {

        stat_q = (run_stat_que_t *) que;
        stat = stat_q->stat_p;

        stat_q->time_dlt = (uint32_t) (stat->time_ttl - stat_q->time_ttl_prev);
        stat_q->time_ttl_prev = stat->time_ttl;
        stat_q->run_counter_dlt = stat->run_counter - stat_q->run_counter_prev;
        stat_q->run_counter_prev = stat->run_counter;
        if (stat->run_counter) {
            stat_q->time_avg = (uint32_t) (stat->time_ttl / stat->run_counter);
        } else {
            stat_q->time_avg = 0;
        }

        cycle += stat_q->time_dlt;
        que = que->next;
    }

    return cycle;
}


static uint32_t run_stat_cpu_load (que_t *que_head, uint32_t cycle)
{
#define SCALE_2_DIGIT  10000  // to get 2 digits accuracy afer floating point
#define SCALE_PRE_AMP  10
#define SCALE_AMP      (SCALE_2_DIGIT / SCALE_PRE_AMP)
    que_t *que;
    run_stat_que_t *stat_q;

    cycle /= SCALE_AMP;
    que = que_head->next;
    while (que != que_head) {
        stat_q = (run_stat_que_t *) que;
        stat_q->cpu_load = (SCALE_PRE_AMP * stat_q->time_dlt + (cycle >> 1)) / cycle;
        que = que->next;
    }

    return cycle;
}


#define BENCHMARK_STAT_UPDT_CYCLE  0


#if BENCHMARK_STAT_UPDT_CYCLE
static uint32_t t_bench_stat_updt_avrg = 0;             // 1698  9 tasks + 3 ISR
static uint32_t t_bench_stat_updt_max = 0;              // 1699
static uint32_t t_bench_stat_updt_min = (uint32_t) -1;  // 1690
static uint32_t t_bench_stat_updt_cycle_avrg;           // 49980985
static uint32_t t_bench_stat_updt_cycle_max;            // 50207977
static uint32_t t_bench_stat_updt_cycle_min = (uint32_t) -1;  // 49999999
#endif // BENCHMARK_STAT_UPDT_CYCLE


void run_stat_update (void)
{
    uint32_t cycle_ttl;
#if BENCHMARK_STAT_UPDT_CYCLE
    volatile uint32_t  t_samp, t_diff;
#endif // BENCHMARK_STAT_UPDT_CYCLE

#if BENCHMARK_STAT_UPDT_CYCLE
    t_samp = sys_timer_get_inline();
#endif // BENCHMARK_STAT_UPDT_CYCLE

    cycle_ttl = run_stat_update_core(&run_stat_que_task) +
                run_stat_update_core(&run_stat_que_isr);
    if (cycle_ttl == 0)
        return;

    run_stat_cpu_load(&run_stat_que_task, cycle_ttl);
    run_stat_cpu_load(&run_stat_que_isr,  cycle_ttl);
    sys_cpu_load = SCALE_2_DIGIT - ((run_stat_que_t *) run_stat_que_task.next)->cpu_load;

#if BENCHMARK_STAT_UPDT_CYCLE
    t_diff = sys_timer_get_inline() - t_samp;
    if (t_diff > t_bench_stat_updt_max)
        t_bench_stat_updt_max = t_diff;
    if (t_diff < t_bench_stat_updt_min)
        t_bench_stat_updt_min = t_diff;
    t_bench_stat_updt_avrg = (t_bench_stat_updt_avrg * 31 + t_diff + 31) >> 5;
    if (cycle_ttl > t_bench_stat_updt_cycle_max)
        t_bench_stat_updt_cycle_max = cycle_ttl;
    if (cycle_ttl < t_bench_stat_updt_cycle_min)
        t_bench_stat_updt_cycle_min = cycle_ttl;
    if (t_bench_stat_updt_cycle_avrg == 0)
        t_bench_stat_updt_cycle_avrg = cycle_ttl;
    t_bench_stat_updt_cycle_avrg = (t_bench_stat_updt_cycle_avrg * 31 + cycle_ttl + 31) >> 5;
#endif // BENCHMARK_STAT_UPDT_CYCLE
}


/* ********************************************************* */
/* ********************************************************* */

void run_stat_display(void)
{
    que_t *que;
    run_stat_que_t *stat_q;

    int load, cont;
    char name_buf[11];
    int n;

    // task ....
    que = run_stat_que_task.next;
    n = 0;
    while (que != &run_stat_que_task) {
        stat_q = (run_stat_que_t *) que;
        load = stat_q->cpu_load;
        cont = stat_q->run_counter_dlt;
        if (n == 0) {
            printf("tsk %s  %2d.%02d  %4d\r\n",
                   string_fill_buf(name_buf, sizeof(name_buf), stat_q->name),
                   load/100, load%100, cont);
        } else {
            printf("    %s  %2d.%02d  %4d\r\n",
                   string_fill_buf(name_buf, sizeof(name_buf), stat_q->name),
                   load/100, load%100, cont);
        }
        que = que->next;
        n++;
    }

    // ISR ....
    que = run_stat_que_isr.next;
    n = 0;
    while (que != &run_stat_que_isr) {
        stat_q = (run_stat_que_t *) que;
        load = stat_q->cpu_load;
        cont = stat_q->run_counter_dlt;
        if (n == 0) {
            printf("isr %s  %2d.%02d  %4d\r\n",
                   string_fill_buf(name_buf, sizeof(name_buf), stat_q->name),
                   load/100, load%100, cont);
        } else {
            printf("    %s  %2d.%02d  %4d\r\n",
                   string_fill_buf(name_buf, sizeof(name_buf), stat_q->name),
                   load/100, load%100, cont);
        }
        que = que->next;
        n++;
    }
    printf("\nload: %lu.%02lu%%\r\n", sys_cpu_load/100, sys_cpu_load%100);
}


/* ********************************************************* */
/* ********************************************************* */

static void run_stat_failure (const char *msg)
{
    schedule_halt();

    printf("!!!! Fatal Error !!!!\r\n");
    printf("PendSV_Handler at level %lu\r\n", run_time_stack_wr_indx);
    printf("type: %s\r\n", msg);
    printf("%s() @ line %d\r\n", __FUNCTION__, __LINE__);

    while (1) ;
}


// ############################################################################
// ##
// ############################################################################


/* flag that control displaying task/isr cpu load in display task */
uint32_t cli_run_stat_root_load_display_enable;


static int cli_cb_run_stat_root_load(cmd_info_t *cmd_info)
{
    cli_run_stat_root_load_display_enable = 1;
    return 1;
}

/* ********************************************************* */
/* ********************************************************* */

static cli_info_t cli_run_stat_root;
static cli_info_t cli_run_stat_root_load;

void cli_run_stat_init(void)
{
    cli_info_init_node(&cli_run_stat_root, "run_stat", "show system info");
    cli_info_attach_root(&cli_run_stat_root);

    cli_info_init_leaf(&cli_run_stat_root_load, "load", "show system load",
                       cli_cb_run_stat_root_load);
    cli_info_attach_node(&cli_run_stat_root, &cli_run_stat_root_load);
}
