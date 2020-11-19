/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
#include <stdio.h>

#include "stm32f4xx/main.h"
#include "sys_core/sys_timer.h"
#include "sys_core/sys_ticks.h"
#include "sys_core/sys_util.h"
#include "sys_device/dev_uart.h"
#include "kernel/task.h"
#include "kernel/task_ctx.h"
#include "kernel/sched.h"
#include "application/task_monitor.h"
#include "application/cmd_util.h"
#include "application/cli_util.h"
#include "examples/task_example.h"
#include "utility/bench.h"


static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void system_info_summary(void);
void system_bringup_test(void);
void MX_USB_OTG_FS_PCD_Init(void);


PCD_HandleTypeDef hpcd_USB_OTG_FS;


/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    task_sys_init(TASK_STACK_SIZE_3_0K);

    // ############################################
    // ##  Platform/Hardware related init .........
    // ############################################
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    /* Configure the system clock */
    SystemClock_Config();
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART3_UART_Init();
//  MX_USB_OTG_FS_PCD_Init();

    sys_timer_init();
    sys_timer_start();
    sysTickHandlerCnfg(1000, 5);

    // ############################################
    // ##  Basic kernel service tasks .............
    // ############################################
    task_id_monitor_display = task_create(monitor_display_task, "Display", PRIORITY_LOWEST + 2,
                                          TASK_STACK_SIZE_4_0K, NULL);
    task_id_monitor = task_create(monitor_task, "Monitor", PRIORITY_HIGHEST,
                                  TASK_STACK_SIZE_4_0K, (void *) task_id_monitor_display);
    task_create(cli_task, "CLI", PRIORITY_LOWEST + 2, TASK_STACK_SIZE_4_0K, NULL);

    // ############################################
    // ##  Application or User task() .............
    // ############################################
    task_timer_cnfg_t ex1;
    ex1.id = 0;
    ex1.period = 4;
    task_create(task_timer, "Exmp_1", PRIORITY_LOWEST + 3, TASK_STACK_SIZE_4_0K, &ex1);

    task_timer_cnfg_t ex2;
    ex2.id = 1;
    ex2.period = 4;
    task_create(task_timer, "Exmp_2", PRIORITY_LOWEST + 3, TASK_STACK_SIZE_4_0K, &ex2);

    task_timer_cnfg_t ex3;
    ex3.id = 2;
    ex3.period = 4;
    task_create(task_timer, "Exmp_3", PRIORITY_LOWEST + 3, TASK_STACK_SIZE_4_0K, &ex3);

    task_timer_cnfg_t ex4;
    ex4.id = 3;
    ex4.period = 4;
    task_create(task_timer, "Exmp_4", PRIORITY_LOWEST + 3, TASK_STACK_SIZE_4_0K, &ex4);

    // dump basic system information ..............
//  system_bringup_test();
    system_info_summary();
//  printf(" ...... %f\r\n", 3.14f);
    
    // ############################################
    // ##  Start SysTick, scheduler will run ......
    // ############################################
    sysTickStart();

    idle_loop(NULL);
}


/** **********************************************
  * @brief System Clock Configuration
  * @retval None
  * ********************************************** */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 384;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 8;
    RCC_OscInitStruct.PLL.PLLR = 2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
        |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
    PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48CLKSOURCE_PLLQ;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        Error_Handler();
    }
}


/** **********************************************
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  * ********************************************** */
void MX_USB_OTG_FS_PCD_Init(void)
{
    hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
    hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
    hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
    hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.battery_charging_enable = ENABLE;
    hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
    hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
    if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) {
        Error_Handler();
    }
}


/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
#define UART_BAUDRATE     115200
#define UART_PRIORITY     14

static void MX_USART3_UART_Init(void)
{
    // STM driver code config code ..... vvv
    stm_uart3.Instance          = USART3;
    stm_uart3.Init.BaudRate     = UART_BAUDRATE;
    stm_uart3.Init.WordLength   = UART_WORDLENGTH_8B;
    stm_uart3.Init.StopBits     = UART_STOPBITS_1;
    stm_uart3.Init.Parity       = UART_PARITY_NONE;
    stm_uart3.Init.Mode         = UART_MODE_TX_RX;
    stm_uart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    stm_uart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&stm_uart3) != HAL_OK) {
        printf("UART3 Config failed !!!!\r\n");
    }
    // STM driver code config code ..... ^^^

    // USER CODE BEGIN USART3_Init ..... vvvv
    // Enable IRQ when data is ready
    __HAL_UART_ENABLE_IT(&stm_uart3, UART_IT_RXNE);
    // Enable USART3 IRQ 
    HAL_NVIC_ClearPendingIRQ(USART3_IRQn);
    HAL_NVIC_SetPriority(USART3_IRQn, UART_PRIORITY, 0);    // NVIC_SetPriority();
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    // USER CODE END__ USART3_Init ..... ^^^^
}


/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  **/
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);
    
    /*Configure GPIO pin : USER_Btn_Pin */
    GPIO_InitStruct.Pin = USER_Btn_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
    GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
    GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : USB_OverCurrent_Pin */
    GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);
}


/** **********************************************
  *
  *
  * ********************************************** */
static void system_info_summary(void)
{
    printf("CPU Hz: %lu\r\n", cpu_ticks_per_sec);

    system_info_linker();
    uint32_t sp = (uint32_t) get_stack_ptr();
    printf("Stack Ptr: 0x%08lx\r\n", sp);
    system_info_heap();

    task_info_dump_all();
    task_fifo_dump_all();
    task_dump_pri_act_map();
    task_fifo_t *fifo = task_get_highest_from_pri_chain();
    printf(">>> Highest Priority FIFO %p <<<\r\n", fifo);
    task_fifo_dump(fifo);
    system_nvic_priority_dump();
}


/** **********************************************
  *
  *
  * ********************************************** */
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


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
