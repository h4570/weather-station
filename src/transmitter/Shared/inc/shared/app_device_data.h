#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Structure to hold station data including temperature, humidity, pressure, and battery input
     */
    typedef struct
    {
        float temperature; /**< Temperature in degrees Celsius */
        float humidity;    /**< Humidity in percentage */
        int32_t pressure;  /**< Pressure in Pascals */
        int32_t bat_in;    /**< Battery input in millivolts */
    } app_device_data;

    /**
     * @brief Check if the device data has changed significantly compared to the last recorded data
     * @param current Pointer to the current device data
     * @param last Pointer to the last recorded device data
     * @return true if significant changes are detected, false otherwise
     */
    bool app_device_data_check_if_changed(const app_device_data *current, const app_device_data *last);

#ifdef __cplusplus
}
#endif
