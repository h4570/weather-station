#include "bat_nimh_calc.h"

int bat_nimh_calc_execute(float bat_vcc, float temp_c)
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

float bat_nimh_calc_get_voltage_for_soc(uint32_t bat_vcc_adc_value, float vref, float multiplier)
{
    // 12-bit ADC, Vref example =  3.3V
    float voltage = (bat_vcc_adc_value / 4095.0f) * vref;

    // Assuming a example voltage divider with R1 = 100k and R2 = 100k
    float battery_voltage = voltage * multiplier;

    return battery_voltage;
}