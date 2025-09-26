#pragma once

#include <stdint.h>

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
    } station_data;

#ifdef __cplusplus
}
#endif
