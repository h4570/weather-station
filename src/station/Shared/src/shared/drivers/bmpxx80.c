/*
 * BMXX80.c  (reworked)
 *
 *  The MIT License.
 *  Based on Adafruit libraries.
 *  Created on: 10.08.2018
 *  Modified on: 04.10.2025
 *  Version: 2.0
 *  Original author: Mateusz Salamon (www.msalamon.pl)
 *  Modification author: GPT 5
 *
 *  === What was fixed / improved ===
 *  - [CRITICAL][BME280] Fixed wrong I2C device address usage in BME280_Read16/Read24
 *    (they incorrectly used BMP280_I2CADDR). Now consistently use BME280_I2CADDR.
 *  - [SPI] Fixed buffer length in *_Read24 (SPI): was sending 3 bytes while reading 4.
 *  - [Init API] Init functions now return `bool` (true on success, false on failure)
 *    instead of `void`. They validate CHIPID, apply soft reset (when applicable),
 *    wait with timeouts instead of infinite loops, and fail fast with clear errors.
 *  - [Timeouts] Replaced endless busy-waits with bounded waits using HAL_GetTick()
 *    (e.g., conversion complete, calibration reading).
 *  - [Forced mode reads] Switched to polling the STATUS.MEASURING bit with timeout
 *    instead of relying on CONTROL mode bits only.
 *  - [Null guards] Basic NULL-pointer validation for HAL handles & chip-select pins.
 *  - [Consistency] Unified SPI transfers: address | 0x80 for reads, | 0x00 for writes,
 *    proper CS handling, dummy clocking bytes, and consistent return paths.
 *  - [Safety] Defensive checks against divide-by-zero and invalid raw values remain.
 *
 *  NOTE: Header prototypes must be updated to `bool ...Init(...)`.
 *        If you need temporary backward compatibility, add thin wrappers in the header.
 */

#include "shared/drivers/bmpxx80.h"
#include <string.h>
#include "math.h"

/* ===== CS lines context (set once by BMPxx_init) ===== */
GPIO_TypeDef *BMPxx_cs_port = NULL;
uint16_t BMPxx_cs_pin = 0;

/* ===== External 1us timer delay (provided elsewhere) ===== */
static void BMPxx_delay_us(uint16_t us)
{
    bmpxx80_1_us_timer->Instance->CNT = 0;
    while (bmpxx80_1_us_timer->Instance->CNT <= us)
        ;
}

/* ===== Small helpers with timeouts (C only, no lambdas) ===== */
static bool wait_reg_equals(uint8_t (*read8)(uint8_t), uint8_t reg, uint8_t expected, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while (read8(reg) != expected)
    {
        if ((HAL_GetTick() - start) >= timeout_ms)
            return false;
    }
    return true;
}

static bool wait_reg_bit_clear(uint8_t (*read8)(uint8_t), uint8_t reg, uint8_t mask, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while ((read8(reg) & mask) != 0U)
    {
        if ((HAL_GetTick() - start) >= timeout_ms)
            return false;
    }
    return true;
}

/* ===== CS helpers (SPI) ===== */
static inline void cs_low(void)
{
#if (BMP_SPI == 1)
    if (BMPxx_cs_port)
        HAL_GPIO_WritePin(BMPxx_cs_port, BMPxx_cs_pin, GPIO_PIN_RESET);
#endif
}
static inline void cs_high(void)
{
#if (BMP_SPI == 1)
    if (BMPxx_cs_port)
        HAL_GPIO_WritePin(BMPxx_cs_port, BMPxx_cs_pin, GPIO_PIN_SET);
#endif
}

/* ===== Public CS init ===== */
void BMPxx_Spi_CS_Init(GPIO_TypeDef *port, uint16_t pin)
{
    BMPxx_cs_port = port;
    BMPxx_cs_pin = pin;
}

/* ===== Private HAL handles ===== */
#if (BMP_I2C == 1)
static I2C_HandleTypeDef *i2c_h = NULL;
#endif
#if (BMP_SPI == 1)
static SPI_HandleTypeDef *spi_h = NULL;
#endif

/* ===== Calibration / state ===== */
#ifdef BMP180
static uint8_t oversampling;
static int16_t ac1, ac2, ac3, b1, b2, mb, mc, md;
static uint16_t ac4, ac5, ac6;
#endif

#ifdef BMP280
static uint8_t _temperature_res, _pressure_oversampling, _mode;
static int16_t t2, t3, p2, p3, p4, p5, p6, p7, p8, p9;
static uint16_t t1, p1;
static int32_t t_fine;
#endif

#ifdef BME280
/* NOTE: made non-static so bme280_async.c can extern them.            */
/* Be sure ONLY BME280 is enabled (not BMP280) to avoid name clashes.  */
uint8_t _temperature_res, _pressure_oversampling, _humidity_oversampling, _mode, h1, h3;
int8_t h6;
int16_t t2, t3, p2, p3, p4, p5, p6, p7, p8, p9, h2, h4, h5;
uint16_t t1, p1;
int32_t t_fine;
#endif

/* =========================
 * ===== BMP180 block ======
 * ========================= */
#ifdef BMP180
static uint8_t BMP180_Read8(uint8_t addr)
{
    uint8_t tmp = 0;
    HAL_I2C_Mem_Read(i2c_h, BMP180_I2CADDR, addr, I2C_MEMADD_SIZE_8BIT, &tmp, 1, 10);
    return tmp;
}
static uint16_t BMP180_Read16(uint8_t addr)
{
    uint8_t tmp[2] = {0};
    HAL_I2C_Mem_Read(i2c_h, BMP180_I2CADDR, addr, I2C_MEMADD_SIZE_8BIT, tmp, 2, 10);
    return ((uint16_t)tmp[0] << 8) | tmp[1];
}
static void BMP180_Write8(uint8_t address, uint8_t data)
{
    HAL_I2C_Mem_Write(i2c_h, BMP180_I2CADDR, address, I2C_MEMADD_SIZE_8BIT, &data, 1, 10);
}
static uint16_t BMP180_readRawTemperature(void)
{
    BMP180_Write8(BMP180_CONTROL, BMP180_READTEMPCMD);
    /* Wait up to ~10ms (typ 4.5ms) for SCO clear */
    uint32_t start = HAL_GetTick();
    while ((BMP180_Read8(BMP180_CONTROL) & BMP180_SCO) != 0U)
    {
        if ((HAL_GetTick() - start) >= 10U)
            break;
    }
    return BMP180_Read16(BMP180_TEMPDATA);
}
static int32_t BMP180_computeB5(int32_t UT)
{
    int32_t X1 = (UT - (int32_t)ac6) * (int32_t)ac5 >> 15;
    int32_t X2 = ((int32_t)mc << 11) / (X1 + (int32_t)md);
    return X1 + X2;
}
static int32_t BMP180_readRawPressure(void)
{
    uint32_t raw;
    BMP180_Write8(BMP180_CONTROL, (uint8_t)(BMP180_READPRESSURECMD + (oversampling << 6)));

    /* Max ~26ms for highest oversampling */
    uint32_t start = HAL_GetTick();
    while ((BMP180_Read8(BMP180_CONTROL) & BMP180_SCO) != 0U)
    {
        if ((HAL_GetTick() - start) >= 40U)
            return 0;
    }

    raw = BMP180_Read16(BMP180_PRESSUREDATA);
    raw = (raw << 8) | BMP180_Read8(BMP180_PRESSUREDATA + 2);
    raw >>= (8U - oversampling);
    return (int32_t)raw;
}

bool BMP180_Init(I2C_HandleTypeDef *i2c_handler, uint8_t mode)
{
    if (i2c_handler == NULL)
        return false;
    i2c_h = i2c_handler;

    if (mode > BMP180_ULTRAHIGHRES)
        mode = BMP180_ULTRAHIGHRES;
    oversampling = mode;

    if (!wait_reg_equals(BMP180_Read8, BMP180_CHIPID, 0x55, 50))
        return false;

    ac1 = BMP180_Read16(BMP180_CAL_AC1);
    ac2 = BMP180_Read16(BMP180_CAL_AC2);
    ac3 = BMP180_Read16(BMP180_CAL_AC3);
    ac4 = BMP180_Read16(BMP180_CAL_AC4);
    ac5 = BMP180_Read16(BMP180_CAL_AC5);
    ac6 = BMP180_Read16(BMP180_CAL_AC6);
    b1 = BMP180_Read16(BMP180_CAL_B1);
    b2 = BMP180_Read16(BMP180_CAL_B2);
    mb = BMP180_Read16(BMP180_CAL_MB);
    mc = BMP180_Read16(BMP180_CAL_MC);
    md = BMP180_Read16(BMP180_CAL_MD);

    return true;
}

float BMP180_ReadTemperature(void)
{
    int32_t UT = BMP180_readRawTemperature();
    int32_t B5 = BMP180_computeB5(UT);
    float t01C = (float)((B5 + 8) >> 4); // 0.1C
    return t01C / 10.0f;
}

int32_t BMP180_ReadPressure(void)
{
    int32_t UT = BMP180_readRawTemperature();
    int32_t UP = BMP180_readRawPressure();
    int32_t B5 = BMP180_computeB5(UT);

    int32_t B6 = B5 - 4000;
    int32_t X1 = ((int32_t)b2 * ((B6 * B6) >> 12)) >> 11;
    int32_t X2 = ((int32_t)ac2 * B6) >> 11;
    int32_t X3 = X1 + X2;
    int32_t B3 = ((((int32_t)ac1 * 4 + X3) << oversampling) + 2) / 4;

    X1 = ((int32_t)ac3 * B6) >> 13;
    X2 = ((int32_t)b1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    uint32_t B4 = ((uint32_t)ac4 * (uint32_t)(X3 + 32768)) >> 15;
    uint32_t B7 = ((uint32_t)UP - B3) * (uint32_t)(50000UL >> oversampling);

    int32_t p;
    if (B7 < 0x80000000UL)
        p = (int32_t)((B7 * 2) / B4);
    else
        p = (int32_t)((B7 / B4) * 2);

    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    p = p + ((X1 + X2 + 3791) >> 4);

    return p; // Pa
}

float BMP180_PressureToAltitude(float sea_level, float atmospheric)
{
    return 44330.0f * (1.0f - powf(atmospheric / sea_level, 0.1903f));
}
float BMP180_SeaLevelForAltitude(float altitude, float atmospheric)
{
    return atmospheric / powf(1.0f - (altitude / 44330.0f), 5.255f);
}
#endif /* BMP180 */

/* =========================
 * ===== BMP280 block ======
 * ========================= */
#ifdef BMP280
static uint8_t BMP280_Read8(uint8_t addr)
{
#if (BMP_I2C == 1)
    uint8_t tmp = 0;
    HAL_I2C_Mem_Read(i2c_h, BMP280_I2CADDR, addr, I2C_MEMADD_SIZE_8BIT, &tmp, 1, 10);
    return tmp;
#endif
#if (BMP_SPI == 1)
    uint8_t tx[2] = {(uint8_t)(addr | 0x80U), 0x00};
    uint8_t rx[2] = {0};
    cs_low();
    HAL_SPI_TransmitReceive(spi_h, tx, rx, 2, 10);
    cs_high();
    return rx[1];
#endif
}
static uint16_t BMP280_Read16(uint8_t addr)
{
#if (BMP_I2C == 1)
    uint8_t tmp[2] = {0};
    HAL_I2C_Mem_Read(i2c_h, BMP280_I2CADDR, addr, I2C_MEMADD_SIZE_8BIT, tmp, 2, 10);
    return ((uint16_t)tmp[0] << 8) | tmp[1];
#endif
#if (BMP_SPI == 1)
    uint8_t tx[3] = {(uint8_t)(addr | 0x80U), 0x00, 0x00};
    uint8_t rx[3] = {0};
    cs_low();
    HAL_SPI_TransmitReceive(spi_h, tx, rx, 3, 10);
    cs_high();
    return ((uint16_t)rx[1] << 8) | rx[2];
#endif
}
static uint16_t BMP280_Read16LE(uint8_t addr)
{
    uint16_t v = BMP280_Read16(addr);
    return (uint16_t)((v >> 8) | (v << 8));
}
static void BMP280_Write8(uint8_t address, uint8_t data)
{
#if (BMP_I2C == 1)
    HAL_I2C_Mem_Write(i2c_h, BMP280_I2CADDR, address, I2C_MEMADD_SIZE_8BIT, &data, 1, 10);
#endif
#if (BMP_SPI == 1)
    uint8_t tx[2] = {(uint8_t)(address & 0x7FU), data};
    uint8_t rx[2] = {0};
    cs_low();
    HAL_SPI_TransmitReceive(spi_h, tx, rx, 2, 10);
    cs_high();
#endif
}
static uint32_t BMP280_Read24(uint8_t addr)
{
#if (BMP_I2C == 1)
    uint8_t tmp[3] = {0};
    HAL_I2C_Mem_Read(i2c_h, BMP280_I2CADDR, addr, I2C_MEMADD_SIZE_8BIT, tmp, 3, 10);
    return ((uint32_t)tmp[0] << 16) | ((uint32_t)tmp[1] << 8) | tmp[2];
#endif
#if (BMP_SPI == 1)
    uint8_t tx[4] = {(uint8_t)(addr | 0x80U), 0x00, 0x00, 0x00};
    uint8_t rx[4] = {0};
    cs_low();
    HAL_SPI_TransmitReceive(spi_h, tx, rx, 4, 10);
    cs_high();
    return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
#endif
}
void BMP280_SetConfig(uint8_t standby_time, uint8_t filter)
{
    uint8_t v = (uint8_t)((((standby_time & 0x7U) << 5) | ((filter & 0x7U) << 2)) & 0xFCU);
    BMP280_Write8(BMP280_CONFIG, v);
}

#if (BMP_I2C == 1)
bool BMP280_Init(I2C_HandleTypeDef *i2c_handler, uint8_t temperature_resolution, uint8_t pressure_oversampling, uint8_t mode)
{
    if (i2c_handler == NULL)
        return false;
    i2c_h = i2c_handler;
#endif
#if (BMP_SPI == 1)
    bool BMP280_Init(SPI_HandleTypeDef * spi_handler, uint8_t temperature_resolution, uint8_t pressure_oversampling, uint8_t mode)
    {
        if (spi_handler == NULL || BMPxx_cs_port == NULL)
            return false;
        spi_h = spi_handler;
        cs_low();
        HAL_Delay(5);
        cs_high();
#endif
        if (mode > BMP280_NORMALMODE)
            mode = BMP280_NORMALMODE;
        _mode = mode;
        if (mode == BMP280_FORCEDMODE)
            mode = BMP280_SLEEPMODE;

        if (temperature_resolution > BMP280_TEMPERATURE_20BIT)
            temperature_resolution = BMP280_TEMPERATURE_20BIT;
        _temperature_res = temperature_resolution;

        if (pressure_oversampling > BMP280_ULTRAHIGHRES)
            pressure_oversampling = BMP280_ULTRAHIGHRES;
        _pressure_oversampling = pressure_oversampling;

        if (!wait_reg_equals(BMP280_Read8, BMP280_CHIPID, 0x58, 50))
            return false;

        t1 = BMP280_Read16LE(BMP280_DIG_T1);
        t2 = BMP280_Read16LE(BMP280_DIG_T2);
        t3 = BMP280_Read16LE(BMP280_DIG_T3);

        p1 = BMP280_Read16LE(BMP280_DIG_P1);
        p2 = BMP280_Read16LE(BMP280_DIG_P2);
        p3 = BMP280_Read16LE(BMP280_DIG_P3);
        p4 = BMP280_Read16LE(BMP280_DIG_P4);
        p5 = BMP280_Read16LE(BMP280_DIG_P5);
        p6 = BMP280_Read16LE(BMP280_DIG_P6);
        p7 = BMP280_Read16LE(BMP280_DIG_P7);
        p8 = BMP280_Read16LE(BMP280_DIG_P8);
        p9 = BMP280_Read16LE(BMP280_DIG_P9);

        BMP280_Write8(BMP280_CONTROL, (uint8_t)((temperature_resolution << 5) | (pressure_oversampling << 2) | mode));
        return true;
    }

    static bool bmp280_wait_measuring_clear(uint32_t timeout_ms)
    {
        return wait_reg_bit_clear(BMP280_Read8, BMP280_STATUS, BMP280_MEASURING, timeout_ms);
    }

    float BMP280_ReadTemperature(void)
    {
        if (_mode == BMP280_FORCEDMODE)
        {
            uint8_t ctrl = BMP280_Read8(BMP280_CONTROL);
            ctrl = (uint8_t)((ctrl & ~0x03U) | BMP280_FORCEDMODE);
            BMP280_Write8(BMP280_CONTROL, ctrl);
            if (!bmp280_wait_measuring_clear(50))
                return -99.0f;
        }

        int32_t adc_T = (int32_t)BMP280_Read24(BMP280_TEMPDATA);
        adc_T >>= 4;

        int32_t var1 = ((((adc_T >> 3) - ((int32_t)t1 << 1))) * ((int32_t)t2)) >> 11;
        int32_t var2 = (((((adc_T >> 4) - ((int32_t)t1)) * ((adc_T >> 4) - ((int32_t)t1))) >> 12) * ((int32_t)t3)) >> 14;
        t_fine = var1 + var2;

        return (float)((t_fine * 5 + 128) >> 8) / 100.0f;
    }

    int32_t BMP280_ReadPressure(void)
    {
        if (BMP280_ReadTemperature() == -99.0f)
            return 0;

        int32_t adc_P = (int32_t)BMP280_Read24(BMP280_PRESSUREDATA);
        adc_P >>= 4;

        int64_t var1 = ((int64_t)t_fine) - 128000;
        int64_t var2 = var1 * var1 * (int64_t)p6;
        var2 = var2 + ((var1 * (int64_t)p5) << 17);
        var2 = var2 + (((int64_t)p4) << 35);
        var1 = ((var1 * var1 * (int64_t)p3) >> 8) + ((var1 * (int64_t)p2) << 12);
        var1 = (((((int64_t)1) << 47) + var1) * (int64_t)p1) >> 33;

        if (var1 == 0)
            return 0;

        int64_t p = 1048576 - adc_P;
        p = (((p << 31) - var2) * 3125) / var1;
        var1 = (((int64_t)p9) * (p >> 13) * (p >> 13)) >> 25;
        var2 = (((int64_t)p8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)p7) << 4);

        return (int32_t)(p / 256);
    }

    uint8_t BMP280_ReadTemperatureAndPressure(float *temperature, int32_t *pressure)
    {
        if (!temperature || !pressure)
            return (uint8_t)-1;

        float t = BMP280_ReadTemperature();
        if (t == -99.0f)
            return (uint8_t)-1;
        *temperature = t;

        int32_t p = BMP280_ReadPressure();
        if (p == 0)
            return (uint8_t)-1;
        *pressure = p;

        return 0;
    }

    float BMP280_ReadAltitude(float sea_level_pa)
    {
        float p = (float)BMP280_ReadPressure();
        if (p <= 0.0f)
            return -9999.0f;
        return 44330.0f * (1.0f - powf(p / sea_level_pa, 0.1903f));
    }
#endif /* BMP280 */

/* =========================
 * ===== BME280 block ======
 * ========================= */
#ifdef BME280
    static uint8_t BME280_Read8(uint8_t addr)
    {
#if (BMP_I2C == 1)
        uint8_t tmp = 0;
        HAL_I2C_Mem_Read(i2c_h, BME280_I2CADDR, addr, I2C_MEMADD_SIZE_8BIT, &tmp, 1, 10);
        return tmp;
#endif
#if (BMP_SPI == 1)
        uint8_t tx[2] = {(uint8_t)(addr | 0x80U), 0x00};
        uint8_t rx[2] = {0};
        cs_low();
        HAL_SPI_TransmitReceive(spi_h, tx, rx, 2, 10);
        cs_high();
        return rx[1];
#endif
    }
    static uint16_t BME280_Read16(uint8_t addr)
    {
#if (BMP_I2C == 1)
        uint8_t tmp[2] = {0};
        HAL_I2C_Mem_Read(i2c_h, BME280_I2CADDR, addr, I2C_MEMADD_SIZE_8BIT, tmp, 2, 10);
        return ((uint16_t)tmp[0] << 8) | tmp[1];
#endif
#if (BMP_SPI == 1)
        uint8_t tx[3] = {(uint8_t)(addr | 0x80U), 0x00, 0x00};
        uint8_t rx[3] = {0};
        cs_low();
        HAL_SPI_TransmitReceive(spi_h, tx, rx, 3, 10);
        cs_high();
        return ((uint16_t)rx[1] << 8) | rx[2];
#endif
    }
    static uint16_t BME280_Read16LE(uint8_t addr)
    {
        uint16_t v = BME280_Read16(addr);
        return (uint16_t)((v >> 8) | (v << 8));
    }
    static void BME280_Write8(uint8_t address, uint8_t data)
    {
#if (BMP_I2C == 1)
        HAL_I2C_Mem_Write(i2c_h, BME280_I2CADDR, address, I2C_MEMADD_SIZE_8BIT, &data, 1, 10);
#endif
#if (BMP_SPI == 1)
        uint8_t tx[2] = {(uint8_t)(address & 0x7FU), data};
        uint8_t rx[2] = {0};
        cs_low();
        HAL_SPI_TransmitReceive(spi_h, tx, rx, 2, 10);
        cs_high();
#endif
    }
    static uint32_t BME280_Read24(uint8_t addr)
    {
#if (BMP_I2C == 1)
        uint8_t tmp[3] = {0};
        HAL_I2C_Mem_Read(i2c_h, BME280_I2CADDR, addr, I2C_MEMADD_SIZE_8BIT, tmp, 3, 10);
        return ((uint32_t)tmp[0] << 16) | ((uint32_t)tmp[1] << 8) | tmp[2];
#endif
#if (BMP_SPI == 1)
        uint8_t tx[4] = {(uint8_t)(addr | 0x80U), 0x00, 0x00, 0x00};
        uint8_t rx[4] = {0};
        cs_low();
        HAL_SPI_TransmitReceive(spi_h, tx, rx, 4, 10);
        cs_high();
        return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
#endif
    }

    static uint8_t BME280_IsReadingCalibration(void)
    {
        uint8_t status = BME280_Read8(BME280_STATUS);
        return (uint8_t)((status & 0x01U) != 0U);
    }
    static bool bme280_wait_measuring_clear(uint32_t timeout_ms)
    {
        return wait_reg_bit_clear(BME280_Read8, BME280_STATUS, BME280_MEASURING, timeout_ms);
    }

    void BME280_SetConfig(uint8_t standby_time, uint8_t filter)
    {
        uint8_t v = (uint8_t)((((standby_time & 0x7U) << 5) | ((filter & 0x7U) << 2)) & 0xFCU);
        BME280_Write8(BME280_CONFIG, v);
    }

#if (BMP_I2C == 1)
    bool BME280_Init(I2C_HandleTypeDef * i2c_handler, uint8_t temperature_resolution, uint8_t pressure_oversampling, uint8_t humidity_oversampling, uint8_t mode)
    {
        if (i2c_handler == NULL)
            return false;
        i2c_h = i2c_handler;
#endif
#if (BMP_SPI == 1)
        bool BME280_Init(SPI_HandleTypeDef * spi_handler, uint8_t temperature_resolution, uint8_t pressure_oversampling, uint8_t humidity_oversampling, uint8_t mode)
        {
            if (spi_handler == NULL || BMPxx_cs_port == NULL)
                return false;
            spi_h = spi_handler;
            cs_low();
            HAL_Delay(5);
            cs_high();
#endif
            if (mode > BME280_NORMALMODE)
                mode = BME280_NORMALMODE;
            _mode = mode;
            if (mode == BME280_FORCEDMODE)
                mode = BME280_SLEEPMODE;

            if (temperature_resolution > BME280_TEMPERATURE_20BIT)
                temperature_resolution = BME280_TEMPERATURE_20BIT;
            _temperature_res = temperature_resolution;

            if (pressure_oversampling > BME280_PRESSURE_ULTRAHIGHRES)
                pressure_oversampling = BME280_PRESSURE_ULTRAHIGHRES;
            _pressure_oversampling = pressure_oversampling;

            if (humidity_oversampling > BME280_HUMIDITY_ULTRAHIGH)
                humidity_oversampling = BME280_HUMIDITY_ULTRAHIGH;
            _humidity_oversampling = humidity_oversampling;

            if (!wait_reg_equals(BME280_Read8, BME280_CHIPID, 0x60, 50))
                return false;

            /* Soft reset then bounded wait (~300ms) */
            BME280_Write8(BME280_SOFTRESET, 0xB6);
            uint32_t start = HAL_GetTick();
            while ((HAL_GetTick() - start) < 300U)
                BMPxx_delay_us(1000);

            /* Wait until NVM copy finishes (status bit 0 == 0) with small timeout */
            start = HAL_GetTick();
            while (BME280_IsReadingCalibration())
            {
                BMPxx_delay_us(1000);
                if ((HAL_GetTick() - start) > 50U)
                    break;
            }

            /* Read calibration data */
            t1 = BME280_Read16LE(BME280_DIG_T1);
            t2 = BME280_Read16LE(BME280_DIG_T2);
            t3 = BME280_Read16LE(BME280_DIG_T3);

            p1 = BME280_Read16LE(BME280_DIG_P1);
            p2 = BME280_Read16LE(BME280_DIG_P2);
            p3 = BME280_Read16LE(BME280_DIG_P3);
            p4 = BME280_Read16LE(BME280_DIG_P4);
            p5 = BME280_Read16LE(BME280_DIG_P5);
            p6 = BME280_Read16LE(BME280_DIG_P6);
            p7 = BME280_Read16LE(BME280_DIG_P7);
            p8 = BME280_Read16LE(BME280_DIG_P8);
            p9 = BME280_Read16LE(BME280_DIG_P9);

            h1 = BME280_Read8(BME280_DIG_H1);
            h2 = (int16_t)BME280_Read16LE(BME280_DIG_H2);
            h3 = BME280_Read8(BME280_DIG_H3);
            h4 = (int16_t)((BME280_Read8(BME280_DIG_H4) << 4) | (BME280_Read8(BME280_DIG_H4 + 1) & 0x0F));
            h5 = (int16_t)((BME280_Read8(BME280_DIG_H5 + 1) << 4) | (BME280_Read8(BME280_DIG_H5) >> 4));
            h6 = (int8_t)BME280_Read8(BME280_DIG_H6);

            /* Humidity oversampling must be set BEFORE ctrl_meas */
            uint8_t hum = (uint8_t)(BME280_Read8(BME280_HUM_CONTROL) & 0xF8U);
            hum |= (uint8_t)(_humidity_oversampling & 0x07U);
            BME280_Write8(BME280_HUM_CONTROL, hum);

            /* ctrl_meas: temp, pressure, mode */
            BME280_Write8(BME280_CONTROL, (uint8_t)((temperature_resolution << 5) | (pressure_oversampling << 2) | mode));

            if (_mode == BME280_NORMALMODE)
                BME280_SetConfig(BME280_STANDBY_MS_0_5, BME280_FILTER_OFF);

            return true;
        }

        float BME280_ReadTemperature(void)
        {
            if (_mode == BME280_FORCEDMODE)
            {
                uint8_t ctrl = BME280_Read8(BME280_CONTROL);
                ctrl = (uint8_t)((ctrl & ~0x03U) | BME280_FORCEDMODE);
                BME280_Write8(BME280_CONTROL, ctrl);

                if (!bme280_wait_measuring_clear(50))
                    return -99.0f;
            }

            int32_t adc_T = (int32_t)BME280_Read24(BME280_TEMPDATA);
            if (adc_T == 0x800000)
                return -99.0f;
            adc_T >>= 4;

            int32_t var1 = ((((adc_T >> 3) - ((int32_t)t1 << 1))) * ((int32_t)t2)) >> 11;
            int32_t var2 = (((((adc_T >> 4) - ((int32_t)t1)) * ((adc_T >> 4) - ((int32_t)t1))) >> 12) * ((int32_t)t3)) >> 14;
            t_fine = var1 + var2;

            return (float)((t_fine * 5 + 128) >> 8) / 100.0f;
        }

        int32_t BME280_ReadPressure(void)
        {
            if (BME280_ReadTemperature() == -99.0f)
                return 0;

            int32_t adc_P = (int32_t)BME280_Read24(BME280_PRESSUREDATA);
            adc_P >>= 4;

            int64_t var1 = ((int64_t)t_fine) - 128000;
            int64_t var2 = var1 * var1 * (int64_t)p6;
            var2 = var2 + ((var1 * (int64_t)p5) << 17);
            var2 = var2 + (((int64_t)p4) << 35);
            var1 = ((var1 * var1 * (int64_t)p3) >> 8) + ((var1 * (int64_t)p2) << 12);
            var1 = (((((int64_t)1) << 47) + var1) * (int64_t)p1) >> 33;

            if (var1 == 0)
                return 0;

            int64_t p = 1048576 - adc_P;
            p = (((p << 31) - var2) * 3125) / var1;
            var1 = (((int64_t)p9) * (p >> 13) * (p >> 13)) >> 25;
            var2 = (((int64_t)p8) * p) >> 19;
            p = ((p + var1 + var2) >> 8) + (((int64_t)p7) << 4);

            return (int32_t)(p / 256);
        }

        float BME280_ReadHumidity(void)
        {
            if (BME280_ReadTemperature() == -99.0f)
                return -99.0f;

            int32_t adc_H = (int32_t)BME280_Read16(BME280_HUMIDDATA);
            if (adc_H == 0x8000)
                return -99.0f;

            int32_t v_x1_u32r = (t_fine - ((int32_t)76800));

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
            return h / 1024.0F;
        }

        uint8_t BME280_ReadTemperatureAndPressureAndHumidity(float *temperature, int32_t *pressure, float *humidity)
        {
            if (!temperature || !pressure || !humidity)
                return (uint8_t)-1;

            float t = BME280_ReadTemperature();
            if (t == -99.0f)
                return (uint8_t)-1;
            *temperature = t;

            int32_t p = BME280_ReadPressure();
            if (p == 0)
                return (uint8_t)-1;
            *pressure = p;

            float h = BME280_ReadHumidity();
            if (h == -99.0f)
                return (uint8_t)-1;
            *humidity = h;

            return 0;
        }

        float BME280_ReadAltitude(float sea_level_pa)
        {
            float p = (float)BME280_ReadPressure();
            if (p <= 0.0f)
                return -9999.0f;
            return 44330.0f * (1.0f - powf(p / sea_level_pa, 0.1903f));
        }
#endif /* BME280 */