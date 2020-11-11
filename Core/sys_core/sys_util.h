/*
 * sys_util.h
 *
 * Created: 10/17/2020 12:01:15 PM
 *  Author: yuenw
 */

#ifndef _SYS_UTIL_H_
#define _SYS_UTIL_H_


void system_heap_update(uint8_t *heap_end);
void system_info_heap(void);
void system_info_linker(void);
void system_nvic_priority_dump(void);

// ###########################################################################
// Testing/Verification API
// ###########################################################################

void mallocTest(void);


#endif /* _SYS_UTIL_H_ */
