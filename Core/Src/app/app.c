#include "app/app.h"
#include "app/display.h"
#include "app/drivers/bmpxx80.h"
#include "app/bat_nimh_calc.h"
#include "app/hourly_clock.h"

// === Business logic variables
volatile uint16_t bat_vcc_adc_value, last_bat_vcc_adc_value;
float temperature = 0, last_temperature = 0, humidity = 0, last_humidity = 0;
int32_t pressure = 0, last_pressure = 0, bat_in = 0, last_bat_in = 0;
bool anything_changed = true;
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;
hourly_clock_handle hclock;
hourly_clock_timestamp_t last_sensor_read_time = 0;
hourly_clock_timestamp_t last_battery_read_time = 0;
hourly_clock_timestamp_t last_check_changes_time = 0;

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

static void read_sensor()
{
    BME280_ReadTemperatureAndPressureAndHumidity(&temperature, &pressure, &humidity);
}

static bool check_if_anything_changed()
{
    if (temperature != last_temperature || humidity != last_humidity || pressure != last_pressure || bat_in != last_bat_in)
    {
        last_temperature = temperature;
        last_humidity = humidity;
        last_pressure = pressure;
        last_bat_in = bat_in;
        return true;
    }

    return false;
}

static void calc_bat_in_if_changed()
{
    if (bat_vcc_adc_value != last_bat_vcc_adc_value)
    {
        last_bat_vcc_adc_value = bat_vcc_adc_value;
        // 2x AA battery, voltage divider 100k/100k, ADC 12-bit, Vref = 3.3V
        float battery_voltage = bat_nimh_calc_get_voltage_for_soc(bat_vcc_adc_value, 3.3F, 2.0F);
        bat_in = bat_nimh_calc_execute(battery_voltage, temperature);
        last_bat_in = bat_in;
    }
}

// Plan:
// - Dodać radio do układu i napisać sterownik na przerwaniach do odbioru danych
// - Dodać usypianie i budzenie co np. 9s (RTC wakeup?)
// - Przerobić na dwa projekty i wspólny kod między nimi

app_handle app_create()
{
    app_handle handle = {};

    return handle;
}

void app_init(app_handle *handle)
{
    handle->display = display_create();
    display_init(&handle->display);

    init_sensor();
    request_battery_read();
    read_sensor();

    hclock = hourly_clock_create(&hrtc, &sTime, &sDate);

    // Initialize timing timestamps
    last_sensor_read_time = hourly_clock_get_timestamp(&hclock);
    last_battery_read_time = hourly_clock_get_timestamp(&hclock);
    last_check_changes_time = hourly_clock_get_timestamp(&hclock);
}

void app_loop(app_handle *handle)
{
    hourly_clock_update(&hclock);

    calc_bat_in_if_changed();

    if (hourly_clock_check_elapsed(&hclock, last_sensor_read_time, SENSOR_CHECK_EVERY_SEC))
    {
        read_sensor();
        last_sensor_read_time = hourly_clock_get_timestamp(&hclock);
    }

    if (hourly_clock_check_elapsed(&hclock, last_battery_read_time, BATTERY_CHECK_EVERY_SEC))
    {
        request_battery_read();
        last_battery_read_time = hourly_clock_get_timestamp(&hclock);
    }

    if (hourly_clock_check_elapsed(&hclock, last_check_changes_time, DISPLAY_CHECK_CHANGES_EVERY_SEC))
    {
        anything_changed = check_if_anything_changed();
        last_check_changes_time = hourly_clock_get_timestamp(&hclock);
    }

    display_data local = {
        .temperature = temperature,
        .humidity = humidity,
        .pressure = pressure,
        .bat_in = bat_in,
    };

    display_loop(&handle->display, &local, NULL, anything_changed);

    if (anything_changed)
    {
        anything_changed = false;
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