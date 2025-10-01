#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "shared/app_device_data.h"
#include "shared/drivers/bme280_async.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Handle structure for the sensor (BME280)
     */
    typedef struct
    {
        spi_bus_manager *spi_mgr;      /**< SPI bus manager for communication */
        bme280_async bme_async;        /**< Asynchronous BME280 handler */
        TIM_HandleTypeDef *sensor_tim; /**< Timer handle for microsecond delays */
        SPI_HandleTypeDef *sensor_spi; /**< SPI handle for BME280 communication */
        GPIO_TypeDef *cs_port;         /**< GPIO port for chip select */
        uint16_t cs_pin;               /**< GPIO pin for chip select */
    } sensor_handle;

    /**
     * @brief Create and initialize a sensor handle
     */
    sensor_handle sensor_create(spi_bus_manager *spi_mgr, TIM_HandleTypeDef *sensor_tim, SPI_HandleTypeDef *sensor_spi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

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
     * @param out Pointer to app_device_data structure to fill with the latest readings
     * @return true if new data was available and copied to out, false otherwise
     */
    bool sensor_try_get(sensor_handle *handle, app_device_data *out);

#ifdef __cplusplus
}
#endif
