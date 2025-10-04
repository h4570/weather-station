#include "shared/app_device_data.h"
#include <math.h>
#include <stdlib.h>

bool app_device_data_check_if_changed(const app_device_data *current, const app_device_data *last)
{
    const float temp_threshold = 0.5F; // degrees Celsius
    const float hum_threshold = 1.0F;  // percentage
    const int32_t pres_threshold = 10; // Pascals
    const int32_t bat_threshold = 100; // millivolts

    if (fabsf(current->temperature - last->temperature) >= temp_threshold)
    {
        return true;
    }

    if (fabsf(current->humidity - last->humidity) >= hum_threshold)
    {
        return true;
    }

    if (abs(current->pressure - last->pressure) >= pres_threshold)
    {
        return true;
    }

    if (abs(current->bat_in - last->bat_in) >= bat_threshold)
    {
        return true;
    }

    return false;
}
