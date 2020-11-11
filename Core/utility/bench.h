/*
 * bench.h
 *
 * Created: 11/2/2020 2:32:53 PM
 *  Author: yuenw
 */ 

#ifndef _BENCH_H_
#define _BENCH_H_

#include <stdint.h>


void bench_speed(void);

// uint32_t bench_X0(uint32_t val);
uint32_t bench_X(uint32_t val);    // in bench.S


#endif /* _BENCH_H_ */
