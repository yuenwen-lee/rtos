/*
 * cmd_util.h
 *
 * Created on: Dec 11, 2018
 *     Author: Y.W. Lee
 */ 

#ifndef _CMD_UTIL_H_
#define _CMD_UTIL_H_

#include <stdint.h>


// ############################################################################
// ############################################################################

#define ARG_LEN     32


// ############################################################################
// ############################################################################

typedef struct arg_info_ {
    char *beg;
    uint32_t len;
} arg_info_t;


typedef struct cmd_info_ {
    char      *cmd_buf;       // command buffer
    uint32_t   cmd_len;       // number of char in cmd_buf
    uint32_t   arg_num;       // number of valid arguments in cmd_buf
    uint32_t   arg_rd_indx;   // read index of argument in arg_list
    uint32_t   arg_list_size; // capacity of arg_list
    arg_info_t arg_list[ARG_LEN];   // argument list
} cmd_info_t;


// ############################################################################
// ############################################################################

int32_t cmd_segment(cmd_info_t *cmd_info);
void cmd_arg_dump(cmd_info_t *cmd_info);
void cmd_arg_print(arg_info_t *arg_info);

int cmd_arg_avail(cmd_info_t *cmd_info);
arg_info_t *cmd_arg_read(cmd_info_t *cmd_info);
arg_info_t *cmd_arg_pop(cmd_info_t *cmd_info);

int cmd_arg_1st_is_help (cmd_info_t *cmd_info);


// ############################################################################
// ############################################################################

#define macro_cmd_arg_pop(cmd_info, arg)       \
{                                              \
    arg_info_t *_arg_ = cmd_arg_pop(cmd_info); \
    if (!_arg_) {                              \
        break;                                 \
    } else {                                   \
        (arg) = _arg_;                         \
    }                                          \
}


#endif /* _CMD_UTIL_H_ */
