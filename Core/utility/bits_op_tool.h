/*
 * bits_op_tool.h
 *
 * Created on: Sep 8, 2013
 *     Author: Y.W. Lee
 */

#ifndef _BITS_OP_TOOL_H_
#define _BITS_OP_TOOL_H_


static inline uint32_t find_msb_loc_fast(uint32_t data)
{
    uint32_t numb;

    __asm volatile ("clz %0, %1":"=r" (numb):"r"(data));
    return (31 - numb);

}

uint32_t find_msb_loc(uint32_t data);


#endif /* _BITS_OP_TOOL_H_ */
