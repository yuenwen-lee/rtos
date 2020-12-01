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
#include "kernel/sleep.h"
#include "kernel/mutex.h"
#include "examples/func_example.h"
#include "examples/task_example.h"


static task_mutex_stat_t task_mutex_stat[TASK_NUMB];


uint32_t task_mutex (void *arg)
{
    uint32_t id;
    mutex_t *mutex_p;
    task_mutex_stat_t *task_stat_p;

    if (arg == NULL) {
        printf("Invalid arrgument ...");
        goto EXIT;
    }

    id = task_self_id();
    task_stat_p = &task_mutex_stat[id];
    task_stat_p->used = 1;
    task_stat_p->count_lock = 0;

    mutex_p = (mutex_t *) arg;
    while (1) {
        mutex_lock(mutex_p);
        task_stat_p->count_lock++;
        sleep_msec(20);
        mutex_unlock(mutex_p);
    }

 EXIT:
    task_terminate();
    return 1;
}


void task_mutex_stat_display(void)
{
    uint32_t  n;

    printf("Task_Example_Mutex Info +++\r\n");
    for (n = 0; n < TASK_NUMB; ++n) {
        task_mutex_stat_t *stat = &task_mutex_stat[n];
        if (stat->used == 0) {
            continue;
        }
        printf("    %2lu - mutex lock: %lu\r\n", n, stat->count_lock);
    }
    printf("\n");
}
