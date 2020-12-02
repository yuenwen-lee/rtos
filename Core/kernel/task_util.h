/*
 * task_util.h
 *
 *  Created on: Aug 21, 2013
 *      Author: Y.W. Lee
 */

#ifndef _TASK_UTIL_H_
#define _TASK_UTIL_H_


#include "kernel/task.h"


uint64_t task_get_ticks(uint32_t task_id);
uint32_t task_is_suspend(uint32_t task_id);
void cli_task_init (void);

char *string_fill_buf(char* buf, uint32_t buf_len, const char *string);


static inline uint32_t task_self_id(void)
{
    return task_id_run;
}


#endif /* _TASK_UTIL_H_ */
