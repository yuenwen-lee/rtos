/*
 * sys_timer.c
 * 
 *  Created on: Sep 6, 2013
 *      Author: Y.W. Lee
 */

#include <stdint.h>
#include "sys_timer.h"
#include "sys_device/dev_board.h"


#define TIM2_PRIORITY     1    // range fron 0 (Highest) ~ 15 (Lowest)


TIM_HandleTypeDef sys_timer_hndl;
uint32_t cpu_ticks_per_sec;
uint32_t cpu_ticks_per_msec;
uint32_t cpu_ticks_per_usec;

volatile uint32_t sys_timer_high = 0xFFFFFFFF;


void sys_timer_init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_IC_InitTypeDef sConfigIC = {0};

    sys_timer_hndl.Instance = TIM2;
    sys_timer_hndl.Init.Prescaler = 0;
    sys_timer_hndl.Init.CounterMode = TIM_COUNTERMODE_UP;
    sys_timer_hndl.Init.Period = 4294967295;  // 0xFFFFFFFF
    sys_timer_hndl.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    sys_timer_hndl.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&sys_timer_hndl) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&sys_timer_hndl, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_IC_Init(&sys_timer_hndl) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&sys_timer_hndl, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    if (HAL_TIM_IC_ConfigChannel(&sys_timer_hndl, &sConfigIC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    sys_timer_cpu_ticks_update();
}


void TIM2_IRQHandler(void)
{
    __HAL_TIM_CLEAR_IT(&sys_timer_hndl, TIM_IT_UPDATE);
    sys_timer_high++;
}


void sys_timer_start(void)
{
    // Enable TIM2_IRQn IRQ
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    HAL_NVIC_SetPriority(TIM2_IRQn, TIM2_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    /* Enable the TIM Update interrupt */
    __HAL_TIM_ENABLE_IT(&sys_timer_hndl, TIM_IT_UPDATE);
    HAL_TIM_Base_Start(&sys_timer_hndl);
}


void sys_timer_stop(void)
{
    HAL_TIM_Base_Stop(&sys_timer_hndl);
    /* Disable the TIM Update interrupt */
    __HAL_TIM_DISABLE_IT(&sys_timer_hndl, TIM_IT_UPDATE);
    // Disable TIM2_IRQn IRQ
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    HAL_NVIC_DisableIRQ(TIM2_IRQn);
}


void sys_timer_cpu_ticks_update(void)
{
    cpu_ticks_per_sec  = HAL_RCC_GetSysClockFreq();
    cpu_ticks_per_msec = cpu_ticks_per_sec / 1000;
    cpu_ticks_per_usec = cpu_ticks_per_sec/ (1000 * 1000);
}
