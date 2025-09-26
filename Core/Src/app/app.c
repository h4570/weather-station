#include "app/app.h"
#include "app/display.h"
#include "app/sensor.h"

static bool check_if_anything_changed_locally(app_handle *handle)
{
    if (handle->local.temperature != handle->last_local.temperature ||
        handle->local.humidity != handle->last_local.humidity ||
        handle->local.pressure != handle->last_local.pressure ||
        handle->local.bat_in != handle->last_local.bat_in)
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

app_handle app_create()
{
    app_handle handle;

    handle.display = display_create();
    handle.battery = battery_create();
    handle.hclock = hourly_clock_create(&hrtc);

    init_station_data(&handle.local);
    init_station_data(&handle.remote);
    init_station_data(&handle.last_local);
    init_station_data(&handle.last_remote);

    handle.last_sensor_read_time = 0;
    handle.last_battery_read_time = 0;
    handle.last_check_changes_time = 0;

    return handle;
}

void app_init(app_handle *handle)
{
    display_init(&handle->display);

    sensor_init();
    sensor_read(&handle->local);

    battery_request_read(&handle->battery);

    // Initialize timing timestamps
    handle->last_sensor_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_battery_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_check_changes_time = hourly_clock_get_timestamp(&handle->hclock);
}

// Plan:
// - Dodać radio do układu i napisać sterownik na przerwaniach do odbioru danych
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

    if (hourly_clock_check_elapsed(&handle->hclock, handle->last_sensor_read_time, SENSOR_CHECK_EVERY_SEC))
    {
        sensor_read(&handle->local);
        handle->last_sensor_read_time = hourly_clock_get_timestamp(&handle->hclock);
        battery_update_temperature(&handle->battery, handle->local.temperature);
    }

    if (hourly_clock_check_elapsed(&handle->hclock, handle->last_battery_read_time, BATTERY_CHECK_EVERY_SEC))
    {
        battery_refresh(&handle->battery);

        if (battery_check_if_level_changed(&handle->battery))
        {
            handle->local.bat_in = battery_get_level(&handle->battery);
        }

        battery_request_read(&handle->battery);
        handle->last_battery_read_time = hourly_clock_get_timestamp(&handle->hclock);
    }

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

    HAL_Delay(100);
}

void app_adc_interrupt_handler(app_handle *handle, ADC_HandleTypeDef *hadc)
{
    battery_adc_interrupt_handler(hadc);
}