/*
 * sys_util.c
 *
 *  Created on: Oct 13, 2020
 *  Author: Y.W. Lee
 */

#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include "main.h"
#include "kernel/task.h"
#include "kernel/task_ctx.h"
#include "sys_core/sys_timer.h"
#include "utility/bench.h"
#include "examples/func_example.h"
#include "sys_util.h"


int32_t sys_disable_irq_req;

extern uint8_t _end;
extern uint8_t *__sbrk_heap_end;


void system_heap_update(uint8_t *heap_end)
{
//  _sbark_end = heap_end;
}


// ###########################################################################
// Information API
// ###########################################################################

void system_info_heap(void)
{
    uint8_t *__sbrk_heap_beg;
    uint32_t size;

    __sbrk_heap_beg = &_end;
    size = ((uint32_t) __sbrk_heap_end) - ((uint32_t) __sbrk_heap_beg);

    printf("Heap Info ....\r\n");
    printf("  __sbrk_heap_end: %p (__sbrk_heap_beg: %p, usuage: %lu)\r\n",
           __sbrk_heap_end, __sbrk_heap_beg, size);
}


// Text (in FLASH)
extern uint32_t g_pfnVectors;
extern uint32_t _stext;    // start of code
extern uint32_t _etext;    // end__ of code
extern uint32_t _srodata;  // start of constant
extern uint32_t _erodata;  // end__ of constant

// Data (in RAM)
extern uint32_t _sidata;   // start address for the initialization values of the .data section
extern uint32_t _sdata;    // start address for the .data section. defined in linker script
extern uint32_t _edata;    // end__ address for the .data section. defined in linker script
extern uint32_t _sbss;     // start address for the .bss section. defined in linker script
extern uint32_t _ebss;     // end__ address for the .bss section. defined in linker script
extern uint32_t _estack;   // end of the RAM, 

void system_info_mem(void)
{
    void *beg, *end;
    uint32_t size;
    
    // FLASH
    printf("FLASH Info\r\n");

    beg = (void *) &g_pfnVectors;
    end = (void *) &_stext - sizeof(uint32_t);
    size = ((uint32_t) end - (uint32_t) beg) + sizeof(uint32_t);
    printf("  vector: 0x%08lx - 0x%08lx  %7lu\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) &_stext;
    end = (void *) &_etext;
    size = ((uint32_t) end - (uint32_t) beg) + sizeof(uint32_t);
    printf("  text  : 0x%08lx - 0x%08lx  %7lu\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) &_srodata;
    end = (void *) &_erodata;
    size = ((uint32_t) end - (uint32_t) beg) + sizeof(uint32_t);
    printf("  rodata: 0x%08lx - 0x%08lx  %7lu\r\n", (uint32_t) beg, (uint32_t) end, size);

    // RAM
    printf("RAM Info\r\n");

    beg = (void *) &_sdata;
    end = (void *) &_edata;
    size = ((uint32_t) end - (uint32_t) beg) + sizeof(uint32_t);
    printf("  data  : 0x%08lx - 0x%08lx  %7lu\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) &_sbss;
    end = (void *) &_ebss;
    size = ((uint32_t) end - (uint32_t) beg);
    printf("  bss   : 0x%08lx - 0x%08lx  %7lu\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) &_end;
    end = (void *) __sbrk_heap_end;
    size = ((uint32_t) end - (uint32_t) beg);
    printf("  heap  : 0x%08lx - 0x%08lx  %7lu\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) __sbrk_heap_end;
    end = (void *) task_next_stack_start;
    size = ((uint32_t) end - (uint32_t) beg);
    printf("  free  : 0x%08lx - 0x%08lx  %7lu\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) task_next_stack_start;
    end = (void *) &_estack;
    size = ((uint32_t) end - (uint32_t) beg);
    printf("  stack : 0x%08lx - 0x%08lx  %7lu\r\n", (uint32_t) beg, (uint32_t) end, size);
}


void system_nvic_priority_dump(void)
{
    // Dump the Cortex-M3 Processor Exceptions' Priority
    printf("### Cortex-M3 Processor Exception Priority ###\r\n");
    printf("   Memory Mgmt: %lu\r\n", __NVIC_GetPriority(MemoryManagement_IRQn));
    printf("   Bus Fault  : %lu\r\n", __NVIC_GetPriority(BusFault_IRQn));
    printf("   Usage Fault: %lu\r\n", __NVIC_GetPriority(UsageFault_IRQn));
    printf("   SVCall     : %lu\r\n", __NVIC_GetPriority(SVCall_IRQn));
    printf("   Dbg Monitor: %lu\r\n", __NVIC_GetPriority(DebugMonitor_IRQn));
    printf("   PendSV     : %lu\r\n", __NVIC_GetPriority(PendSV_IRQn));
    printf("   SysTick    : %lu\r\n", __NVIC_GetPriority(SysTick_IRQn));
}


// ###########################################################################
// Testing/Verification API
// ###########################################################################

void mallocTest(void)
{
    uint32_t *ptr, size;
    size = 256;

    ptr = malloc(size);
    printf("Allocate memory: %p, size: 0x%08lx\r\n", ptr, size);
    system_info_heap();

    ptr = malloc(size);
    printf("Allocate memory: %p, size: 0x%08lx\r\n", ptr, size);
    system_info_heap();

    ptr = malloc(size);
    printf("Allocate memory: %p, size: 0x%08lx\r\n", ptr, size);
    system_info_heap();
}


void system_bringup_test(void)
{
    printf("\r\n\r\n\r\n");

    printf("sysClcokFreq: %lu\r\n", cpu_ticks_per_sec);

    volatile uint32_t count_A = sys_timer_get_inline();
    volatile uint32_t count_B = sys_timer_get_inline();
    printf("timer diff: %lu\r\n", (count_B - count_A));

    volatile uint32_t count;
    count = sys_timer_get_inline();
    printf("timer counter: %lu\r\n", count);
    count = sys_timer_get_inline();
    printf("timer counter: %lu\r\n", count);

    bench_speed();

    mallocTest();
}


void system_math_test(void)
{
    volatile float v0, v1, v2;
    volatile uint32_t t_beg, t_end, t_dlt;

    v0 = (3.141592653f / 3.0f);

    t_beg = sys_timer_get_inline();
    v1 = sinf(v0);
    v2 = asinf(v1);
    t_end = sys_timer_get_inline();
    t_dlt = t_end - t_beg;
    printf("val %f, sinf() %f, asinf() %f (%e)\r\n", v0, v1, v2, (v2 - v0));
    printf("t_beg %lu, t_end %lu t_diff %lu\r\n", t_beg, t_end, t_dlt);

    t_beg = sys_timer_get_inline();
    v1 = tanf(v0);
    v2 = atanf(v1);
    t_end = sys_timer_get_inline();
    t_dlt = t_end - t_beg;
    printf("val %f, tanf() %f, atanf() %f (%e)\r\n", v0, v1, v2, (v2 - v0));
    printf("t_beg %lu, t_end %lu t_diff %lu\r\n", t_beg, t_end, t_dlt);

    uint32_t n;
    v1 = v0;
    for (n = 0; n < 900; ++n) {
        v1 = api_sinf(v1);
    }
    printf("val %f, api_sinf %f (diff %e)\r\n", v0, v1, (v0 - v1));
}
