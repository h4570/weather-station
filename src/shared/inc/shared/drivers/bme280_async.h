#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "app/shared_glue/bme280_async_glue.h"
#include "shared/drivers/spi_bus_manager.h"
#include "shared/drivers/bmpxx80.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        float temperature; // °C
        int32_t pressure;  // Pa
        float humidity;    // %RH
        uint32_t stamp_ms; // znacznik czasu gdy wyniki zostały zaktualizowane
        bool valid;        // wyniki są policzone z poprawnych surowych danych
    } bme280_measurement;

    typedef struct
    {
        // spi-bus-manager
        spi_bus_manager *mgr;
        // linie urządzenia
        spi_bus_gpio cs;
        // (opcjonalnie) DC nieużywane — zostawiamy puste
        // snapshot CR1/CR2 dla BME280 (prescaler/CPOL/CPHA/DS=8)
        uint32_t cr1;
        uint32_t cr2;

        // bufory jednorazowego „burst read”
        // SPI: najpierw 1 bajt adresu z bit7=1 (read), potem 8 bajtów danych
        uint8_t tx9[9];
        uint8_t rx9[9];

        // stan
        volatile bool busy;
        volatile bool error;

        // ostatni pomiar
        bme280_measurement last;

        // (opcjonalnie) user payload/callback po zakończeniu
        void (*on_done)(void *user, const bme280_measurement *m);
        void *user;
    } bme280_async;

    /**
     * @brief Inicjalizacja warstwy async. Zakładamy, że BME280 został już
     *        poprawnie zainicjalizowany przez istniejące BME280_Init() i SetConfig()
     *        w trybie NORMALMODE (ciągłe próbkowanie).
     */
    void bme280_async_init(bme280_async *dev,
                           spi_bus_manager *mgr,
                           spi_bus_gpio cs,
                           uint32_t cr1, uint32_t cr2);

    /**
     * @brief Czy urządzenie jest zajęte (DMA w toku lub oczekiwanie w kolejce).
     */
    bool bme280_async_is_busy(const bme280_async *dev);

    /**
     * @brief Zleć pojedynczy, nieblokujący odczyt burst (P/T/H).
     *        Jeśli kolejka pełna -> zwraca false, nic nie zlecono.
     */
    bool bme280_async_trigger_read(bme280_async *dev);

    /**
     * @brief Czy są świeże wyniki.
     */
    bool bme280_async_has_data(const bme280_async *dev);

    /**
     * @brief Pobierz ostatni pomiar (kopię).
     */
    bme280_measurement bme280_async_get_last(const bme280_async *dev);

    /**
     * @brief Ustal callback po zakończeniu pojedynczego odczytu (opcjonalne).
     */
    static inline void bme280_async_set_done_cb(bme280_async *dev,
                                                void (*cb)(void *, const bme280_measurement *),
                                                void *user)
    {
        dev->on_done = cb;
        dev->user = user;
    }

#ifdef __cplusplus
}
#endif
