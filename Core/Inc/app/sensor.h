#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "app/station_data.h"
#include "app/drivers/bme280_async.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Handle structure for the sensor (BME280)
     */
    typedef struct
    {
        spi_bus_manager *spi_mgr;
        bme280_async bme_async;
    } sensor_handle;

    /**
     * @brief Create and initialize a sensor handle
     */
    sensor_handle sensor_create(spi_bus_manager *spi_mgr);

    /**
     * @brief Initialize the sensor (BME280)
     */
    void sensor_init(sensor_handle *handle);

    /**
     * @brief Trigger a non-blocking read of the sensor. Call sensor_try_get() later to check if data is ready.
     *
     * @return true if the read was successfully triggered, false if the sensor is busy
     */
    bool sensor_kick(sensor_handle *handle);

    /**
     * @brief Try to get the latest sensor readings if available.
     *
     * @param out Pointer to station_data structure to fill with the latest readings
     * @return true if new data was available and copied to out, false otherwise
     */
    bool sensor_try_get(sensor_handle *handle, station_data *out);

#ifdef __cplusplus
}
#endif
