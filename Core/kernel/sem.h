/*
 * sem.h
 *
 *  Created on: Sep 24, 2013
 *      Author: Y.W. Lee
 */

#ifndef _SEM_H_
#define _SEM_H_


typedef struct sem_obj_ {
    int32_t         sem_count;
    task_fifo_t     task_fifo;
} sem_obj_t __attribute__((aligned(4)));


void sem_init(sem_obj_t *sem_p);
void sem_post(sem_obj_t *sem_p);
void sem_wait(sem_obj_t *sem_p);


#endif /* SEM_H_ */
