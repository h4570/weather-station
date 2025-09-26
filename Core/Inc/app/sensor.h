#pragma once

#include <stdint.h>
#include "app/station_data.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Initialize the sensor (BME280)
     */
    void sensor_init();

    /**
     * @brief Read the sensor values into the provided station_data structure
     *
     * @param station_data Pointer to the station_data structure to fill with sensor readings
     */
    void sensor_read(station_data *station_data);

#ifdef __cplusplus
}
#endif
