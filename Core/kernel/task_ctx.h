/*
 * task_ctx.h
 *
 * Created: 10/9/2020 12:08:14 PM
 *  Author: Wayne Lee
 */

#ifndef _TASK_CTX_H_
#define _TASK_CTX_H_

#include "main.h"

int get_stack_ptr(void);
int get_sp_main(void);
int get_sp_process(void);

void gen_pend_sv_irq(void);


static inline void set_pendsv_irq_priority(uint32_t pri)
{
    __NVIC_SetPriority(PendSV_IRQn, pri);
}


#endif /* _TASK_CTX_H_ */
