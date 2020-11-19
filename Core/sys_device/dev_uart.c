/*
 * drv_uart.c
 * 
 * Created: 11/8/2020 08:01 PM
 *  Author: Wayne Lee
 */

#include <stdio.h>

#include "kernel/task.h"
#include "kernel/sem.h"
#include "kernel/ring_buf_event.h"
#include "kernel/run_stat.h"
#include "dev_uart.h"


#define DRV_BUF_LEN       256
#define DRV_SEG_LEN        64


UART_HandleTypeDef stm_uart3;

ring_buf_event_t uart_buf_event;
static char uart_rx_drv_buf[DRV_BUF_LEN];

run_stat_que_t run_stat_que_uart0;
run_stat_t run_stat_uart0;

uint32_t uart_count_false_rdy;
uint32_t uart_count_byte_count;


/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
void uart_handler_init(void)
{
    ring_buf_event_init(&uart_buf_event, uart_rx_drv_buf, DRV_BUF_LEN);
    run_stat_register_isr("uart0", 0, &run_stat_uart0, &run_stat_que_uart0);
}


static void uart_handler_core(uint8_t c)
{
    uint32_t  left;

    if (ring_buf_event_seg_data_full(&uart_buf_event)) {
        if (ring_buf_event_seg_get(&uart_buf_event, DRV_SEG_LEN) == NULL) {
            return;
        }
    }

    if (c == ASCII_DEL || c == ASCII_BS) {
        if (!ring_buf_event_seg_data_empty(&uart_buf_event)) {
            ring_buf_event_clear_char(&uart_buf_event);
            // ASCII sequence of remove the previous character on console ....
            while (uart_write_inline(&stm_uart3, '\b'));
            while (uart_write_inline(&stm_uart3, '\e'));
            while (uart_write_inline(&stm_uart3, '['));
            while (uart_write_inline(&stm_uart3, '0'));
            while (uart_write_inline(&stm_uart3, 'K'));
        }
        return;

    } else if (c == ASCII_EXT) {
        // if press CTL-C
        cli_cb_run_stat_root_load_stop();

    } else {
        while (uart_write_inline(&stm_uart3, c));
        if (c == ASCII_CR) {
            while (uart_write_inline(&stm_uart3, ASCII_LF));
        }

        left = ring_buf_event_write_char(&uart_buf_event, c);
        if (left == 0 || c == ASCII_CR) {
            ring_buf_event_post(&uart_buf_event);
        }
    }
}


void USART3_IRQHandler(void)
{
    uint8_t byte;

    run_time_stack_push_isr(1);

    if (uart_read_inline(&stm_uart3, &byte) == 0) {
        uart_handler_core(byte);
        uart_count_byte_count++;
    } else {
        uart_count_false_rdy++;
    }

    run_stat_uart0.run_counter++;
    run_stat_uart0.time_ttl += run_time_stack_pop_isr(1);
}
