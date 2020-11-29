/*
 * mutex.c
 *
 *  Created on: Dec 29, 2016
 *      Author: Y.W. Lee
 */ 

#include <stdio.h>
#include <stdint.h>

#include "sys_core/sys_util.h"
#include "kernel/que.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "mutex.h"

#include "utility/bits_op_tool.h"


void mutex_reset (mutex_t *mutex_p)
{
    int n;

    cpu_irq_enter_critical();    // __disable_irq();

    mutex_p->task_hold = NULL;
    mutex_p->pri_act_map.act_b_map = 0;
    mutex_p->pri_act_map.act_pri_num = 0;

    for (n = 0; n < TASK_FIFO_NUMB; ++n) {
        task_fifo_t *task_fifo_p = &mutex_p->task_fifo[n];
        task_fifo_p->priority = n;
        task_fifo_p->task_numb = 0;
        que_init(&task_fifo_p->que_head_task_info);
    }

    cpu_irq_leave_critical();    // __enable_irq();
}


int mutex_try_lock (mutex_t *mutex_p)
{
    int rtn;

    cpu_irq_enter_critical();    // __disable_irq();

    if (mutex_p->task_hold == NULL) {
        mutex_p->task_hold = task_info_run_p;
        rtn = 1;
    } else {
        rtn = 0;
    }

    cpu_irq_leave_critical();    // __enable_irq();

    return rtn;
}


void mutex_lock (mutex_t *mutex_p)
{
    cpu_irq_enter_critical();    // __disable_irq();

    if (mutex_p->task_hold == task_info_run_p) {
        // try to lock the same mutex multiple times?
        cpu_irq_leave_critical();    // __enable_irq();
        return;
    }

    while (mutex_p->task_hold != NULL) {
        // mutex is locked by other ..
        uint32_t priority = task_info_run_p->priority;
        task_fifo_t *task_fifo_p = &mutex_p->task_fifo[priority];

        que_enque(&task_fifo_p->que_head_task_info, &task_info_run_p->que_task_info);
        task_fifo_p->task_numb++;
        if (task_fifo_p->task_numb == 1) {
            uint32_t  bit_loc;
            bit_loc = priority & 0x1F;   // priority % 32
            mutex_p->pri_act_map.act_b_map |= (1 << bit_loc);
            mutex_p->pri_act_map.act_pri_num++;
        }

        task_info_run_p->state = TASK_STATE_WAIT_MUTEX;

        cpu_irq_leave_critical();    // __enable_irq();
        scheduler();

        cpu_irq_enter_critical();    // __disable_irq();
    }

    mutex_p->task_hold = task_info_run_p;

    cpu_irq_leave_critical();    // __enable_irq();
}


void mutex_unlock (mutex_t *mutex_p)
{
    cpu_irq_enter_critical();    // __disable_irq();

    if (mutex_p->task_hold != task_info_run_p) {
        // if no one holds the mutex, or the mutex is NOT held by myself
        cpu_irq_leave_critical();    // __enable_irq();
        return;
    }
    mutex_p->task_hold = NULL;

    if (mutex_p->pri_act_map.act_b_map) {

        task_fifo_t  *task_fifo_p;
        task_info_t  *task_info_p;
        uint32_t      priority;

        priority = find_msb_loc_fast(mutex_p->pri_act_map.act_b_map);

        task_fifo_p = &mutex_p->task_fifo[priority];
        task_info_p = (task_info_t *) que_deque(&task_fifo_p->que_head_task_info);
        task_fifo_p->task_numb--;

        if (task_fifo_p->task_numb == 0) {
            uint32_t  bit_loc;

            bit_loc = priority & 0x1F;   // priority % 32
            mutex_p->pri_act_map.act_b_map &= ~(1 << bit_loc);
            mutex_p->pri_act_map.act_pri_num--;
        }

        // put the task to ready tree
        task_info_p->state = TASK_STATE_READY;
        (void) task_enque_to_task_fifo(task_info_p);

        cpu_irq_leave_critical();    // __enable_irq();
        scheduler();

    } else {
        cpu_irq_leave_critical();    // __enable_irq();
    }
}
