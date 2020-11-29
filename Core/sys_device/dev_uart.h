/*
 * dev_uart.h
 * 
 * Created on: Nov 8, 2020
 *     Author: Y.W. Lee
 */

#ifndef _DRV_UART_H_
#define _DRV_UART_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"


#define ASCII_EXT   0x03     // end of text (CTL-C)
#define ASCII_BS    0x08     // backspace
#define ASCII_LF    0x0a     // line feed
#define ASCII_FF    0x0c     // form feed
#define ASCII_CR    0x0d     // carriage return
#define ASCII_DEL   0x7f     // delete


extern UART_HandleTypeDef stm_uart3;


void uart_handler_init(void);

static inline uint32_t uart_read_inline(UART_HandleTypeDef *hndl_p, uint8_t *p_byte)
{
    /* Check if the receiver is ready */
    if (__HAL_UART_GET_FLAG(hndl_p, USART_SR_RXNE) == 0) {
        return 1;   // return 1, if data is not received
    }
    /* Read character */
    *p_byte = (uint8_t) (hndl_p->Instance->DR & 0xFF);
    return 0;
}

static inline uint32_t uart_write_inline(UART_HandleTypeDef *hndl_p, uint8_t byte)
{
    /* Check if the transmitter is ready */
    if (__HAL_UART_GET_FLAG(hndl_p, USART_SR_TXE) == 0) {
        return 1;   // return 1, if data is not transferred to the shift register
    }
    /* Send character */
    hndl_p->Instance->DR = byte;
    return 0;
}

static inline int __io_putchar(int ch)
{
    while (uart_write_inline(&stm_uart3, ch));
    return ch;
}

static inline char __io_getchar(void)
{
    uint8_t c;
    while (uart_read_inline(&stm_uart3, &c));
    return (char) c;
}

#endif // _DRV_UART_H_
