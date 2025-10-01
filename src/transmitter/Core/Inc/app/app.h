#pragma once

#include <stdint.h>

#include "shared/drivers/spi_bus_manager.h"
#include "shared/hourly_clock.h"
#include "shared/battery.h"
#include "shared/sensor.h"
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
        hourly_clock_handle hclock;
        battery_handle battery;
        sensor_handle sensor;
        radio_handle radio;
        spi_bus_manager spi_mgr;
        spi_bus_transaction app_spiq_storage[8];
        app_device_data local, last_local;
        hourly_clock_timestamp_t last_sensor_read_time;
        hourly_clock_timestamp_t last_battery_read_time;
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
