/*
 * cmd_util.c
 *
 * Created on: Dec 11, 2018
 *     Author: ywlee
 */ 

#include <application/cmd_util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// ############################################################################
// ############################################################################
// ##
// ############################################################################

int32_t cmd_segment(cmd_info_t *cmd_info)
{
    char *buf = cmd_info->cmd_buf;
    int buf_len = cmd_info->cmd_len;
    arg_info_t *arg_info = cmd_info->arg_list;

    int found;
    int n, count;

    cmd_info->arg_num = 0;
    buf[buf_len - 1] = 0;    // for safety, may truncate the last command !!!
    for (n = count = found = 0; n < buf_len; ++n) {

        char c = buf[n];

        if (c <= 0x20 || c >= 0x7F) {
            if (found) {
                buf[n] = 0;
            }
            found = 0;
            continue;
        }

        if (found == 0) {
            found = 1;
            count++;
            if ((uint32_t) count > cmd_info->arg_list_size) {
                return -1;   // overflow !!!
            }

            cmd_info->arg_num = count;
            arg_info[count - 1].beg = buf + n;
            arg_info[count - 1].len = 0;
        }
        arg_info[count - 1].len++;
    }

    return count;
}


void cmd_arg_dump(cmd_info_t *cmd_info)
{
    arg_info_t *arg_info = cmd_info->arg_list;
    int count = cmd_info->arg_num;
    int n;

    printf("%d commands (%lu)\n", count, cmd_info->cmd_len);
    for (n = 0; n < count; ++n) {
        printf("  %02d - %s (%lu)\n", n, arg_info[n].beg, arg_info[n].len);
    }
}


void cmd_arg_print(arg_info_t *arg_info)
{
    uint32_t n;

    for (n = 0; n < arg_info->len; ++n) {
        putchar(arg_info->beg[n]);
    }
}


// ############################################################################
// ############################################################################
// ##
// ############################################################################

int cmd_arg_avail(cmd_info_t *cmd_info)
{
    if (cmd_info->arg_rd_indx < cmd_info->arg_num) {
        return 1;
    } else {
        return 0;
    }
}


arg_info_t *cmd_arg_read(cmd_info_t *cmd_info)
{
    return &cmd_info->arg_list[cmd_info->arg_rd_indx];
}


arg_info_t *cmd_arg_pop(cmd_info_t *cmd_info)
{
    if (cmd_info->arg_rd_indx >= cmd_info->arg_num) {
        return NULL;

    } else {
        arg_info_t *arg_info = &cmd_info->arg_list[cmd_info->arg_rd_indx];
        cmd_info->arg_rd_indx++;
        return arg_info;
    }
}


int cmd_arg_1st_is_help(cmd_info_t *cmd_info)
{
    if (!cmd_arg_avail(cmd_info)) {
        return 0;
    }

    arg_info_t *arg_info = cmd_arg_read(cmd_info);
    if (!strcmp(arg_info->beg, "?") || !strcmp(arg_info->beg, "help")) {
        return 1;
    } else {
        return 0;
    }
}
