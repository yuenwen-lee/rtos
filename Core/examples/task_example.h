/*
 * task_examples.h
 *
 * Created on: Sep 15, 2018
 *     Author: Y.W. Lee
 */ 

#ifndef _TASK_EXAMPLES_H_
#define _TASK_EXAMPLES_H_


//
// task_example_dummy.c  .........................................
//
uint32_t task_dummy_AA(void *argv);
void task_dummy_AA_dump(void);


//
// task_example_timer.c  .........................................
//
#define TASK_TIMER_NUM    20

typedef struct task_timer_stat_ {
    uint32_t used;
    uint32_t wake_num;
    uint32_t wake_count;
    uint32_t wake_count_prev;
    uint32_t wake_count_dlt;
    uint32_t overdue;
    uint32_t overdue_prev;
    uint32_t overdue_delt;
    float    math_diff;
} task_timer_stat_t;

typedef struct task_timer_cnfg_ {
    uint32_t id;       // id
    uint32_t period;   // in msec
} task_timer_cnfg_t;


uint32_t task_timer(void *arg);
uint32_t task_timer_sinf(void *arg);
uint32_t task_timer_tanf(void *arg);
void task_timer_stat_update(void);
void task_timer_stat_display(void);
uint32_t task_timer_hello(void);
uint32_t task_timer_greeting (void);

//
// task_example_mutex.c  .........................................
//
typedef struct task_mutex_stat_ {
    uint32_t used;
    uint32_t count_lock;
} task_mutex_stat_t;


uint32_t task_mutex (void *arg);
void task_mutex_stat_display(void);


#endif /* _TASK_EXAMPLES_H_ */
