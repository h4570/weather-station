#include "app/app.h"
#include "app/display.h"
#include "app/drivers/bmpxx80.h"
#include "app/bat_nimh_calc.h"

volatile uint16_t bat_vcc_adc_value;

// // Release
// #define SENSOR_CHECK_EVERY_SEC 60
// #define BATTERY_CHECK_EVERY_SEC 600
// #define DISPLAY_CHECK_CHANGES_EVERY_SEC 120

// Debug
#define SENSOR_CHECK_EVERY_SEC 2
#define BATTERY_CHECK_EVERY_SEC 2
#define DISPLAY_CHECK_CHANGES_EVERY_SEC 4

static void init_sensor()
{
    HAL_TIM_Base_Start(&htim1);
    BMPxx_init(BME280_CS_GPIO_Port, BME280_CS_Pin);
    BME280_Init(&hspi2, BME280_TEMPERATURE_16BIT, BME280_PRESSURE_ULTRALOWPOWER, BME280_HUMIDITY_STANDARD, BME280_FORCEDMODE);
    BME280_SetConfig(BME280_STANDBY_MS_10, BME280_FILTER_OFF);
}

static void request_battery_read()
{
    HAL_ADC_Start_IT(&hadc1);
}

static void read_sensor(app_handle *handle)
{
    BME280_ReadTemperatureAndPressureAndHumidity(&handle->local.temperature, &handle->local.pressure, &handle->local.humidity);
}

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

static void calc_bat_in_if_changed(app_handle *handle)
{
    if (bat_vcc_adc_value != handle->last_bat_vcc_adc_value)
    {
        handle->last_bat_vcc_adc_value = bat_vcc_adc_value;

        // 2x AA battery, voltage divider 100k/100k, ADC 12-bit, Vref = 3.3V
        float battery_voltage = bat_nimh_calc_get_voltage_for_soc(bat_vcc_adc_value, 3.3F, 2.0F);
        handle->local.bat_in = bat_nimh_calc_execute(battery_voltage, handle->local.temperature);
    }
}

// Plan:
// - Dodać radio do układu i napisać sterownik na przerwaniach do odbioru danych
// - Dodać usypianie i budzenie co np. 9s (RTC wakeup?)
// - Przerobić na dwa projekty i wspólny kod między nimi

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
    handle.hclock = hourly_clock_create(&hrtc);

    init_station_data(&handle.local);
    init_station_data(&handle.remote);
    init_station_data(&handle.last_local);
    init_station_data(&handle.last_remote);

    handle.anything_changed = true;

    handle.last_sensor_read_time = 0;
    handle.last_battery_read_time = 0;
    handle.last_check_changes_time = 0;

    handle.last_bat_vcc_adc_value = 0;

    return handle;
}

void app_init(app_handle *handle)
{
    display_init(&handle->display);

    init_sensor();
    request_battery_read();
    read_sensor(handle);

    // Initialize timing timestamps
    handle->last_sensor_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_battery_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_check_changes_time = hourly_clock_get_timestamp(&handle->hclock);
}

void app_loop(app_handle *handle)
{
    hourly_clock_update(&handle->hclock);

    calc_bat_in_if_changed(handle);

    if (hourly_clock_check_elapsed(&handle->hclock, handle->last_sensor_read_time, SENSOR_CHECK_EVERY_SEC))
    {
        read_sensor(handle);
        handle->last_sensor_read_time = hourly_clock_get_timestamp(&handle->hclock);
    }

    if (hourly_clock_check_elapsed(&handle->hclock, handle->last_battery_read_time, BATTERY_CHECK_EVERY_SEC))
    {
        request_battery_read();
        handle->last_battery_read_time = hourly_clock_get_timestamp(&handle->hclock);
    }

    if (hourly_clock_check_elapsed(&handle->hclock, handle->last_check_changes_time, DISPLAY_CHECK_CHANGES_EVERY_SEC))
    {
        handle->anything_changed = check_if_anything_changed_locally(handle);
        handle->last_check_changes_time = hourly_clock_get_timestamp(&handle->hclock);
    }

    display_loop(&handle->display, &handle->local, NULL, handle->anything_changed);

    if (handle->anything_changed)
    {
        handle->anything_changed = false;
    }

    HAL_Delay(100);
}

void app_adc_interrupt_handler(app_handle *handle, ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        bat_vcc_adc_value = HAL_ADC_GetValue(hadc);
    }
}