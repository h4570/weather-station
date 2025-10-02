#include "app/app.h"
#include "stdlib.h"

void app_init(app_handle *handle)
{
    // handle->battery = battery_create(&hadc);
    handle->hclock = hourly_clock_create(&hrtc);
    handle->radio = radio_create(RAD_CS_GPIO_Port, RAD_CS_Pin, RAD_DI0_GPIO_Port, RAD_DI0_Pin, &hspi1);
    handle->spi_mgr = spi_bus_manager_create(&hspi1, handle->app_spiq_storage, (uint16_t)(sizeof(handle->app_spiq_storage) / sizeof(handle->app_spiq_storage[0])));
    // handle->sensor = sensor_create(&handle->spi_mgr, &htim2, &hspi1, BME280_CS_GPIO_Port, BME280_CS_Pin);

    // sensor_init(&handle->sensor);

    // battery_request_read(&handle->battery);

    radio_init(&handle->radio);

    handle->last_sensor_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_battery_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_radio_send_time = hourly_clock_get_timestamp(&handle->hclock);
}

// // Release
// #define SENSOR_CHECK_EVERY_SEC 60
// #define BATTERY_CHECK_EVERY_SEC 600
// #define RADIO_SEND_EVERY_SEC 600

// Debug
#define SENSOR_CHECK_EVERY_SEC 2
#define BATTERY_CHECK_EVERY_SEC 2
#define RADIO_SEND_EVERY_SEC 2

void app_loop(app_handle *handle)
{
    hourly_clock_update(&handle->hclock);
    radio_loop(&handle->radio);
    // sensor_try_get(&handle->sensor, &handle->local);

    // if (hourly_clock_check_elapsed(&handle->hclock, handle->last_sensor_read_time, SENSOR_CHECK_EVERY_SEC))
    // {
    //     sensor_kick(&handle->sensor);

    //     handle->last_sensor_read_time = hourly_clock_get_timestamp(&handle->hclock);
    //     battery_update_temperature(&handle->battery, handle->local.temperature);
    // }

    if (hourly_clock_check_elapsed(&handle->hclock, handle->last_radio_send_time, RADIO_SEND_EVERY_SEC))
    {
        // DBG:
        handle->local.temperature = 12.3F;
        handle->local.humidity = 45.6F;
        handle->local.pressure = 101325;
        handle->local.bat_in = 3300;

        radio_send(&handle->radio, &handle->local);
        handle->last_radio_send_time = hourly_clock_get_timestamp(&handle->hclock);
    }

    // if (hourly_clock_check_elapsed(&handle->hclock, handle->last_battery_read_time, BATTERY_CHECK_EVERY_SEC))
    // {
    //     battery_refresh(&handle->battery);

    //     if (battery_check_if_level_changed(&handle->battery))
    //     {
    //         handle->local.bat_in = battery_get_level(&handle->battery);
    //     }

    //     battery_request_read(&handle->battery);
    //     handle->last_battery_read_time = hourly_clock_get_timestamp(&handle->hclock);
    // }

    HAL_Delay(1);
}

void app_adc_conv_cplt_callback(app_handle *handle, ADC_HandleTypeDef *hadc)
{
    battery_adc_interrupt_handler(hadc);
}

void app_gpio_exti_callback(app_handle *handle, const uint16_t pin)
{
    radio_exti_interrupt_handler(pin);
}

void app_spi_tx_cplt_callback(app_handle *handle, SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_tx_cplt(&handle->spi_mgr, hspi);
}

void app_spi_tx_half_cplt_callback(app_handle *handle, SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_tx_half(&handle->spi_mgr, hspi);
}

void app_spi_txrx_cplt_callback(app_handle *handle, SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_txrx_cplt(&handle->spi_mgr, hspi);
}

void app_spi_txrx_half_cplt_callback(app_handle *handle, SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_txrx_half(&handle->spi_mgr, hspi);
}

void app_spi_error_callback(app_handle *handle, SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_error(&handle->spi_mgr, hspi);
}
