/*
 * idle_loop.c
 *
 *  Created on: Nov 7, 2015
 *      Author: Y.W. Lee
 */ 

#include <stdint.h>
#include "kernel/task_util.h"


uint64_t  idle_loop_counter;
uint32_t  idle_loop_id;


void idle_loop(void *arg)
{
    idle_loop_id = task_self_id();

    while (1) {
        idle_loop_counter++;
    }
}
