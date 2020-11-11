/*
 * drv_uart.c
 * 
 * Created: 11/8/2020 08:01 PM
 *  Author: Wayne Lee
 */

#include <stdio.h>
#include "dev_uart.h"


#define UART_BAUDRATE     115200
#define UART_PRIORITY     14

#define DRV_BUF_LEN       256
#define DRV_SEG_LEN        64


UART_HandleTypeDef stm_uart3;
uint32_t uart_count_false_rdy;
uint32_t uart_count_byte_count;


/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
void uart_handler_init(void)
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


static void uart_handler_core(uint8_t c)
{
    if (c == ASCII_DEL || c == ASCII_BS) {
        // ASCII sequence of remove the previous character on console ....
        while (uart_write_inline(&stm_uart3, '\b'));
        while (uart_write_inline(&stm_uart3, '\e'));
        while (uart_write_inline(&stm_uart3, '['));
        while (uart_write_inline(&stm_uart3, '0'));
        while (uart_write_inline(&stm_uart3, 'K'));

    } else if (c == ASCII_EXT) {
        // if press CTL-C

    } else {
        while (uart_write_inline(&stm_uart3, c));
        if (c == ASCII_CR) {
            while (uart_write_inline(&stm_uart3, ASCII_LF));
        }
    }
}


void USART3_IRQHandler(void)
{
    uint8_t byte;

    if (uart_read_inline(&stm_uart3, &byte) == 0) {
        uart_handler_core(byte);
        uart_count_byte_count++;
    } else {
        uart_count_false_rdy++;
    }
}
