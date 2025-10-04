#include "app/sensor.h"
#include "gpio.h"
#include "tim.h"

sensor_handle sensor_create(TIM_HandleTypeDef *sensor_tim, I2C_HandleTypeDef *sensor_i2c)
{
    sensor_handle handle;
    handle.sensor_tim = sensor_tim;
    handle.sensor_i2c = sensor_i2c;
    return handle;
}

void sensor_init(sensor_handle *handle)
{
    HAL_TIM_Base_Start(handle->sensor_tim);
    BME280_Init(handle->sensor_i2c,
                BME280_TEMPERATURE_16BIT,
                BME280_PRESSURE_ULTRALOWPOWER,
                BME280_HUMIDITY_STANDARD,
                BME280_FORCEDMODE);
    BME280_SetConfig(BME280_STANDBY_MS_1000, BME280_FILTER_OFF);
}

void sensor_read(sensor_handle *handle, app_device_data *out)
{
    BME280_ReadTemperatureAndPressureAndHumidity(&out->temperature, &out->pressure, &out->humidity);
}