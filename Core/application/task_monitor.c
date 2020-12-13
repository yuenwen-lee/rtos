/*
 * task_monitor.c
 *
 * Created on: Nov 7, 2015
 *     Author: Y.W. Lee
 */ 

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sys_core/sys_ticks.h"
#include "sys_core/sys_timer.h"
#include "kernel/task.h"
#include "kernel/task_util.h"
#include "kernel/timer.h"
#include "kernel/run_stat.h"
#include "kernel/sem.h"
#include "application/task_monitor.h"
#include "sys_device/dev_board.h"
#include "sys_device/dev_uart.h"
#include "examples/task_example.h"


static void monitor_display_ctx_stat(void);


uint32_t task_id_monitor;
uint32_t task_id_monitor_display;


void monitor_task(void *arg)
{
    timer_obj_t  timer_mon;
    uint32_t task_id_mon_disp;
    run_stat_que_t *run_stat_que_mon_disp;

    task_id_mon_disp = (uint32_t) arg;
    run_stat_que_mon_disp = run_stat_get_que_task(task_id_mon_disp);

    timer_init(&timer_mon, 1000);
    while (1) {
        timer_wait(&timer_mon);

        board_toggle_led_monitor();
        run_stat_update();
//      task_timer_stat_update();   // Task EXAMPLE ........

        if (run_stat_que_mon_disp->time_dlt == 0) {
            task_set_priority(task_id_mon_disp, (PRIORITY_HIGHEST - 1));
        } else {
            task_resume(task_id_mon_disp);
        }
    }
}


void monitor_display_task(void *arg)
{
    uint32_t my_priority;

    my_priority = task_get_priority_self();

    while (1) {
        task_suspend();

        if (cli_run_stat_root_load_display_enable) {
//          putchar(ASCII_FF);
            putchar('\033');
            putchar('[');
            putchar('2');
            putchar('J');
            monitor_display_ctx_stat();
            run_stat_display();
//          task_timer_stat_display();   // Task EXAMPLE ........
//          task_mutex_stat_display();   // Task EXAMPLE ........
        }

        task_set_priority_self(my_priority);
    }
}


static void monitor_display_ctx_stat(void)
{
    printf("sys_tmr: %llu\r\n", sys_timer_64_get_inline());
    printf("sys_tck: %lu\r\n", sys_ticks);
    printf("tsk_ctx: %ld\r\n", task_ctx_counter);
    putchar('\n');
}
