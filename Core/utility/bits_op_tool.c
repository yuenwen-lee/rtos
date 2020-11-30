/*
 * bits_op_tool.c
 *
 * Created on: Sep 8, 2013
 *     Author: Y.W. Lee
 */

#include <stdint.h>
#include "bits_op_tool.h"


uint32_t find_msb_loc(uint32_t data)
{
    uint32_t loc;

    loc = 0;

    while (data >>= 1)
        loc++;
    return loc;
}
