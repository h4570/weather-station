#pragma once

#include <stdint.h>

#include "app/display.h"
#include "app/hourly_clock.h"
#include "app/battery.h"
#include "app/radio.h"

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
        hourly_clock_handle hclock;
        battery_handle battery;
        radio_handle radio;
        station_data local, remote, last_local, last_remote;
        hourly_clock_timestamp_t last_sensor_read_time;
        hourly_clock_timestamp_t last_battery_read_time;
        hourly_clock_timestamp_t last_check_changes_time;
    } app_handle;

    app_handle app_create();
    void app_init(app_handle *handle);
    void app_loop(app_handle *handle);
    void app_adc_interrupt_handler(app_handle *handle, ADC_HandleTypeDef *hadc);
    void app_exti_interrupt_handler(const uint16_t pin);
#ifdef __cplusplus
}
#endif
