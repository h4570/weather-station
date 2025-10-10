#include "app/app.h"
#include "stdlib.h"

void app_init(app_handle *handle, void (*system_clock_config_func)(void))
{
    HAL_Delay(50); // Wait for power to stabilize

    handle->system_clock_config_func = system_clock_config_func;
    handle->battery = battery_create(&hadc);
    handle->hclock = hourly_clock_create(&hrtc);
    handle->sensor = sensor_create(&htim2, &hi2c1);
    handle->radio = radio_create(RAD_CS_GPIO_Port, RAD_CS_Pin, RAD_DI0_GPIO_Port, RAD_DI0_Pin, &hspi1, &handle->hclock);

    radio_init(&handle->radio);
    sensor_init(&handle->sensor);

    battery_request_read(&handle->battery);

    handle->last_battery_read_time = hourly_clock_get_timestamp(&handle->hclock);
    handle->last_radio_send_time = hourly_clock_get_timestamp(&handle->hclock);
}

#ifdef DEBUG
#define INTERVAL_SEC 5
#else
#define INTERVAL_SEC 5
// #define INTERVAL_SEC 54
#endif

static void app_enter_low_power_mode(app_handle *handle, uint32_t seconds)
{
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, seconds, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
    HAL_SuspendTick();

    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);

    HAL_ResumeTick();
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    handle->system_clock_config_func();
}

void app_loop(app_handle *handle)
{
    hourly_clock_update(&handle->hclock);
    radio_loop(&handle->radio);

    {
        battery_refresh(&handle->battery);

        if (battery_check_if_level_changed(&handle->battery))
        {
            handle->local.bat_in = battery_get_level(&handle->battery);
        }

        battery_request_read(&handle->battery);
    }

    sensor_read(&handle->sensor, &handle->local);
    battery_update_temperature(&handle->battery, handle->local.temperature);

    radio_send(&handle->radio, &handle->local);

#if DEBUG
    HAL_Delay(INTERVAL_SEC * 1000);
#else
    app_enter_low_power_mode(handle, INTERVAL_SEC);
#endif

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