/*
 * task_util.c
 *
 *  Created on: Aug 21, 2013
 *      Author: Y.W. Lee
 */ 

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sys_core/sys_ticks.h"
#include "kernel/task.h"
#include "kernel/task_util.h"
#include "application/cmd_util.h"
#include "application/cli_util.h"


uint64_t task_get_ticks(uint32_t task_id)
{
    if (task_id > TASK_ID_MAX) {
        return 0;
    } else {
        return task_info_pool[task_id].run_stat.time_ttl;
    }
}


uint32_t task_is_suspend(uint32_t task_id)
{
    if (task_id > TASK_ID_MAX) {
        return 0;
    } else {
        return (task_info_pool[task_id].state == TASK_STATE_SUSPEND);
    }
}


char *string_fill_buf(char* buf, uint32_t buf_len, const char *string)
{
    uint32_t str_len = strlen(string);
    uint32_t len = (str_len > (buf_len - 1)) ? (buf_len - 1) : str_len;

    memset(buf, ' ', buf_len * sizeof(char));
    for (uint32_t n = 0; n < len; ++n) {
      buf[n] = string[n];
    }

    buf[buf_len - 1] = 0;
    return buf;
}



// ############################################################################
// ##
// ############################################################################

static int cli_cb_task_root_show(cmd_info_t *cmd_info)
{
    const char *help = "usuage: sum | detail <id_start> [<id_end>]";
    arg_info_t *arg;
    uint32_t mode, n;

    if (!cmd_arg_avail(cmd_info) || cmd_arg_1st_is_help(cmd_info)) {
        printf("%s\r\n", help);
        goto EXIT;
    }

    arg = cmd_arg_pop(cmd_info);
    if (!strcmp("sum", arg->beg)) {
        mode = 0;
    } else if (!strcmp("detail", arg->beg)) {
        mode = 1;
    } else {
        printf("invalid command %s\r\n", arg->beg);
        return -1;
    }

    if (mode == 0) {
        task_info_summary();
    } else {
        uint32_t id_beg, id_end;

        arg = cmd_arg_pop(cmd_info);
        if (arg == NULL) {
            printf("miss <id_start>\r\n");
            return -1;
        }
        id_beg = atoi(arg->beg);
        if (id_beg < 0) {
            printf("bad <id_start> %lu\r\n", id_beg);
            return -1;
        }

        arg = cmd_arg_pop(cmd_info);
        if (arg) {
            id_end = atoi(arg->beg);
        } else {
            id_end = id_beg;
        }
        if (id_end < 0 || id_end >= task_numb_total) {
            id_end = task_numb_total - 1;
        }
        printf("%lu ~ %lu\r\n", id_beg, id_end);

        for (n = id_beg; n <= id_end; ++n) {
            task_info_dump(&task_info_pool[n], 0);
        }
    }

EXIT:
    return 0;
}


static int cli_cb_task_root_list(cmd_info_t *cmd_info)
{
    task_info_t *info_p;
    char name_buf[13];
    char stat_buf[10];
    uint32_t n;

    printf("Id   Name          Pri  State      Rdy_Count\r\n");
    printf("--   ------------  ---  ---------  ---------\r\n");
    for (n = 0; n < TASK_NUMB; ++n) {
        info_p = &task_info_pool[n];
        if (info_p->state == TASK_STATE_UNUSED) {
            continue;
        }
        printf("%2d - %s  %3u  %s  %lu\r\n", info_p->id,
               string_fill_buf(name_buf, sizeof(name_buf), info_p->name),
               info_p->priority,
               string_fill_buf(stat_buf, sizeof(stat_buf), task_state_name[info_p->state]),
               info_p->run_stat.run_counter);
    }   
    printf("\r\n");
    return 0;
}


/* flag that control the checking/displaying task stack
 * usuage in montor/display task */
uint32_t cli_task_root_stack_check_enable;


static int cli_cb_task_root_stack(cmd_info_t *cmd_info)
{
    cli_task_root_stack_check_enable = 1;
    return 0;
}


// ############################################################################
// ############################################################################

static cli_info_t cli_task_root;
static cli_info_t cli_task_root_show;
static cli_info_t cli_task_root_list;
static cli_info_t cli_task_root_stack;

void cli_task_init(void)
{
    cli_info_init_node(&cli_task_root, "task", "task info");
    cli_info_attach_root(&cli_task_root);

    cli_info_init_leaf(&cli_task_root_show, "show", "show task information",
                       cli_cb_task_root_show);
    cli_info_attach_node(&cli_task_root, &cli_task_root_show);

    cli_info_init_leaf(&cli_task_root_list, "list", "briefly list all tasks",
                       cli_cb_task_root_list);
    cli_info_attach_node(&cli_task_root, &cli_task_root_list);

    cli_info_init_leaf(&cli_task_root_stack, "stack", "check and list tasks stack usuage",
                       cli_cb_task_root_stack);
    cli_info_attach_node(&cli_task_root, &cli_task_root_stack);
}
