#pragma once

#include "stm32l0xx_hal.h"
#include "gpio.h"
#include "tim.h"
#include "i2c.h"

#ifdef __cplusplus
extern "C"
{
#endif

//
//	Settings
//	Choose sensor
//
#define BME280
// #define BMP_SPI 1
#define BMP_I2C 1

    // Nucleo G474RE - 170MHz | Prescaler 169, Counter period 9999 -> 1us
    // Nucleo L010K8 - 32MHz | Prescaler 31, Counter period 9999 -> 1us
    extern TIM_HandleTypeDef *bmpxx80_1_us_timer;

#ifdef __cplusplus
}
#endif
