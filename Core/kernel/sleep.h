/*
 * sleep.h
 *
 *  Created on: Aug 27, 2013
 *      Author: wayne
 */

#ifndef _SLEEP_H_
#define _SLEEP_H_


#include "kernel/que.h"


typedef struct sleep_que_ {
	uint32_t   latest_wake_up_time;
	que_t      que_head_task_info;
	uint32_t   numb;
} sleep_que_t __attribute__((aligned(4)));


void sleep_init(void);
void sleep_core(uint32_t tick);
void sleep_msec(uint32_t msec);
void delay_usec(uint32_t usec);
void sleep_que_dump(void);

uint32_t check_sleep_que(uint32_t time_now);


#endif /* _SLEEP_H_ */
