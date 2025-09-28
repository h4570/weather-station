#pragma once

#include <stdint.h>
#include <stdbool.h>
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
     * @brief Trigger a non-blocking read of the sensor. Call sensor_try_get() later to check if data is ready.
     *
     * @return true if the read was successfully triggered, false if the sensor is busy
     */
    bool sensor_kick();

    /**
     * @brief Try to get the latest sensor readings if available.
     *
     * @param out Pointer to station_data structure to fill with the latest readings
     * @return true if new data was available and copied to out, false otherwise
     */
    bool sensor_try_get(station_data *out);

#ifdef __cplusplus
}
#endif
