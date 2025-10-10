#pragma once

#include <stdint.h>

#include "shared/hourly_clock.h"
#include "shared/battery.h"
#include "app/sensor.h"
#include "app/radio.h"

#include "stm32l0xx_hal.h"
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
        void (*system_clock_config_func)(void); /**< Function pointer to system clock configuration function */
        hourly_clock_handle hclock;
        battery_handle battery;
        sensor_handle sensor;
        radio_handle radio;
        app_device_data local, last_local;
        hourly_clock_timestamp_t last_battery_read_time;
        hourly_clock_timestamp_t last_radio_send_time;
    } app_handle;

    void app_init(app_handle *handle, void (*system_clock_config_func)(void));
    void app_loop(app_handle *handle);

    void app_adc_conv_cplt_callback(app_handle *handle, ADC_HandleTypeDef *hadc);
    void app_gpio_exti_callback(app_handle *handle, const uint16_t pin);

#ifdef __cplusplus
}
#endif
