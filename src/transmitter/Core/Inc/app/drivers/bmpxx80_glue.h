#pragma once

#include "stm32l0xx_hal.h"
#include "gpio.h"
#include "tim.h"
#include "spi.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Nucleo G474RE - 170MHz | Prescaler 169, Counter period 9999 -> 1us
    // Nucleo L010K8 - 32MHz | Prescaler 31, Counter period 9999 -> 1us
    extern TIM_HandleTypeDef *bmpxx80_1_us_timer;

#ifdef __cplusplus
}
#endif
