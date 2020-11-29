/*
 * task_ctx.h
 *
 *  Created on: Feb 11, 2016
 *      Author: Y.W. Lee
 */

#ifndef _TASK_CTX_H_
#define _TASK_CTX_H_

#include "main.h"


int get_stack_ptr(void);
int get_sp_main(void);
int get_sp_process(void);
uint32_t push_fp(void);

void gen_pend_sv_irq(void);
void gen_svc_irq(uint32_t arg);


static inline void set_pendsv_irq_priority(uint32_t pri)
{
    __NVIC_SetPriority(PendSV_IRQn, pri);
}


#endif /* _TASK_CTX_H_ */
