#include "app/sensor.h"
#include "app/drivers/bmpxx80.h"

#include "gpio.h"
#include "tim.h"

void sensor_init()
{
    HAL_TIM_Base_Start(&htim1);
    BMPxx_init(BME280_CS_GPIO_Port, BME280_CS_Pin);
    BME280_Init(&hspi2, BME280_TEMPERATURE_16BIT, BME280_PRESSURE_ULTRALOWPOWER, BME280_HUMIDITY_STANDARD, BME280_FORCEDMODE);
    BME280_SetConfig(BME280_STANDBY_MS_10, BME280_FILTER_OFF);
}

void sensor_read(station_data *station_data)
{
    BME280_ReadTemperatureAndPressureAndHumidity(&station_data->temperature, &station_data->pressure, &station_data->humidity);
}