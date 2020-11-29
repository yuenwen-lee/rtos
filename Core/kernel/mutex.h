/*
 * mutex.h
 *
 *  Created on: Dec 29, 2016
 *      Author: Y.W. Lee
 */

#ifndef _MUTEX_H_
#define _MUTEX_H_


#include "kernel/task.h"


typedef struct mutex_s {
    task_info_t   *task_hold;
    pri_act_map_t  pri_act_map;
    task_fifo_t    task_fifo[TASK_FIFO_NUMB];
} mutex_t;


void mutex_reset (mutex_t *mutex_p);
int mutex_try_lock (mutex_t *mutex_p);
void mutex_lock (mutex_t *mutex_p);
void mutex_unlock (mutex_t *mutex_p);

  
#endif /* _MUTEX_H_ */
