/*
 * sleep.c
 *
 *  Created on: Aug 27, 2013
 *      Author: Y.W. Lee
 */ 

#include "stdint.h"
#include <stdio.h>

#include "sys_core/sys_timer.h"
#include "sys_core/sys_ticks.h"
#include "sys_core/sys_util.h"
#include "kernel/que.h"
#include "kernel/task.h"
#include "kernel/sched.h"
#include "kernel/sleep.h"


sleep_que_t sleep_que;
bool sleep_que_sample_control = false;


static void cli_sleep_init(void);
static void sleep_que_sample_save(uint32_t t_now);


void sleep_init(void)
{
    sleep_que.latest_wake_up_time = 0;
    sleep_que.numb = 0;
    que_init(&sleep_que.que_head_task_info);
    cli_sleep_init();
}


void sleep_core (uint32_t wake_up_time)
{
    que_t     *que_head_p;
    que_t     *que_p;

    uint32_t   t_now = sys_timer_get_inline();

    if (task_id_run == 0)
        return;   // never allow main/IDLE-loop task sleep

    cpu_irq_enter_critical();    // __disable_irq();

    task_info_run_p->wake_up_time = wake_up_time;
    task_info_run_p->state = TASK_STATE_WAIT_SLEEP;

    que_head_p = &sleep_que.que_head_task_info;
    que_p = que_head_p->prev;
    while (que_p != que_head_p) {
        if ((int32_t) (wake_up_time - ((task_info_t *) que_p)->wake_up_time) >= 0)
            break;
        que_p = que_p->prev;
    }

    que_insert_after(que_p, &task_info_run_p->que_task_info);
    if (que_head_p->next == (que_t *) task_info_run_p)
        sleep_que.latest_wake_up_time = wake_up_time;
    sleep_que.numb++;

    if (sleep_que_sample_control) {
        sleep_que_sample_save(t_now);
    }

    cpu_irq_leave_critical();    // __enable_irq();

    scheduler();
}


void sleep_core_64(uint64_t wake_up_time_64)
{
    uint32_t wake_up_time;

    while (1) {
        uint64_t t_now_64 = sys_timer_64_get_inline();
        if (t_now_64 >= wake_up_time_64) {
            break;  // already overdue ...
        }

        uint64_t t_diff_64 = wake_up_time_64 - t_now_64;
        wake_up_time = (uint32_t) t_now_64;
        if ((uint32_t) (t_diff_64 >> CPU_TICKS_SLEEP_SHIFT) != 0) {
            wake_up_time += CPU_TICKS_SLEEP_STEP;
        } else {
            wake_up_time += (uint32_t) (t_diff_64 & CPU_TICKS_SLEEP_MASK);
        }

        sleep_core(wake_up_time);  // put to sleep
    }
}


void sleep_msec(uint32_t msec)
{
    uint64_t ticks_64 = msec_to_cpu_tick_64(msec);

    if ((ticks_64 >> CPU_TICKS_SLEEP_SHIFT) == 0) {
        uint32_t wake_up_time = sys_timer_get_inline() + (uint32_t) ticks_64;
        sleep_core(wake_up_time);
    } else {
        uint64_t wake_up_time = sys_timer_64_get_inline() + ticks_64;
        sleep_core_64(wake_up_time);
    }
}


void delay_usec(uint32_t usec)
{
    volatile uint32_t timeup, time;

    timeup = sys_timer_get_inline() + usec_to_cpu_tick(usec);
    do {
        time = sys_timer_get_inline();
    } while ((int32_t) (timeup - time) > 0);
}


/* ********************************************************* */
/* ********************************************************* */

uint32_t check_sleep_que(uint32_t time_now)
{
    que_t       *que_head_p;
    que_t       *que_p;
    task_info_t *task_info_p;
    uint32_t     ready;

    cpu_irq_enter_critical();    // __disable_irq();

    ready = 0;
    if (sleep_que.numb == 0 ||
        ((int32_t) (sleep_que.latest_wake_up_time - time_now) > 0)) {
        goto CHECK_SLEEP_QUE_EXIT;
    }

    que_head_p = &sleep_que.que_head_task_info;
    que_p = que_head_p->next;
    while (que_p != que_head_p) {

        if ((int32_t) (((task_info_t *) que_p)->wake_up_time - time_now) > 0)
            break;
        sleep_que.numb--;
        task_info_p = (task_info_t *) que_deque(que_head_p);

        // put the task to ready tree
        task_info_p->state = TASK_STATE_READY;
        (void) task_enque_to_task_fifo(task_info_p);
        ready++;

        que_p = que_head_p->next;
    }

    if (sleep_que.numb) {
        sleep_que.latest_wake_up_time = ((task_info_t *) que_p)->wake_up_time;
    }

CHECK_SLEEP_QUE_EXIT:
    cpu_irq_leave_critical();    // __enable_irq();
    return ready;
}


/* ********************************************************* */
/*     CLI Cmnd                                              */
/* ********************************************************* */

#include "task_util.h"
#include "application/cmd_util.h"
#include "application/cli_util.h"


/* ********************************************************* */
/* ********************************************************* */
#define SLEEP_QUE_SAMPLE_NUM   8


typedef struct sleep_que_info_ {
    uint32_t  id;
    uint32_t  t_wkup;
} sleep_que_info_t;


uint32_t sleep_que_smpl_indx;
sleep_que_info_t sleep_que_smpl[SLEEP_QUE_SAMPLE_NUM][TASK_NUMB];
uint32_t sleep_que_smpl_t_now[SLEEP_QUE_SAMPLE_NUM];
uint32_t sleep_que_smpl_count[SLEEP_QUE_SAMPLE_NUM];


static void sleep_que_sample_save(uint32_t t_now)
{
    uint32_t     smpl_indx = sleep_que_smpl_indx;
    que_t       *que_head_p;
    que_t       *que_p;
    task_info_t *task_info_p;
    uint32_t     count;

    sleep_que_info_t *que_info_p = sleep_que_smpl[smpl_indx];

    sleep_que_smpl_t_now[smpl_indx] = t_now;

    que_head_p = &sleep_que.que_head_task_info;
    que_p = que_head_p->next;
    count = 0;
    while (que_p != que_head_p) {
        task_info_p = (task_info_t *) que_p;
        que_info_p[count].id     = task_info_p->id;
        que_info_p[count].t_wkup = task_info_p->wake_up_time;
        count++;
        que_p = que_p->next;
    }
    sleep_que_smpl_count[smpl_indx] = count;

    sleep_que_smpl_indx = (sleep_que_smpl_indx + 1) & (SLEEP_QUE_SAMPLE_NUM - 1);
}


static int cli_cb_sleep_root_smpl_show(cmd_info_t *cmd_info)
{
    char  name_buf[13];

    for (uint32_t nn = 0; nn < SLEEP_QUE_SAMPLE_NUM; ++nn) {
        sleep_que_info_t *que_info_p = sleep_que_smpl[nn];
        uint32_t count = sleep_que_smpl_count[nn];

        printf("sys_tmr: 0x%08lx (%lu)\r\n", sleep_que_smpl_t_now[nn], sleep_que_smpl_t_now[nn]);
        for (uint32_t mm = 0; mm < count; ++mm) {
            uint32_t id     = que_info_p[mm].id;
            uint32_t t_wkup = que_info_p[mm].t_wkup;
            task_info_t *task_info_p = &task_info_pool[id];
            printf("%2lu - %s   0x%08lx - %lu\r\n", id,
                   string_fill_buf(name_buf, sizeof(name_buf), task_info_p->name),
                   t_wkup, t_wkup);
        }
        printf("\n");
    }

    return 0;
}


static int cli_cb_sleep_root_smpl_enable(cmd_info_t *cmd_info)
{
    sleep_que_sample_control = true;
    printf("sleep_que_sample_control: %d\n\r", sleep_que_sample_control);
    return 0;
}


static int cli_cb_sleep_root_smpl_disable(cmd_info_t *cmd_info)
{
    sleep_que_sample_control = false;
    printf("sleep_que_sample_control: %d\n\r", sleep_que_sample_control);
    return 0;
}


/* ********************************************************* */
/* ********************************************************* */

static int cli_cb_sleep_root_que(cmd_info_t *cmd_info)
{
    que_t       *que_head_p;
    que_t       *que_p;
    task_info_t *task_info_p;
    uint32_t     count;
    uint64_t     sys_tmr;
    uint32_t     sys_tck, tsk_ctx;
    uint32_t     tsk_id[TASK_NUMB];
    uint32_t     t_wkup[TASK_NUMB];
    char         name_buf[13];

    cpu_irq_enter_critical();    // __disable_irq();

    sys_tmr = sys_timer_64_get_inline();
    sys_tck = sys_ticks;
    tsk_ctx = task_ctx_counter;

    que_head_p = &sleep_que.que_head_task_info;
    que_p = que_head_p->next;
    count = 0;
    while (que_p != que_head_p) {
        task_info_p = (task_info_t *) que_p;
        tsk_id[count] = task_info_p->id;
        t_wkup[count] = task_info_p->wake_up_time;
        count++;
        que_p = que_p->next;
    }

    cpu_irq_leave_critical();    // __enable_irq();

    printf("sys_tmr_32: %lu (0x%08lx)\r\n", (uint32_t) sys_tmr, (uint32_t) sys_tmr);
    printf("sys_tmr_64: %llu\r\n", sys_tmr);
    printf("sys_ticks : %lu\r\n",  sys_tck);
    printf("tsk_swtich: %ld\r\n",  tsk_ctx);
    printf("Id   Name           Wakeup Time\r\n");
    printf("--   ------------   ---------- ----------\r\n");
    for (int n = 0; n < count; ++n) {
        uint32_t id = tsk_id[n];
        task_info_p = &task_info_pool[id];
        printf("%2lu - %s  0x%08lx %lu\r\n", id,
               string_fill_buf(name_buf, sizeof(name_buf), task_info_p->name),
               t_wkup[n], t_wkup[n]);
    }
    printf("\n");

    return 0;
}


/* ********************************************************* */
/* ********************************************************* */

static cli_info_t cli_sleep_root;
static cli_info_t cli_sleep_root_que;
static cli_info_t cli_sleep_root_smpl;
static cli_info_t cli_sleep_root_smpl_show;
static cli_info_t cli_sleep_root_smpl_enable;
static cli_info_t cli_sleep_root_smpl_disable;


static void cli_sleep_init(void)
{
    cli_info_init_node(&cli_sleep_root, "sleep", "sleep info");
    cli_info_attach_root(&cli_sleep_root);

    cli_info_init_leaf(&cli_sleep_root_que, "que", "dump the sleep queue",
                       cli_cb_sleep_root_que);
    cli_info_attach_node(&cli_sleep_root, &cli_sleep_root_que);

    cli_info_init_node(&cli_sleep_root_smpl, "smpl", "control/dump the sleep que sample");
    cli_info_attach_node(&cli_sleep_root, &cli_sleep_root_smpl);

    cli_info_init_leaf(&cli_sleep_root_smpl_show, "dump", "dump the sleep que sample",
                       cli_cb_sleep_root_smpl_show);
    cli_info_attach_node(&cli_sleep_root_smpl, &cli_sleep_root_smpl_show);

    cli_info_init_leaf(&cli_sleep_root_smpl_enable, "enable", "enable the sleep que sample",
                       cli_cb_sleep_root_smpl_enable);
    cli_info_attach_node(&cli_sleep_root_smpl, &cli_sleep_root_smpl_enable);

    cli_info_init_leaf(&cli_sleep_root_smpl_disable, "disable", "disable the sleep que sample",
                       cli_cb_sleep_root_smpl_disable);
    cli_info_attach_node(&cli_sleep_root_smpl, &cli_sleep_root_smpl_disable);
}
