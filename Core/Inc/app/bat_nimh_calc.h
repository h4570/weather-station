
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

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
    int bat_nimh_calc_execute(float bat_vcc, float temp_c);

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
    float bat_nimh_calc_get_voltage_for_soc(uint32_t bat_vcc_adc_value, float vref, float multiplier);

#ifdef __cplusplus
}
#endif
