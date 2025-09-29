#include "app/app.h"
#include "app/display.h"
#include "app/sensor.h"
#include "stdlib.h"
#include <math.h>

static bool check_if_anything_changed_locally(app_handle *handle)
{
    const float temp_threshold = 0.5F; // degrees Celsius
    const float hum_threshold = 1.0F;  // percentage
    const int32_t pres_threshold = 10; // Pascals
    const int32_t bat_threshold = 100; // millivolts

    if (fabsf(handle->local.temperature - handle->last_local.temperature) >= temp_threshold)
    {
        return true;
    }

    if (fabsf(handle->local.humidity - handle->last_local.humidity) >= hum_threshold)
    {
        return true;
    }

    if (abs(handle->local.pressure - handle->last_local.pressure) >= pres_threshold)
    {
        return true;
    }

    if (abs(handle->local.bat_in - handle->last_local.bat_in) >= bat_threshold)
    {
        return true;
    }

    return false;
}

static void update_last_local_data(app_handle *handle)
{
    handle->last_local.temperature = handle->local.temperature;
    handle->last_local.humidity = handle->local.humidity;
    handle->last_local.pressure = handle->local.pressure;
    handle->last_local.bat_in = handle->local.bat_in;
}

static void init_station_data(station_data *data)
{
    data->temperature = 0;
    data->humidity = 0;
    data->pressure = 0;
    data->bat_in = 0;
}

void app_init(app_handle *handle)
{
    handle->battery = battery_create();
    handle->hclock = hourly_clock_create(&hrtc);
    handle->radio = radio_create(RAD_CS_GPIO_Port, RAD_CS_Pin, RAD_DIO0_GPIO_Port, RAD_DIO0_Pin, &hspi3);
    handle->spi_mgr = spi_bus_manager_create(&hspi2, handle->app_spiq_storage, (uint16_t)(sizeof(handle->app_spiq_storage) / sizeof(handle->app_spiq_storage[0])));
    handle->sensor = sensor_create(&handle->spi_mgr);
    handle->display = display_create(&handle->spi_mgr);

    init_station_data(&handle->local);
    init_station_data(&handle->remote);
    init_station_data(&handle->last_local);
    init_station_data(&handle->last_remote);

    handle->last_sensor_read_time = 0;
    handle->last_battery_read_time = 0;
    handle->last_check_changes_time = 0;

    display_init(&handle->display);

    sensor_init(&handle->sensor);

    // battery_request_read(&handle->battery);

    // radio_init(&handle->radio);

    // Initialize timing timestamps
    handle->last_sensor_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_battery_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_check_changes_time = hourly_clock_get_timestamp(&handle->hclock);
}

// Plan:
// - Radio na DMA
// - Obsługa błędów na wyświetlaczu
// - Dodać usypianie i budzenie co np. 9s (RTC wakeup?)
// - Przerobić na dwa projekty i wspólny kod między nimi

// // Release
// #define SENSOR_CHECK_EVERY_SEC 60
// #define BATTERY_CHECK_EVERY_SEC 600
// #define DISPLAY_CHECK_CHANGES_EVERY_SEC 120

// Debug
#define SENSOR_CHECK_EVERY_SEC 2
#define BATTERY_CHECK_EVERY_SEC 2
#define DISPLAY_CHECK_CHANGES_EVERY_SEC 4

void app_loop(app_handle *handle)
{
    hourly_clock_update(&handle->hclock);
    // radio_loop(&handle->radio);
    sensor_try_get(&handle->sensor, &handle->local);

    if (hourly_clock_check_elapsed(&handle->hclock, handle->last_sensor_read_time, SENSOR_CHECK_EVERY_SEC))
    {
        sensor_kick(&handle->sensor);

        handle->last_sensor_read_time = hourly_clock_get_timestamp(&handle->hclock);
        battery_update_temperature(&handle->battery, handle->local.temperature);
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

    bool changes_detected = false;

    if (hourly_clock_check_elapsed(&handle->hclock, handle->last_check_changes_time, DISPLAY_CHECK_CHANGES_EVERY_SEC))
    {
        changes_detected = check_if_anything_changed_locally(handle);
        handle->last_check_changes_time = hourly_clock_get_timestamp(&handle->hclock);

        if (changes_detected)
        {
            update_last_local_data(handle);
        }
    }

    display_loop(&handle->display, &handle->local, NULL, changes_detected);

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
