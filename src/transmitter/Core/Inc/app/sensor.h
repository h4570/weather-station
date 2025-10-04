#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "shared/app_device_data.h"
#include "shared/drivers/bmpxx80.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Handle structure for the sensor (BME280)
     */
    typedef struct
    {
        TIM_HandleTypeDef *sensor_tim; /**< Timer handle for microsecond delays */
        I2C_HandleTypeDef *sensor_i2c; /**< I2C handle for BME280 communication */
    } sensor_handle;

    /**
     * @brief Create and initialize a sensor handle
     */
    sensor_handle sensor_create(TIM_HandleTypeDef *sensor_tim, I2C_HandleTypeDef *sensor_i2c);

    /**
     * @brief Initialize the sensor (BME280)
     */
    void sensor_init(sensor_handle *handle);

    /**
     * @brief Read sensor data into the provided output structure
     * @param handle Pointer to the sensor handle
     */
    void sensor_read(sensor_handle *handle, app_device_data *out);

#ifdef __cplusplus
}
#endif
