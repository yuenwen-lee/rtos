/*
 * stm32f4xx_hal_ovw.c.c
 * 
 * Created: 10/13/2020 2:47:49 PM
 *  Author: yuenw
 */

/*
 *  This file contains the APIs that overlap the default HAL API under
 *  Drivers/STM32F4xx_HAL_Driver/Src, such as stm32f4xx_hal.c
 */

#include <stdint.h>
#include "stm32f4xx/main.h"


uint32_t ov_HAL_InitTick;


/*  Overlaps the API HAL_InitTick() defined as __weak in
 *  Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    ov_HAL_InitTick++;
    return HAL_OK;
}

