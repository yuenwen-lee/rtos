/*
 * sys_util.c
 *
 * Created: 10/13/2020 4:29:28 PM
 *  Author: yuenw
 */

#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include "main.h"
#include "kernel/task_ctx.h"
#include "sys_util.h"


int32_t sys_disable_irq_req;

extern uint8_t _end;
extern uint8_t *__sbrk_heap_end;


void system_heap_update(uint8_t *heap_end)
{
//  _sbark_end = heap_end;
}


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

void system_info_linker(void)
{
    void *beg, *end;
    uint32_t size;
    
    printf("Memory Section Info ......\r\n");

    beg = (void *) &g_pfnVectors;
    end = (void *) &_stext - sizeof(uint32_t);
    size = ((uint32_t) end - (uint32_t) beg) + sizeof(uint32_t);
    printf("  vector: 0x%08lx, 0x%08lx (%lu)\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) &_stext;
    end = (void *) &_etext;
    size = ((uint32_t) end - (uint32_t) beg) + sizeof(uint32_t);
    printf("  text  : 0x%08lx, 0x%08lx (%lu)\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) &_srodata;
    end = (void *) &_erodata;
    size = ((uint32_t) end - (uint32_t) beg) + sizeof(uint32_t);
    printf("  rodata: 0x%08lx, 0x%08lx (%lu)\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) &_sdata;
    end = (void *) &_edata;
    size = ((uint32_t) end - (uint32_t) beg) + sizeof(uint32_t);
    printf("  data  : 0x%08lx, 0x%08lx (%lu)\r\n", (uint32_t) beg, (uint32_t) end, size);

    beg = (void *) &_sbss;
    end = (void *) &_ebss;
    size = ((uint32_t) end - (uint32_t) beg);
    printf("  bss   : 0x%08lx, 0x%08lx (%lu)\r\n", (uint32_t) beg, (uint32_t) end, size);

    printf("  estack: 0x%08lx\r\n", (uint32_t) &_estack);
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
