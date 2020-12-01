/*
 * task_example_timer.c
 *
 *  Created on: Sep 15, 2018
 *      Author: Y.W. Lee
 */ 

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "kernel/task.h"
#include "kernel/task_util.h"
#include "kernel/timer.h"
#include "examples/func_example.h"
#include "examples/task_example.h"


static task_timer_stat_t timer_stat[TASK_TIMER_NUM];


uint32_t task_timer (void *arg)
{
    task_timer_cnfg_t *cnfg;
    task_timer_stat_t *stat = NULL;
    timer_obj_t  task_A_timer;
    uint32_t  id;
    uint32_t  a1, m;
    uint32_t  overdue;

    cnfg = (task_timer_cnfg_t *) arg;
    id = cnfg->id;
    if (id < TASK_TIMER_NUM) {
        stat = &timer_stat[id];
        stat->used = 1;
        stat->wake_num = 1000 / cnfg->period;
        stat->overdue = 0;
    }
    timer_init(&task_A_timer, cnfg->period);

    a1 = 0;
    while (1) {
        overdue = timer_wait_fixed(&task_A_timer);
        if (stat) {
            stat->wake_count++;
            stat->overdue += overdue;
        }

        for (m = 0; m < 2000; ++m) {
            // for some CPU, the wait state of program flash will cause CPU
            // adds huge wait cycles because of the API call (the branch)
            a1 += api_A(1, a1);
//          a1 += (id * 7);   // much faster, no branch
        }
    }

    return(a1);
}


uint32_t task_timer_sinf (void *arg)
{
    task_timer_cnfg_t *cnfg;
    task_timer_stat_t *stat = NULL;
    timer_obj_t  task_A_timer;
    uint32_t  id, m;
    uint32_t  overdue = 0;
    volatile float org, a1;

    cnfg = (task_timer_cnfg_t *) arg;
    id = cnfg->id;
    if (id < TASK_TIMER_NUM) {
        stat = &timer_stat[id];
        stat->used = 1;
        stat->wake_num = 1000 / cnfg->period;
        stat->overdue = 0;
    }
    timer_init(&task_A_timer, cnfg->period);

    org = (3.141592653f / 3.0f);
    a1 = org;
    while (1) {
        overdue = timer_wait_fixed(&task_A_timer);
        if (stat) {
            stat->wake_count++;
            stat->overdue += overdue;
            stat->math_diff = org - a1;
        }

        for (m = 0; m < 400; ++m) {
            a1 = api_sinf(a1);
        }
    }

    return(a1);
}


uint32_t task_timer_tanf (void *arg)
{
    task_timer_cnfg_t *cnfg;
    task_timer_stat_t *stat = NULL;
    timer_obj_t  task_A_timer;
    uint32_t  id, m;
    uint32_t  overdue = 0;
    volatile float org, a1;

    cnfg = (task_timer_cnfg_t *) arg;
    id = cnfg->id;
    if (id < TASK_TIMER_NUM) {
        stat = &timer_stat[id];
        stat->used = 1;
        stat->wake_num = 1000 / cnfg->period;
        stat->overdue = 0;
    }
    timer_init(&task_A_timer, cnfg->period);

    org = (3.141592653f / 3.0f);
    a1 = org;
    while (1) {
        overdue = timer_wait_fixed(&task_A_timer);
        if (stat) {
            stat->wake_count++;
            stat->overdue += overdue;
            stat->math_diff = org - a1;
        }

        for (m = 0; m < 400; ++m) {
            a1 = api_tanf(a1);
        }
    }

    return(a1);
}


void task_timer_stat_update (void)
{
    uint32_t  n;

    for (n = 0; n < TASK_TIMER_NUM; ++n) {
        task_timer_stat_t *stat = &timer_stat[n];
        if (stat->used == 0)
            continue;
        stat->wake_count_dlt = stat->wake_count - stat->wake_count_prev;
        stat->wake_count_prev = stat->wake_count;
        stat->overdue_delt = stat->overdue - stat->overdue_prev;
        stat->overdue_prev = stat->overdue;
    }
}


void task_timer_stat_display (void)
{
    uint32_t  n;
    const char *rst;

    printf("Task_Example_Timer Info +++\r\n");
    for (n = 0; n < TASK_TIMER_NUM; ++n) {
        task_timer_stat_t *stat = &timer_stat[n];
        if (stat->used == 0) {
            continue;
        }

        if (stat->math_diff == 0.0f) {
            rst = "GOOD";
        } else {
            rst = "BAD";
        }
        printf("    %lu - wake total: %lu, wake delta: %lu, overdue delta: %3lu, math_diff: %e (%s)\r\n",
               n, stat->wake_count, stat->wake_count_dlt, stat->overdue_delt, stat->math_diff, rst);
    }
    printf("\n");
}
