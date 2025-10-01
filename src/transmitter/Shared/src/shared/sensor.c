#include "shared/sensor.h"
#include "gpio.h"
#include "tim.h"

sensor_handle sensor_create(spi_bus_manager *spi_mgr, TIM_HandleTypeDef *sensor_tim, SPI_HandleTypeDef *sensor_spi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    sensor_handle handle;
    handle.spi_mgr = spi_mgr;
    handle.sensor_tim = sensor_tim;
    handle.sensor_spi = sensor_spi;
    handle.cs_port = cs_port;
    handle.cs_pin = cs_pin;
    return handle;
}

void sensor_init(sensor_handle *handle)
{
    // Timer do delay_us w legacy kodzie
    HAL_TIM_Base_Start(handle->sensor_tim);

    // Legacy init — ustawia kalibrację i NORMALMODE (ciągła konwersja)
    BMPxx_init(handle->cs_port, handle->cs_pin);
    BME280_Init(handle->sensor_spi,
                BME280_TEMPERATURE_16BIT,
                BME280_PRESSURE_ULTRALOWPOWER,
                BME280_HUMIDITY_STANDARD,
                BME280_NORMALMODE);
    BME280_SetConfig(BME280_STANDBY_MS_1000, BME280_FILTER_OFF);

    // Przygotuj CR1/CR2 snapshot dla BME280 (8-bit data, CPOL/CPHA wg Twojej konfiguracji sensor_spi)
    // Najprościej: skopiuj aktualne rejestry z sensor_spi po jego init.
    uint32_t cr1 = handle->sensor_spi->Instance->CR1;
    uint32_t cr2 = handle->sensor_spi->Instance->CR2;

    // Init async warstwy
    bme280_async_init(&handle->bme_async, handle->spi_mgr,
                      (spi_bus_gpio){.port = handle->cs_port, .pin = handle->cs_pin, .active_low = true},
                      cr1, cr2);
}

bool sensor_kick(sensor_handle *handle)
{
    // Zleca jednorazowy burst-read (9 bajtów) P/T/H przez DMA. Nieblokujące.
    return bme280_async_trigger_read(&handle->bme_async);
}

bool sensor_try_get(sensor_handle *handle, app_device_data *out)
{
    if (!bme280_async_has_data(&handle->bme_async))
        return false;

    bme280_measurement m = bme280_async_get_last(&handle->bme_async);

    if (!m.valid)
        return false;

    out->temperature = m.temperature;
    out->pressure = m.pressure;
    out->humidity = m.humidity;

    return true;
}