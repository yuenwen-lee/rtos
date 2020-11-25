/*
 * board_ctrl.c
 *
 * Created: 11/2/2020 6:57:32 PM
 *  Author: yuenw
 */

#include <stdint.h>
#include "kernel/sleep.h"
#include "stm32f4xx/stm32f4xx_nucleo_144.h"
#include "dev_board.h"

void board_toggle_led_monitor(void)
{
    BSP_LED_Toggle(LED3);
}


void board_blink_led_cli(void)
{
    BSP_LED_On(LED2);
    sleep_msec(100);
    BSP_LED_Off(LED2);
}
