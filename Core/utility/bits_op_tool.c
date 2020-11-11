/*
 * bits_op_tool.c
 *
 * Created: 10/17/2020 5:03:50 PM
 *  Author: yuenw
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
