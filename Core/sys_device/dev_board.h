/*
 * dev_board.h
 *
 * Created on: Nov 11, 2020
 *     Author: Y.W. Lee
 */

#ifndef _DEV_BOARD_H_
#define _DEV_BOARD_H_

#include "stm32f4xx/stm32f4xx_nucleo_144.h"


static inline void board_toggle_led_monitor(void)
{
    BSP_LED_Toggle(LED3);
}

static inline void board_toggle_led_cli(void)
{
    BSP_LED_Toggle(LED2);
}

static inline void board_toggle_led_systck(void)
{
    BSP_LED_Toggle(LED1);
}


void board_blink_led_cli(void);


#endif /* _DEV_BOARD_H_ */
