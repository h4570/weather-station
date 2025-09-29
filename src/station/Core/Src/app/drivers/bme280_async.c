#include "app/drivers/bme280_async.h"
#include "stm32g4xx_hal.h"
#include <string.h>

/* ---------------- Reuse danych kalibracyjnych z bmpxx80.c ----------------
   W bmpxx80.c zmienne nie są static, więc mają zewn. łączenie.
   Deklarujemy je tutaj jako extern, tylko gdy BME280 jest aktywny.
*/
#ifdef BME280
extern uint16_t t1, p1;
extern int16_t t2, t3, p2, p3, p4, p5, p6, p7, p8, p9;
extern uint8_t h1, h3;
extern int8_t h6;
extern int16_t h2, h4, h5;
extern int32_t t_fine; // będzie nadpisany przez nasze liczenie T
#endif

/* ----------------- Prywatne: kompensacja z datasheeta ------------------ */
static float compensate_temperature_bme280(int32_t adc_T)
{
    // Z datasheetu – dokładnie jak w bmpxx80.c
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)t1 << 1))) * ((int32_t)t2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)t1)) *
                      ((adc_T >> 4) - ((int32_t)t1))) >>
                     12) *
                    ((int32_t)t3)) >>
                   14;
    t_fine = var1 + var2;
    float T = (t_fine * 5 + 128) >> 8;
    return T / 100.0f;
}

static int32_t compensate_pressure_bme280(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)p6;
    var2 = var2 + ((var1 * (int64_t)p5) << 17);
    var2 = var2 + (((int64_t)p4) << 35);
    var1 = ((var1 * var1 * (int64_t)p3) >> 8) +
           ((var1 * (int64_t)p2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)p1) >> 33;
    if (var1 == 0)
        return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)p9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)p8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)p7) << 4);
    return (int32_t)p / 256;
}

static float compensate_humidity_bme280(int32_t adc_H)
{
    if (adc_H == 0x8000)
        return -99.0f; // humidity disabled
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)h4) << 20) -
                    (((int32_t)h5) * v_x1_u32r)) +
                   ((int32_t)16384)) >>
                  15) *
                 (((((((v_x1_u32r * ((int32_t)h6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)h3)) >> 11) + ((int32_t)32768))) >>
                     10) +
                    ((int32_t)2097152)) *
                       ((int32_t)h2) +
                   8192) >>
                  14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                               ((int32_t)h1)) >>
                              4));
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    float h = (v_x1_u32r >> 12);
    return h / 1024.0f;
}

/* ----------------------- Callbacks SPI bus managera --------------------- */
static void _bme280_async_on_done(spi_bus_manager *mgr, void *user)
{
    (void)mgr;
    bme280_async *dev = (bme280_async *)user;
    // rx9[0] = echo adresu; właściwe dane od rx9[1]
    const uint8_t *d = &dev->rx9[1];

    // Pressure 20-bit: MSB(0) LSB(1) XLSB(2) >> 4
    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | ((int32_t)(d[2] >> 4));
    // Temperature 20-bit: MSB(3) LSB(4) XLSB(5) >> 4
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | ((int32_t)(d[5] >> 4));
    // Humidity 16-bit: MSB(6) LSB(7)
    int32_t adc_H = ((int32_t)d[6] << 8) | (int32_t)d[7];

    // Kompensacja (reużywa globali z bmpxx80.c)
    float T = compensate_temperature_bme280(adc_T);
    int32_t P = compensate_pressure_bme280(adc_P);
    float H = compensate_humidity_bme280(adc_H);

    dev->last.temperature = T;
    dev->last.pressure = P;
    dev->last.humidity = H;
    dev->last.stamp_ms = HAL_GetTick();
    dev->last.valid = (T != -99.0f && H != -99.0f);

    dev->busy = false;
    dev->error = false;

    if (dev->on_done)
        dev->on_done(dev->user, &dev->last);
}

static void _bme280_async_on_error(spi_bus_manager *mgr, void *user)
{
    (void)mgr;
    bme280_async *dev = (bme280_async *)user;
    dev->busy = false;
    dev->error = true;
}

/* --------------------------------- API ---------------------------------- */
void bme280_async_init(bme280_async *dev,
                       spi_bus_manager *mgr,
                       spi_bus_gpio cs,
                       uint32_t cr1, uint32_t cr2)
{
    memset(dev, 0, sizeof(*dev));
    dev->mgr = mgr;
    dev->cs = cs;
    dev->cr1 = cr1;
    dev->cr2 = cr2;
    dev->tx9[0] = BME280_PRESSUREDATA | 0x80; // 0xF7 | READ (bit7)
    for (int i = 1; i < 9; i++)
        dev->tx9[i] = 0x00; // dummy clocks
}

bool bme280_async_is_busy(const bme280_async *dev) { return dev->busy; }
bool bme280_async_has_data(const bme280_async *dev) { return dev->last.valid; }
bme280_measurement bme280_async_get_last(const bme280_async *dev) { return dev->last; }

bool bme280_async_trigger_read(bme280_async *dev)
{
    if (dev->busy)
        return true; // już w toku; nie duplikuj
    dev->busy = true;
    dev->error = false;

    spi_bus_transaction t = {
        .kind = SPI_BUS_ITEM_TX,
        .cs = dev->cs,
        .dc = (spi_bus_gpio){.port = NULL, .pin = 0, .active_low = true},
        .dc_mode = SPI_BUS_DC_UNUSED,
        .cr1 = dev->cr1,
        .cr2 = dev->cr2,
        .tx = dev->tx9,
        .rx = dev->rx9,
        .len = 9, // 1 bajt adresu + 8 danych
        .dir = SPI_BUS_DIR_TXRX,
        .spi_timeout = HAL_MAX_DELAY,
        .wait_ready = NULL,
        .wait_timeout_ms = 0,
        .on_half = NULL,
        .on_done = _bme280_async_on_done,
        .on_error = _bme280_async_on_error,
        .user = dev};

    spi_bus_manager_status st = spi_bus_manager_submit(dev->mgr, &t);
    if (st != SPI_BUS_MANAGER_OK)
    {
        dev->busy = false;
        dev->error = true;
        return false;
    }
    return true;
}
