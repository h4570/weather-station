#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Render the weather station display with given parameters
     * @param t_in Indoor temperature in °C
     * @param h_in Indoor humidity in %
     * @param p_in Indoor pressure in Pa
     * @param batt_in Indoor battery level in % (0-100)
     * @param t_out Outdoor temperature in °C
     * @param h_out Outdoor humidity in %
     * @param p_out Outdoor pressure in Pa
     * @param batt_out Outdoor battery level in % (0-100)
     */
    void renderer_execute(
        float t_in, float h_in, int32_t p_in, int batt_in,
        float t_out, float h_out, int32_t p_out, int batt_out);

#ifdef __cplusplus
}
#endif
