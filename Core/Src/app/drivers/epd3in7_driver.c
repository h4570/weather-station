/*****************************************************************************
 * | File          :   epd3in7_driver.h
 * | Author        :   h4570 (based on Waveshare code)
 * | Function      :   quasi-async alternative driver for official 3.7inch e-paper display (Waveshare 20123)
 *----------------
 * | Version       :   V1.0
 * | Date          :   2025-09-19
 ******************************************************************************/

#include "app/drivers/epd3in7_driver.h"
#include "string.h"
#include "assert.h"

#define EPD3IN7_DRIVER_TRY(expr)           \
    do                                     \
    {                                      \
        epd3in7_driver_status _s = (expr); \
        if (_s != EPD3IN7_DRIVER_OK)       \
        {                                  \
            err = _s;                      \
            goto fail;                     \
        }                                  \
    } while (0)

typedef enum
{
    EPD_CMD_GATE_SETTING = 0x01,
    EPD_CMD_POWEROFF = 0x02,
    EPD_CMD_SLEEP2 = 0x07,
    EPD_CMD_GATE_VOLTAGE = 0x03,
    EPD_CMD_GATE_VOLTAGE_SOURCE = 0x04,
    EPD_CMD_BOOSTER_SOFT_START_CONTROL = 0x0C,
    EPD_CMD_DEEP_SLEEP = 0x10,
    EPD_CMD_DATA_ENTRY_SEQUENCE = 0x11,
    EPD_CMD_SW_RESET = 0x12,
    EPD_CMD_TEMPERATURE_SENSOR_SELECTION = 0x18,
    EPD_CMD_TEMPERATURE_SENSOR_WRITE = 0x1A,
    EPD_CMD_TEMPERATURE_SENSOR_READ = 0x1B,
    EPD_CMD_DISPLAY_UPDATE_SEQUENCE = 0x20,
    EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING = 0x22,
    EPD_CMD_WRITE_RAM = 0x24,
    EPD_CMD_WRITE_RAM2 = 0x26,
    EPD_CMD_WRITE_VCOM_REGISTER = 0x2C,
    EPD_CMD_WRITE_LUT_REGISTER = 0x32,
    EPD_CMD_DISPLAY_OPTION = 0x37,
    EPD_CMD_BORDER_WAVEFORM_CONTROL = 0x3C,
    EPD_CMD_SET_RAMX_START_END = 0x44,
    EPD_CMD_SET_RAMY_START_END = 0x45,
    EPD_CMD_AUTO_WRITE_RED_RAM_REG_PATTERN = 0x46,
    EPD_CMD_AUTO_WRITE_BW_RAM_REG_PATTERN = 0x47,
    EPD_CMD_SET_RAMX_COUNTER = 0x4E,
    EPD_CMD_SET_RAMY_COUNTER = 0x4F,
    EPD_CMD_UNKNOWN_0x49 = 0x49,
    EPD_CMD_SLEEP = 0x50 // Not sure about this one
} epd3in7_driver_cmd;

static const uint8_t epd3in7_driver_lut_4_gray_gc[] =
    {
        0x2A, 0x06, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1
        0x28, 0x06, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 2
        0x20, 0x06, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3
        0x14, 0x06, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
        0x00, 0x02, 0x02, 0x0A, 0x00, 0x00, 0x00, 0x08, 0x08, 0x02, // 6
        0x00, 0x02, 0x02, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 10
        0x22, 0x22, 0x22, 0x22, 0x22};

static const uint8_t epd3in7_driver_lut_1_gray_gc[] =
    {
        0x2A, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1
        0x05, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 2
        0x2A, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3
        0x05, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
        0x00, 0x02, 0x03, 0x0A, 0x00, 0x02, 0x06, 0x0A, 0x05, 0x00, // 6
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 10
        0x22, 0x22, 0x22, 0x22, 0x22};

static const uint8_t epd3in7_driver_lut_1_gray_du[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1
        0x01, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0A, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
        0x00, 0x00, 0x05, 0x05, 0x00, 0x05, 0x03, 0x05, 0x05, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x22, 0x22, 0x22, 0x22, 0x22};

static const uint8_t epd3in7_driver_lut_1_gray_a2[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 1
        0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 2
        0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 3
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 5
        0x00, 0x00, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 6
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 7
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 9
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 10
        0x22, 0x22, 0x22, 0x22, 0x22};

static void epd3in7_driver_send_begin(epd3in7_driver_handle *handle)
{
    HAL_GPIO_WritePin(handle->pins.cs_port, handle->pins.cs_pin, GPIO_PIN_RESET);
    handle->is_cs_low = true;
    handle->is_cs_low_has_value = true;
}

static void epd3in7_driver_send_end(epd3in7_driver_handle *handle)
{
    HAL_GPIO_WritePin(handle->pins.cs_port, handle->pins.cs_pin, GPIO_PIN_SET);
    handle->is_cs_low = false;
    handle->is_cs_low_has_value = true;
}

static epd3in7_driver_status epd3in7_driver_send_command(epd3in7_driver_handle *handle, const epd3in7_driver_cmd reg)
{
    HAL_GPIO_WritePin(handle->pins.dc_port, handle->pins.dc_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef res = HAL_SPI_Transmit(handle->spi_handle, (uint8_t *)&reg, 1, EPD3IN7_DRIVER_SPI_TIMEOUT);

    if (res != HAL_OK)
    {
        return EPD3IN7_DRIVER_ERR_HAL;
    }

    return EPD3IN7_DRIVER_OK;
}

static epd3in7_driver_status epd3in7_driver_send_data(epd3in7_driver_handle *handle, const uint8_t data)
{
    HAL_GPIO_WritePin(handle->pins.dc_port, handle->pins.dc_pin, GPIO_PIN_SET);
    HAL_StatusTypeDef res = HAL_SPI_Transmit(handle->spi_handle, &data, 1, EPD3IN7_DRIVER_SPI_TIMEOUT);

    if (res != HAL_OK)
    {
        return EPD3IN7_DRIVER_ERR_HAL;
    }

    return EPD3IN7_DRIVER_OK;
}

static epd3in7_driver_status epd3in7_driver_send_data_many(const epd3in7_driver_handle *handle, const uint8_t *data, uint32_t len)
{
    HAL_GPIO_WritePin(handle->pins.dc_port, handle->pins.dc_pin, GPIO_PIN_SET);
    HAL_StatusTypeDef res = HAL_SPI_Transmit(handle->spi_handle, (uint8_t *)data, len, EPD3IN7_DRIVER_SPI_TIMEOUT);

    if (res != HAL_OK)
    {
        return EPD3IN7_DRIVER_ERR_HAL;
    }

    return EPD3IN7_DRIVER_OK;
}

static epd3in7_driver_lut_type epd3in7_driver_mode_to_lut(const epd3in7_driver_mode mode, const bool is_1_color)
{
    if (is_1_color)
    {
        if (mode == EPD3IN7_DRIVER_MODE_GC)
        {
            return EPD3IN7_DRIVER_LUT_1_GRAY_GC;
        }
        else if (mode == EPD3IN7_DRIVER_MODE_DU)
        {
            return EPD3IN7_DRIVER_LUT_1_GRAY_DU;
        }
        else if (mode == EPD3IN7_DRIVER_MODE_A2)
        {
            return EPD3IN7_DRIVER_LUT_1_GRAY_A2;
        }
    }
    else
    {
        return EPD3IN7_DRIVER_LUT_4_GRAY_GC;
    }

    return EPD3IN7_DRIVER_LUT_1_GRAY_GC;
}

/**
 * REQUIRES: CS pin to be low
 */
static epd3in7_driver_status epd3in7_driver_load_lut(epd3in7_driver_handle *handle, const epd3in7_driver_lut_type lut)
{
    if (handle->last_lut_has_value && handle->last_lut == lut)
    {
        return EPD3IN7_DRIVER_OK;
    }

    if (handle->is_cs_low == false || handle->is_cs_low_has_value == false)
    {
        return EPD3IN7_DRIVER_ERR_PARAM;
    }

    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;

    epd3in7_driver_status res = epd3in7_driver_send_command(handle, EPD_CMD_WRITE_LUT_REGISTER);

    if (res != EPD3IN7_DRIVER_OK)
    {
        return res;
    }

    const uint8_t *lut_ptr = NULL;

    if (lut == EPD3IN7_DRIVER_LUT_4_GRAY_GC)
    {
        lut_ptr = epd3in7_driver_lut_4_gray_gc;
    }
    else if (lut == EPD3IN7_DRIVER_LUT_1_GRAY_GC)
    {
        lut_ptr = epd3in7_driver_lut_1_gray_gc;
    }
    else if (lut == EPD3IN7_DRIVER_LUT_1_GRAY_DU)
    {
        lut_ptr = epd3in7_driver_lut_1_gray_du;
    }
    else if (lut == EPD3IN7_DRIVER_LUT_1_GRAY_A2)
    {
        lut_ptr = epd3in7_driver_lut_1_gray_a2;
    }

    if (lut_ptr)
    {
        res = epd3in7_driver_send_data_many(handle, lut_ptr, 105);

        if (res != EPD3IN7_DRIVER_OK)
        {
            return res;
        }
    }

    handle->last_lut_has_value = true;
    handle->last_lut = lut;

    return err;
}

epd3in7_driver_handle epd3in7_driver_create(const epd3in7_driver_pins pins, SPI_HandleTypeDef *spi_handle, const bool busy_active_high)
{
    epd3in7_driver_handle handle;

    handle.width = EPD3IN7_WIDTH;
    handle.height = EPD3IN7_HEIGHT;

    handle.pins = pins;
    handle.spi_handle = spi_handle;
    handle.busy_active_high = busy_active_high;

    handle.last_lut_has_value = false;
    handle.last_lut = EPD3IN7_DRIVER_LUT_4_GRAY_GC;

    handle.is_cs_low = false;
    handle.is_cs_low_has_value = false;

    return handle;
}

epd3in7_driver_status epd3in7_driver_busy_wait_for_idle(const epd3in7_driver_handle *handle)
{
    uint8_t busy;
    uint32_t start = HAL_GetTick();

    do
    {
        busy = HAL_GPIO_ReadPin(handle->pins.busy_port, handle->pins.busy_pin);
        if (handle->busy_active_high)
        {
            if (busy == GPIO_PIN_RESET)
                break;
        }
        else
        {
            if (busy == GPIO_PIN_SET)
                break;
        }
        if ((HAL_GetTick() - start) > EPD3IN7_DRIVER_BUSY_TIMEOUT)
        {
            return EPD3IN7_DRIVER_ERR_TIMEOUT;
        }
    } while (handle->busy_active_high ? busy == GPIO_PIN_SET : busy == GPIO_PIN_RESET);

    HAL_Delay(200);
    return EPD3IN7_DRIVER_OK;
}

epd3in7_driver_status epd3in7_driver_sleep(epd3in7_driver_handle *handle, const epd3in7_driver_sleep_mode mode)
{
    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;

    if (mode == EPD3IN7_DRIVER_SLEEP_DEEP)
    {
        epd3in7_driver_send_begin(handle);
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD3IN7_DRIVER_SLEEP_DEEP));
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x03));
        epd3in7_driver_send_end(handle);
    }
    else
    {
        epd3in7_driver_send_begin(handle);
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SLEEP));
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0xF7));
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_POWEROFF));
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SLEEP2));
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0xA5));
        epd3in7_driver_send_end(handle);
    }

    return EPD3IN7_DRIVER_OK;

fail:
    epd3in7_driver_send_end(handle);
    return err;
}

void epd3in7_driver_reset(const epd3in7_driver_handle *handle)
{
    HAL_GPIO_WritePin(handle->pins.reset_port, handle->pins.reset_pin, GPIO_PIN_SET);
    HAL_Delay(300);
    HAL_GPIO_WritePin(handle->pins.reset_port, handle->pins.reset_pin, GPIO_PIN_RESET);
    HAL_Delay(3);
    HAL_GPIO_WritePin(handle->pins.reset_port, handle->pins.reset_pin, GPIO_PIN_SET);
    HAL_Delay(300);
}

epd3in7_driver_status epd3in7_driver_init_4_gray(epd3in7_driver_handle *handle)
{
    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;

    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    epd3in7_driver_reset(handle);

    epd3in7_driver_send_begin(handle);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SW_RESET));

    HAL_Delay(300);

    if (epd3in7_driver_send_command(handle, EPD_CMD_AUTO_WRITE_RED_RAM_REG_PATTERN) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_data(handle, 0xF7) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_busy_wait_for_idle(handle) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_command(handle, EPD_CMD_AUTO_WRITE_BW_RAM_REG_PATTERN) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_data(handle, 0xF7) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_busy_wait_for_idle(handle) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_GATE_SETTING) != EPD3IN7_DRIVER_OK)
        goto fail;
    uint8_t gate_setting[] = {0xDF, 0x01, 0x00};
    if (epd3in7_driver_send_data_many(handle, gate_setting, 3) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_GATE_VOLTAGE) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_data(handle, 0x00) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_GATE_VOLTAGE_SOURCE) != EPD3IN7_DRIVER_OK)
        goto fail;
    uint8_t gate_voltage_source[] = {0x41, 0xA8, 0x32};
    if (epd3in7_driver_send_data_many(handle, gate_voltage_source, 3) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_DATA_ENTRY_SEQUENCE) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_data(handle, 0x03) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_BORDER_WAVEFORM_CONTROL) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_data(handle, 0x03) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_BOOSTER_SOFT_START_CONTROL) != EPD3IN7_DRIVER_OK)
        goto fail;
    uint8_t booster[] = {0xAE, 0xC7, 0xC3, 0xC0, 0xC0};
    if (epd3in7_driver_send_data_many(handle, booster, 5) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_TEMPERATURE_SENSOR_SELECTION) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_data(handle, 0x80) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_WRITE_VCOM_REGISTER) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_data(handle, 0x44) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_OPTION) != EPD3IN7_DRIVER_OK)
        goto fail;
    uint8_t display_option[10] = {0};
    if (epd3in7_driver_send_data_many(handle, display_option, 10) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_START_END) != EPD3IN7_DRIVER_OK)
        goto fail;
    uint8_t ramx[] = {0x00, 0x00, 0x17, 0x01};
    if (epd3in7_driver_send_data_many(handle, ramx, 4) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_START_END) != EPD3IN7_DRIVER_OK)
        goto fail;
    uint8_t ramy[] = {0x00, 0x00, 0xDF, 0x01};
    if (epd3in7_driver_send_data_many(handle, ramy, 4) != EPD3IN7_DRIVER_OK)
        goto fail;

    if (epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING) != EPD3IN7_DRIVER_OK)
        goto fail;
    if (epd3in7_driver_send_data(handle, 0xCF) != EPD3IN7_DRIVER_OK)
        goto fail;

    epd3in7_driver_send_end(handle);

    handle->last_lut_has_value = false;
    return EPD3IN7_DRIVER_OK;
fail:
    epd3in7_driver_send_end(handle);
    return err;
}

epd3in7_driver_status epd3in7_driver_init_1_gray(epd3in7_driver_handle *handle)
{
    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;

    epd3in7_driver_reset(handle);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    epd3in7_driver_send_begin(handle);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SW_RESET));
    HAL_Delay(300);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_AUTO_WRITE_RED_RAM_REG_PATTERN));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0xF7));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_AUTO_WRITE_BW_RAM_REG_PATTERN));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0xF7));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_GATE_SETTING));
    uint8_t gate_setting[] = {0xDF, 0x01, 0x00};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, gate_setting, 3));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_GATE_VOLTAGE));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_GATE_VOLTAGE_SOURCE));
    uint8_t gate_voltage_source[] = {0x41, 0xA8, 0x32};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, gate_voltage_source, 3));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DATA_ENTRY_SEQUENCE));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x03));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_BORDER_WAVEFORM_CONTROL));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x03));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_BOOSTER_SOFT_START_CONTROL));
    uint8_t booster[] = {0xAE, 0xC7, 0xC3, 0xC0, 0xC0};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, booster, 5));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_TEMPERATURE_SENSOR_SELECTION));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x80));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_WRITE_VCOM_REGISTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x44));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_OPTION));
    uint8_t display_option[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0xFF, 0xFF};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, display_option, 10));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_START_END));
    uint8_t ramx[] = {0x00, 0x00, 0x17, 0x01};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ramx, 4));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_START_END));
    uint8_t ramy[] = {0x00, 0x00, 0xDF, 0x01};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ramy, 4));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0xCF));

    epd3in7_driver_send_end(handle);

    handle->last_lut_has_value = false;
    return EPD3IN7_DRIVER_OK;
fail:
    epd3in7_driver_send_end(handle);
    return err;
}

epd3in7_driver_status epd3in7_driver_clear_4_gray(epd3in7_driver_handle *handle)
{
    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;
    uint16_t width = (EPD3IN7_WIDTH % 8 == 0) ? (EPD3IN7_WIDTH / 8) : (EPD3IN7_WIDTH / 8 + 1);
    uint16_t height = EPD3IN7_HEIGHT;

    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    epd3in7_driver_send_begin(handle);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_UNKNOWN_0x49));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_WRITE_RAM));

    uint32_t total = width * height;
    uint8_t ff_buf[32];
    memset(ff_buf, 0xFF, sizeof(ff_buf));
    uint32_t left = total;
    while (left > 0)
    {
        uint32_t chunk = left > sizeof(ff_buf) ? sizeof(ff_buf) : left;
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ff_buf, chunk));
        left -= chunk;
    }

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_WRITE_RAM2));

    left = total;
    while (left > 0)
    {
        uint32_t chunk = left > sizeof(ff_buf) ? sizeof(ff_buf) : left;
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ff_buf, chunk));
        left -= chunk;
    }

    EPD3IN7_DRIVER_TRY(epd3in7_driver_load_lut(handle, EPD3IN7_DRIVER_LUT_4_GRAY_GC));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0xC7));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_driver_send_end(handle);

    return EPD3IN7_DRIVER_OK;

fail:
    epd3in7_driver_send_end(handle);
    return err;
}

epd3in7_driver_status epd3in7_driver_clear_1_gray(epd3in7_driver_handle *handle, const epd3in7_driver_mode mode)
{
    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;
    const uint16_t image_counter = EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 8;

    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    epd3in7_driver_send_begin(handle);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_WRITE_RAM));

    uint8_t ff_buf[32];
    memset(ff_buf, 0xFF, sizeof(ff_buf));
    uint32_t left = image_counter;
    while (left > 0)
    {
        uint32_t chunk = left > sizeof(ff_buf) ? sizeof(ff_buf) : left;
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ff_buf, chunk));
        left -= chunk;
    }

    epd3in7_driver_lut_type lut_type = epd3in7_driver_mode_to_lut(mode, true);
    EPD3IN7_DRIVER_TRY(epd3in7_driver_load_lut(handle, lut_type));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_driver_send_end(handle);

    return EPD3IN7_DRIVER_OK;

fail:
    epd3in7_driver_send_end(handle);
    return err;
}

epd3in7_driver_status epd3in7_driver_display_4_gray(epd3in7_driver_handle *handle, const uint8_t *image)
{
    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;
    const uint16_t image_counter = EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 8;

    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    epd3in7_driver_send_begin(handle);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, 0x49));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_WRITE_RAM));

    // pętla z konwersją, nie można użyć send_data_many
    for (uint32_t i = 0; i < image_counter; i++)
    {
        uint8_t temp3 = 0;
        for (uint32_t j = 0; j < 2; j++)
        {
            uint8_t temp1 = image[i * 2 + j];
            for (uint32_t k = 0; k < 2; k++)
            {
                uint8_t temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01; // white
                else if (temp2 == 0x00)
                    temp3 |= 0x00; // black
                else if (temp2 == 0x80)
                    temp3 |= 0x00; // gray1
                else               // 0x40
                    temp3 |= 0x01; // gray2
                temp3 <<= 1;
                temp1 <<= 2;
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0) // white
                    temp3 |= 0x01;
                else if (temp2 == 0x00) // black
                    temp3 |= 0x00;
                else if (temp2 == 0x80)
                    temp3 |= 0x00; // gray1
                else               // 0x40
                    temp3 |= 0x01; // gray2
                if (j != 1 || k != 1)
                    temp3 <<= 1;
                temp1 <<= 2;
            }
        }
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, temp3));
    }

    // new  data
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_WRITE_RAM2));

    for (uint32_t i = 0; i < image_counter; i++)
    {
        uint8_t temp3 = 0;
        for (uint32_t j = 0; j < 2; j++)
        {
            uint8_t temp1 = image[i * 2 + j];
            for (uint32_t k = 0; k < 2; k++)
            {
                uint8_t temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0)
                    temp3 |= 0x01; // white
                else if (temp2 == 0x00)
                    temp3 |= 0x00; // black
                else if (temp2 == 0x80)
                    temp3 |= 0x01; // gray1
                else               // 0x40
                    temp3 |= 0x00; // gray2
                temp3 <<= 1;
                temp1 <<= 2;
                temp2 = temp1 & 0xC0;
                if (temp2 == 0xC0) // white
                    temp3 |= 0x01;
                else if (temp2 == 0x00) // black
                    temp3 |= 0x00;
                else if (temp2 == 0x80)
                    temp3 |= 0x01; // gray1
                else               // 0x40
                    temp3 |= 0x00; // gray2
                if (j != 1 || k != 1)
                    temp3 <<= 1;
                temp1 <<= 2;
            }
        }
        EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, temp3));
    }

    EPD3IN7_DRIVER_TRY(epd3in7_driver_load_lut(handle, EPD3IN7_DRIVER_LUT_4_GRAY_GC));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0xC7));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_driver_send_end(handle);

    return EPD3IN7_DRIVER_OK;
fail:
    epd3in7_driver_send_end(handle);
    return err;
}

epd3in7_driver_status epd3in7_driver_display_1_gray(epd3in7_driver_handle *handle, const uint8_t *image, const epd3in7_driver_mode mode)
{
    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;

    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    epd3in7_driver_send_begin(handle);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_START_END));
    uint8_t ramx[] = {0x00, 0x00, 0x17, 0x01};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ramx, 4));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_START_END));
    uint8_t ramy[] = {0x00, 0x00, 0xDF, 0x01};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ramy, 4));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, 0x00));

    const uint16_t image_counter = EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 8;

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_WRITE_RAM));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, image, image_counter));

    epd3in7_driver_lut_type lut_type = epd3in7_driver_mode_to_lut(mode, true);
    EPD3IN7_DRIVER_TRY(epd3in7_driver_load_lut(handle, lut_type));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_driver_send_end(handle);

    return EPD3IN7_DRIVER_OK;
fail:
    epd3in7_driver_send_end(handle);
    return err;
}

epd3in7_driver_status epd3in7_driver_display_1_gray_top(epd3in7_driver_handle *handle, const uint8_t *image, const uint16_t y_end_exclusive, const epd3in7_driver_mode mode)
{
    epd3in7_driver_status err = EPD3IN7_DRIVER_OK;
    uint16_t x_start = 0;
    uint16_t y_start = 0;
    uint16_t x_end = EPD3IN7_WIDTH;

    if (image == NULL)
    {
        return EPD3IN7_DRIVER_ERR_PARAM; // null pointer
    }
    else if (y_end_exclusive == 0 || y_end_exclusive > EPD3IN7_HEIGHT)
    {
        return EPD3IN7_DRIVER_ERR_PARAM; // invalid height
    }

    uint16_t y_end_exclusive_temp = y_end_exclusive;

    x_end -= 1;
    y_end_exclusive_temp -= 1;

    EPD3IN7_DRIVER_TRY(epd3in7_driver_busy_wait_for_idle(handle));

    epd3in7_driver_send_begin(handle);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_START_END));
    uint8_t ramx[4] = {
        x_start & 0xff,
        (x_start >> 8) & 0x03,
        x_end & 0xff,
        (x_end >> 8) & 0x03};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ramx, 4));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_START_END));
    uint8_t ramy[4] = {
        y_start & 0xff,
        (y_start >> 8) & 0x03,
        y_end_exclusive_temp & 0xff,
        (y_end_exclusive_temp >> 8) & 0x03};
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, ramy, 4));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMX_COUNTER)); // SET_RAM_X_ADDRESS_COUNTER
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, x_start & 0xFF));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_SET_RAMY_COUNTER)); // SET_RAM_Y_ADDRESS_COUNTER
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, y_start & 0xFF));
    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data(handle, (y_start >> 8) & 0xFF));

    uint16_t width = (x_end - x_start) % 8 == 0 ? (x_end - x_start) / 8 : (x_end - x_start) / 8 + 1;
    uint16_t image_counter = width * (y_end_exclusive - y_start);

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_WRITE_RAM));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_data_many(handle, image, image_counter));

    epd3in7_driver_lut_type lut_type = epd3in7_driver_mode_to_lut(mode, true);
    EPD3IN7_DRIVER_TRY(epd3in7_driver_load_lut(handle, lut_type));

    EPD3IN7_DRIVER_TRY(epd3in7_driver_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_driver_send_end(handle);

    return 0;

fail:
    epd3in7_driver_send_end(handle);
    return err;
}

// === DMA / SPI BUS MANAGER IMPLEMENTATION ===

// Command and data bytes for DMA transactions.
// These are used to build spi_bus_transaction objects.

uint8_t epd3in7_driver_dma_ramx_start_end_payload[] = {0x00, 0x00, 0x17, 0x01};
uint8_t epd3in7_driver_dma_ramy_start_end_payload[] = {0x00, 0x00, 0xDF, 0x01};
uint8_t epd3in7_driver_dma_ramx_counter_payload = 0x00;
uint8_t epd3in7_driver_dma_ramy_counter_payload[] = {0x00, 0x00};
uint8_t epd3in7_driver_dma_ramx_start_end = EPD_CMD_SET_RAMX_START_END;
uint8_t epd3in7_driver_dma_ramy_start_end = EPD_CMD_SET_RAMY_START_END;
uint8_t epd3in7_driver_dma_ramx_counter = EPD_CMD_SET_RAMX_COUNTER;
uint8_t epd3in7_driver_dma_ramy_counter = EPD_CMD_SET_RAMY_COUNTER;
uint8_t epd3in7_driver_dma_write_ram = EPD_CMD_WRITE_RAM; // 0x24
uint8_t epd3in7_driver_dma_display_update = EPD_CMD_DISPLAY_UPDATE_SEQUENCE;
uint8_t epd3in7_driver_dma_cmd_lut = EPD_CMD_WRITE_LUT_REGISTER;
uint8_t epd3in7_driver_dma_sleep_deep = EPD3IN7_DRIVER_SLEEP_DEEP;
uint8_t epd3in7_driver_dma_sleep = EPD_CMD_SLEEP;
uint8_t epd3in7_driver_dma_power_off = EPD_CMD_POWEROFF;
uint8_t epd3in7_driver_dma_sleep2 = EPD_CMD_SLEEP2;

/* Wait predicate: returns true when the panel is ready (BUSY de-asserted). */
static bool epd3in7_wait_ready_predicate(void *user)
{
    epd3in7_driver_handle *h = (epd3in7_driver_handle *)user;
    GPIO_PinState s = HAL_GPIO_ReadPin(h->pins.busy_port, h->pins.busy_pin);
    /* BUSY polarity: active high -> ready when LOW; active low -> ready when HIGH */
    if (h->busy_active_high)
    {
        return (s == GPIO_PIN_RESET);
    }
    else
    {
        return (s == GPIO_PIN_SET);
    }
}

/* Build a "single byte command" transaction. */
static spi_bus_transaction epd_tx_cmd(epd3in7_driver_handle *h,
                                      spi_bus_gpio cs, spi_bus_gpio dc,
                                      uint8_t *cmd)
{
    spi_bus_transaction t = {0};
    t.cs = cs;
    t.dc = dc;
    t.dc_mode = SPI_BUS_DC_COMMAND;
    /* Keep current CR1/CR2 as-is (SPI already configured for this device) */
    t.cr1 = h->spi_handle->Instance->CR1;
    t.cr2 = h->spi_handle->Instance->CR2;
    t.tx = cmd;
    t.rx = NULL;
    t.len = 1;
    t.dir = SPI_BUS_DIR_TX;
    t.spi_timeout = HAL_MAX_DELAY;
    t.wait_ready = NULL;
    t.wait_timeout_ms = 0;
    t.on_done = NULL;
    t.on_half = NULL;
    t.on_error = NULL;
    t.user = NULL;
    return t;
}

/* Build a "data block" transaction. Optionally attach a BUSY wait after it. */
static spi_bus_transaction epd_tx_data(epd3in7_driver_handle *h,
                                       spi_bus_gpio cs, spi_bus_gpio dc,
                                       const uint8_t *buf, uint32_t len, bool wait_busy_after)
{
    assert(len <= 65535);

    spi_bus_transaction t = {0};
    t.cs = cs;
    t.dc = dc;
    t.dc_mode = SPI_BUS_DC_DATA;
    t.cr1 = h->spi_handle->Instance->CR1;
    t.cr2 = h->spi_handle->Instance->CR2;
    t.tx = buf;
    t.rx = NULL;
    t.len = (uint16_t)len; /* If len > 65535, enqueue in chunks before calling this. */
    t.dir = SPI_BUS_DIR_TX;
    t.spi_timeout = HAL_MAX_DELAY;
    t.on_done = NULL;
    t.on_half = NULL;
    t.on_error = NULL;

    if (wait_busy_after)
    {
        t.wait_ready = epd3in7_wait_ready_predicate;
        t.wait_timeout_ms = EPD3IN7_DRIVER_BUSY_TIMEOUT;
        t.user = h;
    }
    else
    {
        t.wait_ready = NULL;
        t.wait_timeout_ms = 0;
        t.user = NULL;
    }

    return t;
}

static spi_bus_transaction epd_tx_cmd_wait(epd3in7_driver_handle *h,
                                           spi_bus_gpio cs, spi_bus_gpio dc,
                                           uint8_t *cmd, bool wait_busy_after)
{
    spi_bus_transaction t = {0};
    t.cs = cs;
    t.dc = dc;
    t.dc_mode = SPI_BUS_DC_COMMAND; // DC=0
    t.cr1 = h->spi_handle->Instance->CR1;
    t.cr2 = h->spi_handle->Instance->CR2;
    t.tx = cmd;
    t.rx = NULL;
    t.len = 1;
    t.dir = SPI_BUS_DIR_TX;
    t.spi_timeout = HAL_MAX_DELAY;

    if (wait_busy_after)
    {
        t.wait_ready = epd3in7_wait_ready_predicate;
        t.wait_timeout_ms = EPD3IN7_DRIVER_BUSY_TIMEOUT;
        t.user = h;
    }
    else
    {
        t.wait_ready = NULL;
        t.wait_timeout_ms = 0;
        t.user = NULL;
    }

    return t;
}

/* Enqueue small data buffer (command's payload). */
static spi_bus_transaction epd_tx_payload(epd3in7_driver_handle *h,
                                          spi_bus_gpio cs, spi_bus_gpio dc,
                                          const uint8_t *buf, uint16_t len)
{
    return epd_tx_data(h, cs, dc, buf, len, false);
}

/* Enqueue a LUT write (mirrors epd3in7_driver_load_lut, but queue-based). */
static epd3in7_driver_status epd3in7_enqueue_lut(epd3in7_driver_handle *h,
                                                 spi_bus_manager *mgr,
                                                 spi_bus_gpio cs, spi_bus_gpio dc,
                                                 epd3in7_driver_lut_type lut)
{
    if (h->last_lut_has_value && h->last_lut == lut)
    {
        return EPD3IN7_DRIVER_OK;
    }

    const uint8_t *lut_ptr = NULL;
    if (lut == EPD3IN7_DRIVER_LUT_4_GRAY_GC)
        lut_ptr = epd3in7_driver_lut_4_gray_gc;
    else if (lut == EPD3IN7_DRIVER_LUT_1_GRAY_GC)
        lut_ptr = epd3in7_driver_lut_1_gray_gc;
    else if (lut == EPD3IN7_DRIVER_LUT_1_GRAY_DU)
        lut_ptr = epd3in7_driver_lut_1_gray_du;
    else if (lut == EPD3IN7_DRIVER_LUT_1_GRAY_A2)
        lut_ptr = epd3in7_driver_lut_1_gray_a2;

    if (!lut_ptr)
        return EPD3IN7_DRIVER_ERR_PARAM;

    spi_bus_transaction t_cmd = epd_tx_cmd(h, cs, dc, &epd3in7_driver_dma_cmd_lut);
    if (spi_bus_manager_submit(mgr, &t_cmd) != SPI_BUS_MANAGER_OK)
        return EPD3IN7_DRIVER_SPI_BUS_ERR;

    spi_bus_transaction t_data = epd_tx_payload(h, cs, dc, lut_ptr, 105);
    if (spi_bus_manager_submit(mgr, &t_data) != SPI_BUS_MANAGER_OK)
        return EPD3IN7_DRIVER_SPI_BUS_ERR;

    h->last_lut_has_value = true;
    h->last_lut = lut;
    return EPD3IN7_DRIVER_OK;
}

epd3in7_driver_status epd3in7_driver_display_1_gray_dma(epd3in7_driver_handle *handle,
                                                        spi_bus_manager *mgr,
                                                        const uint8_t *image,
                                                        const epd3in7_driver_mode mode)
{
    if (!handle || !mgr || !image)
        return EPD3IN7_DRIVER_ERR_PARAM;

    /* Prepare GPIO roles for the manager based on driver pins. */
    spi_bus_gpio cs = {handle->pins.cs_port, handle->pins.cs_pin, true};  /* active low */
    spi_bus_gpio dc = {handle->pins.dc_port, handle->pins.dc_pin, false}; /* data=HIGH */

    /* Sequence mirrors the blocking epd3in7_driver_display_1_gray() */

    /* SET_RAMX_START_END */
    {
        spi_bus_transaction tr = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_ramx_start_end);

        if (spi_bus_manager_submit(mgr, &tr) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr2 = epd_tx_payload(handle, cs, dc, epd3in7_driver_dma_ramx_start_end_payload, sizeof(epd3in7_driver_dma_ramx_start_end_payload));
        if (spi_bus_manager_submit(mgr, &tr2) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;
    }

    /* SET_RAMY_START_END */
    {
        spi_bus_transaction tr = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_ramy_start_end);

        if (spi_bus_manager_submit(mgr, &tr) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr2 = epd_tx_payload(handle, cs, dc, epd3in7_driver_dma_ramy_start_end_payload, sizeof(epd3in7_driver_dma_ramy_start_end_payload));
        if (spi_bus_manager_submit(mgr, &tr2) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;
    }

    /* SET_RAMX/Y_COUNTER */
    {

        spi_bus_transaction tr = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_ramx_counter);
        if (spi_bus_manager_submit(mgr, &tr) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr2 = epd_tx_payload(handle, cs, dc, &epd3in7_driver_dma_ramx_counter_payload, 1);
        if (spi_bus_manager_submit(mgr, &tr2) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr3 = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_ramy_counter);
        if (spi_bus_manager_submit(mgr, &tr3) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr4 = epd_tx_payload(handle, cs, dc, epd3in7_driver_dma_ramy_counter_payload, 2);
        if (spi_bus_manager_submit(mgr, &tr4) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;
    }

    /* WRITE_RAM + frame data */
    {
        spi_bus_transaction tr_cmd = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_write_ram);
        if (spi_bus_manager_submit(mgr, &tr_cmd) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        const uint16_t image_counter = (uint16_t)(EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 8); // 16800
        spi_bus_transaction tr_data = epd_tx_data(handle, cs, dc, image, image_counter, false);
        if (spi_bus_manager_submit(mgr, &tr_data) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;
    }

    /* Load LUT for the selected mode. */
    {
        epd3in7_driver_lut_type lut_type = epd3in7_driver_mode_to_lut(mode, true);
        epd3in7_driver_status s = epd3in7_enqueue_lut(handle, mgr, cs, dc, lut_type);
        if (s != EPD3IN7_DRIVER_OK)
            return s;
    }

    /* Display update sequence */
    {
        spi_bus_transaction tr = epd_tx_cmd_wait(handle, cs, dc, &epd3in7_driver_dma_display_update, true /* wait BUSY */);
        if (spi_bus_manager_submit(mgr, &tr) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;
    }

    return EPD3IN7_DRIVER_OK;
}

epd3in7_driver_status epd3in7_driver_sleep_dma(epd3in7_driver_handle *handle,
                                               spi_bus_manager *mgr,
                                               const epd3in7_driver_sleep_mode mode)
{
    if (!handle || !mgr)
        return EPD3IN7_DRIVER_ERR_PARAM;

    spi_bus_gpio cs = {handle->pins.cs_port, handle->pins.cs_pin, true};
    spi_bus_gpio dc = {handle->pins.dc_port, handle->pins.dc_pin, false};

    if (mode == EPD3IN7_DRIVER_SLEEP_DEEP)
    {
        uint8_t val = 0x03;
        spi_bus_transaction tr = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_sleep_deep);
        if (spi_bus_manager_submit(mgr, &tr) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr2 = epd_tx_payload(handle, cs, dc, &val, 1);
        if (spi_bus_manager_submit(mgr, &tr2) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;
        return EPD3IN7_DRIVER_OK;
    }
    else
    {
        /* Normal sleep sequence mirrors blocking version: SLEEP(0xF7) -> POWEROFF -> SLEEP2(0xA5) */
        uint8_t v_f7 = 0xF7;
        spi_bus_transaction tr = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_sleep);
        if (spi_bus_manager_submit(mgr, &tr) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr2 = epd_tx_payload(handle, cs, dc, &v_f7, 1);
        if (spi_bus_manager_submit(mgr, &tr2) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr3 = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_power_off);
        if (spi_bus_manager_submit(mgr, &tr3) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        uint8_t v_a5 = 0xA5;
        spi_bus_transaction tr4 = epd_tx_cmd(handle, cs, dc, &epd3in7_driver_dma_sleep2);
        if (spi_bus_manager_submit(mgr, &tr4) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        spi_bus_transaction tr5 = epd_tx_payload(handle, cs, dc, &v_a5, 1);
        if (spi_bus_manager_submit(mgr, &tr5) != SPI_BUS_MANAGER_OK)
            return EPD3IN7_DRIVER_SPI_BUS_ERR;

        return EPD3IN7_DRIVER_OK;
    }
}
