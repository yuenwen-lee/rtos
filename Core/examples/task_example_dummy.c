/*
 * task_example_dummy.c
 *
 * Created on: Unknown ...
 *     Author: Y.W. Lee
 */ 

#include <stdint.h>
#include <stdio.h>
#include "task_example.h"


volatile uint32_t counter_AA;


uint32_t task_dummy_AA(void *argv)
{
    counter_AA++;
    while (1) {
        counter_AA++;
    }

    return 0;
}


void task_dummy_AA_dump(void)
{
    printf("    Task_dummy_AA_dump(): %lu\r\n", counter_AA);
}
