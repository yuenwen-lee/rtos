/*
 * dev_board.c
 *
 * Created on: Nov 11, 2020
 *     Author: Y.W. Lee
 */

#include <stdint.h>
#include "kernel/sleep.h"
#include "dev_board.h"


void board_blink_led_cli(void)
{
    BSP_LED_On(LED2);
    sleep_msec(50);
    BSP_LED_Off(LED2);
}
