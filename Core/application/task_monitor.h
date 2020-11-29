/*
 * task_monitor.h
 *
 *  Created on: Oct 20, 2018
 *      Author: Y.W. Lee
 */ 

#ifndef _TASK_MONITOR_H_
#define _TASK_MONITOR_H_


extern uint32_t task_id_monitor;
extern uint32_t task_id_monitor_display;

void monitor_task(void *arg);
void monitor_display_task(void *arg);


#endif /* _TASK_MONITOR_H_ */
