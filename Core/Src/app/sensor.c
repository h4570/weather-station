#include "app/sensor.h"
#include "app/drivers/bme280_async.h"

#include "gpio.h"
#include "tim.h"

// --- SPI bus manager: storage + handle ---
static spi_bus_transaction spiq_storage[8]; // kolejka na 8 transakcji (zmień, jeśli potrzebujesz)
static spi_bus_manager spi_mgr;             // nasz manager dla hspi2

// -------------------- HAL -> bus-manager glue (IRQ) ---------------------
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_tx_cplt(&spi_mgr, hspi);
}
void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_tx_cplt(&spi_mgr, hspi);
}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_txrx_cplt(&spi_mgr, hspi);
}
void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_txrx_cplt(&spi_mgr, hspi);
}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    spi_bus_manager_on_error(&spi_mgr, hspi);
}

// --- Async device handle ---
static bme280_async bme_async;

void sensor_init()
{
    // Timer do delay_us w legacy kodzie
    HAL_TIM_Base_Start(&htim1);

    // Inicjalizacja SPI bus managera
    spi_mgr = spi_bus_manager_create(&hspi2, spiq_storage, (uint16_t)(sizeof(spiq_storage) / sizeof(spiq_storage[0])));
    // (opcjonalnie) spi_bus_manager_set_clean_dcache(&spi_mgr, true);

    // Legacy init — ustawia kalibrację i NORMALMODE (ciągła konwersja)
    BMPxx_init(BME280_CS_GPIO_Port, BME280_CS_Pin);
    BME280_Init(&hspi2,
                BME280_TEMPERATURE_16BIT,
                BME280_PRESSURE_ULTRALOWPOWER,
                BME280_HUMIDITY_STANDARD,
                BME280_NORMALMODE);
    BME280_SetConfig(BME280_STANDBY_MS_1000, BME280_FILTER_OFF);

    // Przygotuj CR1/CR2 snapshot dla BME280 (8-bit data, CPOL/CPHA wg Twojej konfiguracji hspi2)
    // Najprościej: skopiuj aktualne rejestry z hspi2 po jego init.
    uint32_t cr1 = hspi2.Instance->CR1;
    uint32_t cr2 = hspi2.Instance->CR2;

    // Init async warstwy
    bme280_async_init(&bme_async, &spi_mgr,
                      (spi_bus_gpio){.port = BME280_CS_GPIO_Port, .pin = BME280_CS_Pin, .active_low = true},
                      cr1, cr2);
}

bool sensor_kick(void)
{
    // Zleca jednorazowy burst-read (9 bajtów) P/T/H przez DMA. Nieblokujące.
    return bme280_async_trigger_read(&bme_async);
}

bool sensor_try_get(station_data *out)
{
    if (!bme280_async_has_data(&bme_async))
        return false;

    bme280_measurement m = bme280_async_get_last(&bme_async);

    if (!m.valid)
        return false;

    out->temperature = m.temperature;
    out->pressure = m.pressure;
    out->humidity = m.humidity;

    return true;
}