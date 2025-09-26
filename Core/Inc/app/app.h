#pragma once

#include <stdint.h>

#include "app/display.h"

#include "stm32g4xx_hal.h"
#include "adc.h"
#include "gpio.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Handle structure for the application
     */
    typedef struct
    {
        display_handle display;
    } app_handle;

    app_handle app_create();
    void app_init(app_handle *handle);
    void app_loop(app_handle *handle);
    void app_adc_interrupt_handler(app_handle *handle, ADC_HandleTypeDef *hadc);

#ifdef __cplusplus
}
#endif
