#include "shared/battery.h"

volatile uint16_t battery_vcc_adc_value;

/**
 * @brief Calculate the State of Charge (SoC) percentage for a NiMH battery pack based on voltage and temperature.
 *
 * This function uses a look-up table (LUT) to estimate the SoC based on the battery voltage for a 2-cell NiMH pack
 * at approximately 20°C under moderate load. It also applies a simple temperature correction to account for
 * variations in battery performance at different temperatures.
 *
 * @param bat_vcc The measured battery voltage in volts (V) for a 2-cell NiMH pack.
 * @param temp_c The ambient temperature in degrees Celsius (°C).
 * @return int The estimated State of Charge (SoC) as a percentage (0-100%).
 */
static int battery_nimh_calc_execute(float bat_vcc, float temp_c)
{
    // LUT napięcie [V dla 2 ogniw] -> %SOC (dla ~20°C, średnie obciążenie)
    static const float LUT_V[] = {2.64f, 2.56f, 2.50f, 2.46f, 2.42f, 2.38f, 2.34f, 2.30f, 2.24f, 2.16f, 2.00f};
    static const int LUT_PC[] = {100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0};
    const int LUT_N = sizeof(LUT_V) / sizeof(LUT_V[0]);

    // --- interpolacja liniowa ---
    float soc;
    if (bat_vcc >= LUT_V[0])
        soc = 100.0f;
    else if (bat_vcc <= LUT_V[LUT_N - 1])
        soc = 0.0f;
    else
    {
        for (int i = 0; i < LUT_N - 1; i++)
        {
            if (bat_vcc <= LUT_V[i] && bat_vcc >= LUT_V[i + 1])
            {
                float t = (bat_vcc - LUT_V[i + 1]) / (LUT_V[i] - LUT_V[i + 1]); // 0..1
                soc = LUT_PC[i + 1] + t * (LUT_PC[i] - LUT_PC[i + 1]);
                break;
            }
        }
    }

    // --- prosta korekta temperatury ---
    if (temp_c < 10.0f)
    {
        // im zimniej tym gorzej – przesuwamy w dół
        soc -= 5.0f + (10.0f - temp_c) * 0.5f; // ok. -5..-15 p.p.
    }
    else if (temp_c > 35.0f)
    {
        // na ciepło lekki bonus
        soc += 3.0f + (temp_c - 35.0f) * 0.2f; // ok. +3..+7 p.p.
    }

    // clamp 0–100
    if (soc < 0.0f)
        soc = 0.0f;
    if (soc > 100.0f)
        soc = 100.0f;

    return (int)(soc + 0.5f);
}

/**
 * @brief Calculate the battery voltage from ADC reading.
 *
 * This function converts a raw ADC value to the corresponding battery voltage,
 * taking into account the reference voltage and the voltage divider ratio.
 *
 * @param bat_vcc_adc_value The raw ADC value representing the battery voltage.
 * @param vref The reference voltage used by the ADC (e.g., 3.3V).
 * @param multiplier The multiplier to account for the voltage divider ratio (e.g., 2 for a 100k/100k divider).
 * @return float The calculated battery voltage in volts (V).
 */
static float battery_nimh_calc_get_voltage_for_soc(uint32_t bat_vcc_adc_value, float vref, float multiplier)
{
    // 12-bit ADC, Vref example =  3.3V
    float voltage = (bat_vcc_adc_value / 4095.0f) * vref;

    // Assuming a example voltage divider with R1 = 100k and R2 = 100k
    float battery_voltage = voltage * multiplier;

    return battery_voltage;
}

battery_handle battery_create(ADC_HandleTypeDef *battery_adc)
{
    battery_handle handle;

    handle.current_adc_value = 0;
    handle.previous_adc_value = 0;
    handle.last_returned_bat_level = 0;
    handle.current_bat_level = 0;
    handle.temperature = 20.0f; // default
    handle.battery_adc = battery_adc;

    return handle;
}

void battery_request_read(battery_handle *handle)
{
    HAL_ADC_Start_IT(handle->battery_adc);
}

void battery_refresh(battery_handle *handle)
{
    handle->current_adc_value = battery_vcc_adc_value;

    if (handle->current_adc_value != handle->previous_adc_value)
    {
        // 2x AA battery, voltage divider 100k/100k, ADC 12-bit, Vref = 3.3V
        float battery_voltage = battery_nimh_calc_get_voltage_for_soc(handle->current_adc_value, 3.3F, 2.0F);
        handle->current_bat_level = battery_nimh_calc_execute(battery_voltage, handle->temperature);

        handle->previous_adc_value = handle->current_adc_value;
    }
}

bool battery_check_if_level_changed(battery_handle *handle)
{
    return handle->current_bat_level != handle->last_returned_bat_level;
}

uint16_t battery_get_level(battery_handle *handle)
{
    handle->last_returned_bat_level = handle->current_bat_level;
    return handle->current_bat_level;
}

void battery_update_temperature(battery_handle *handle, float temperature_c)
{
    handle->temperature = temperature_c;
}

void battery_adc_interrupt_handler(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        battery_vcc_adc_value = HAL_ADC_GetValue(hadc);
    }
}