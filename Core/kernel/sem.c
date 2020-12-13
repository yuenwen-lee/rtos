/*
 * sem.c
 *
 *  Created on: Sep 24, 2013
 *      Author: Y.W. Lee
 */

#include <stdint.h>

#include "sys_core/sys_timer.h"
#include "sys_core/sys_util.h"
#include "kernel/que.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "kernel/sem.h"
#include "utility/bits_op_tool.h"


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


void sem_post(sem_obj_t *sem_p)
{
    task_info_t   *task_info_p;
    task_fifo_t   *task_fifo_p;

    cpu_irq_enter_critical();    // __disable_irq();

    sem_p->sem_count++;

    task_fifo_p = &sem_p->task_fifo;
    if (task_fifo_p->task_numb == 0) {
        cpu_irq_leave_critical();    // __enable_irq();
        return;
    }

    // remove the 1st task in sema wait FIFO
    task_fifo_p->task_numb--;
    task_info_p = (task_info_t *) que_deque(&task_fifo_p->que_head_task_info);

    // put the task to ready tree
    task_info_p->state = TASK_STATE_READY;
    (void) task_enque_to_task_fifo(task_info_p);

    cpu_irq_leave_critical();    // __enable_irq();

    scheduler();
}


void sem_wait(sem_obj_t *sem_p)
{
    task_fifo_t   *task_fifo_p;

    cpu_irq_enter_critical();    // __disable_irq();

    if (task_id_run == 0) {
        cpu_irq_leave_critical();    // __enable_irq();
        return;   // never allow main/IDLE-loop try semaphore
    }

    sem_p->sem_count--;
    if (sem_p->sem_count >= 0) {
        cpu_irq_leave_critical();    // __enable_irq();

    } else {
        task_info_run_p->state = TASK_STATE_WAIT_SEM;

        // put task into sema wait FIFO
        task_fifo_p = &sem_p->task_fifo;
        que_enque(&task_fifo_p->que_head_task_info, &task_info_run_p->que_task_info);
        task_fifo_p->task_numb++;

        cpu_irq_leave_critical();    // __enable_irq();

        scheduler();
    }
}
