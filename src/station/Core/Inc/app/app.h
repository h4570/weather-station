#pragma once

#include <stdint.h>

#include "app/display.h"
#include "app/hourly_clock.h"
#include "app/battery.h"
#include "app/radio.h"
#include "app/sensor.h"
#include "shared/drivers/spi_bus_manager.h"

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
        sensor_handle sensor;
        spi_bus_manager spi_mgr;
        spi_bus_transaction app_spiq_storage[64];
        station_data local, remote, last_local, last_remote;
        hourly_clock_timestamp_t last_sensor_read_time;
        hourly_clock_timestamp_t last_battery_read_time;
        hourly_clock_timestamp_t last_check_changes_time;
    } app_handle;

    void app_init(app_handle *handle);
    void app_loop(app_handle *handle);

    void app_adc_conv_cplt_callback(app_handle *handle, ADC_HandleTypeDef *hadc);
    void app_gpio_exti_callback(app_handle *handle, const uint16_t pin);
    void app_spi_tx_cplt_callback(app_handle *handle, SPI_HandleTypeDef *hspi);
    void app_spi_tx_half_cplt_callback(app_handle *handle, SPI_HandleTypeDef *hspi);
    void app_spi_txrx_cplt_callback(app_handle *handle, SPI_HandleTypeDef *hspi);
    void app_spi_txrx_half_cplt_callback(app_handle *handle, SPI_HandleTypeDef *hspi);
    void app_spi_error_callback(app_handle *handle, SPI_HandleTypeDef *hspi);

#ifdef __cplusplus
}
#endif
