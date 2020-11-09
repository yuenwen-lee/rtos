/*
 * bench.c
 *
 * Created: 11/2/2020 2:32:34 PM
 *  Author: yuenw
 */ 

#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "bench.h"


static uint32_t bench_delay(TIM_TypeDef *tm, uint32_t t_msec)
{
  volatile uint32_t tick_beg, tick_end;
  volatile uint32_t tick_delt;
  volatile uint32_t count = 0;

  tick_delt = t_msec * (HAL_RCC_GetSysClockFreq() / 1000);
  tick_beg = tm->CNT;
  tick_end = tick_beg + tick_delt;

  while ((int32_t) (tick_end - tm->CNT) > 0) {
    count++;
  }
  volatile uint32_t tick_now = tm->CNT;

  printf("   beg %lu --> end %lu, (now %lu, delt %lu)\r\n",
		 tick_beg, tick_end, tick_now, tick_delt);
  return count;
}


#if 0   // .....................................................
extern int api_A(int a0, int b0);

uint32_t bench_X0(uint32_t val)
{
  uint32_t sum, n;

  for (n = sum = 0; n < (250 * 2000); ++n) {
    sum += api_A(1, val);
  }

  return sum;
}
#endif  // .....................................................


static void bench_cyle(TIM_TypeDef *tm)
{
  // measure the latency to do some simple math 500000 times ....
  uint32_t t_beg = tm->CNT;
  uint32_t sum = bench_X(5);
  uint32_t t_end = tm->CNT;
  uint32_t t_dlt = t_end - t_beg;
  printf("................. t_beg %lu, t_end %lu, diff %lu (sum %lu)\r\n",
  t_beg, t_end, t_dlt, sum);
}


void bench_speed(TIM_TypeDef *tm)
{
	// Measure the 15 sec delay ...
	printf(".................!! START !!..............\r\n");
	bench_delay(tm, 15 * 1000);
	printf(".................!! _END_ !!..............\r\n");

	// measure the latency to do some simple math 500000 times ....
	for (uint32_t n = 0; n < 4; ++n) {
	  bench_delay(tm, (2 * 1000));
	  bench_cyle(tm);
	}
}
