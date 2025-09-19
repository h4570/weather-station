/*****************************************************************************
* | File      	:   epd_3in7.c
* | Author      :   h4570 (based on Waveshare code)
* | Function    :   3.7inch e-paper
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-09-18
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#include "epd3in7.h"
#include "string.h"

#define EPD3IN7_TRY(expr)           \
    do                              \
    {                               \
        epd3in7_status _s = (expr); \
        if (_s != EPD3IN7_OK)       \
        {                           \
            err = _s;               \
            goto fail;              \
        }                           \
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
} epd3in7_cmd;

static const uint8_t epd3in7_lut_4_gray_gc[] =
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

static const uint8_t epd3in7_lut_1_gray_gc[] =
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

static const uint8_t epd3in7_lut_1_gray_du[] =
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

static const uint8_t epd3in7_lut_1_gray_a2[] =
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

static void epd3in7_reset(const epd3in7_handle *handle)
{
    HAL_GPIO_WritePin(handle->pins.reset_port, handle->pins.reset_pin, GPIO_PIN_SET);
    HAL_Delay(300);
    HAL_GPIO_WritePin(handle->pins.reset_port, handle->pins.reset_pin, GPIO_PIN_RESET);
    HAL_Delay(3);
    HAL_GPIO_WritePin(handle->pins.reset_port, handle->pins.reset_pin, GPIO_PIN_SET);
    HAL_Delay(300);
}

static void epd3in7_send_begin(epd3in7_handle *handle)
{
    HAL_GPIO_WritePin(handle->pins.cs_port, handle->pins.cs_pin, GPIO_PIN_RESET);
    handle->is_cs_low = true;
}

static void epd3in7_send_end(epd3in7_handle *handle)
{
    HAL_GPIO_WritePin(handle->pins.cs_port, handle->pins.cs_pin, GPIO_PIN_SET);
    handle->is_cs_low = false;
}

static epd3in7_status epd3in7_send_command(const epd3in7_handle *handle, const epd3in7_cmd reg)
{
    HAL_GPIO_WritePin(handle->pins.dc_port, handle->pins.dc_pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(handle->spi_handle, (uint8_t *)&reg, 1, EPD3IN7_SPI_TIMEOUT) != HAL_OK)
        return EPD3IN7_ERR_HAL;
    return EPD3IN7_OK;
}

static epd3in7_status epd3in7_send_data(const epd3in7_handle *handle, const uint8_t data)
{
    HAL_GPIO_WritePin(handle->pins.dc_port, handle->pins.dc_pin, GPIO_PIN_SET);
    if (HAL_SPI_Transmit(handle->spi_handle, &data, 1, EPD3IN7_SPI_TIMEOUT) != HAL_OK)
        return EPD3IN7_ERR_HAL;
    return EPD3IN7_OK;
}

static epd3in7_status epd3in7_send_data_many(const epd3in7_handle *handle, const uint8_t *data, uint32_t len)
{
    HAL_GPIO_WritePin(handle->pins.dc_port, handle->pins.dc_pin, GPIO_PIN_SET);
    if (HAL_SPI_Transmit(handle->spi_handle, (uint8_t *)data, len, EPD3IN7_SPI_TIMEOUT) != HAL_OK)
        return EPD3IN7_ERR_HAL;
    return EPD3IN7_OK;
}

static epd3in7_status epd3in7_busy_wait_for_idle(const epd3in7_handle *handle)
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
        if ((HAL_GetTick() - start) > EPD3IN7_BUSY_TIMEOUT)
        {
            return EPD3IN7_ERR_TIMEOUT;
        }
    } while (handle->busy_active_high ? busy == GPIO_PIN_SET : busy == GPIO_PIN_RESET);

    HAL_Delay(200);
    return EPD3IN7_OK;
}

static epd3in7_lut_type epd3in7_mode_to_lut(const epd3in7_mode mode, const bool is_1_color)
{
    if (is_1_color)
    {
        if (mode == EPD3IN7_MODE_GC)
        {
            return EPD3IN7_LUT_1_GRAY_GC;
        }
        else if (mode == EPD3IN7_MODE_DU)
        {
            return EPD3IN7_LUT_1_GRAY_DU;
        }
        else if (mode == EPD3IN7_MODE_A2)
        {
            return EPD3IN7_LUT_1_GRAY_A2;
        }
    }
    else
    {
        return EPD3IN7_LUT_4_GRAY_GC;
    }

    return EPD3IN7_LUT_1_GRAY_GC;
}

/**
 * REQUIRES: CS pin to be low
 */
static epd3in7_status epd3in7_load_lut(epd3in7_handle *handle, const epd3in7_lut_type lut)
{
    if (handle->was_lut_sent && handle->last_lut == lut)
    {
        return EPD3IN7_OK;
    }

    if (handle->is_cs_low == false)
    {
        return EPD3IN7_ERR_PARAM;
    }

    epd3in7_status err = EPD3IN7_OK;

    epd3in7_status res = epd3in7_send_command(handle, EPD_CMD_WRITE_LUT_REGISTER);

    if (res != EPD3IN7_OK)
    {
        return res;
    }

    const uint8_t *lut_ptr = NULL;

    if (lut == EPD3IN7_LUT_4_GRAY_GC)
    {
        lut_ptr = epd3in7_lut_4_gray_gc;
    }
    else if (lut == EPD3IN7_LUT_1_GRAY_GC)
    {
        lut_ptr = epd3in7_lut_1_gray_gc;
    }
    else if (lut == EPD3IN7_LUT_1_GRAY_DU)
    {
        lut_ptr = epd3in7_lut_1_gray_du;
    }
    else if (lut == EPD3IN7_LUT_1_GRAY_A2)
    {
        lut_ptr = epd3in7_lut_1_gray_a2;
    }

    if (lut_ptr)
    {
        res = epd3in7_send_data_many(handle, lut_ptr, 105);

        if (res != EPD3IN7_OK)
        {
            return res;
        }
    }

    handle->was_lut_sent = true;
    handle->last_lut = lut;

    return err;
}

epd3in7_handle epd3in7_create(const epd3in7_pins pins, SPI_HandleTypeDef *spi_handle, const bool busy_active_high)
{
    epd3in7_handle handle;
    handle.width = EPD3IN7_WIDTH;
    handle.height = EPD3IN7_HEIGHT;
    handle.pins = pins;
    handle.spi_handle = spi_handle;
    handle.was_lut_sent = false;
    handle.last_lut = EPD3IN7_LUT_4_GRAY_GC;
    handle.busy_active_high = busy_active_high;
    return handle;
}

epd3in7_status epd3in7_init_4_gray(epd3in7_handle *handle)
{
    epd3in7_status err = EPD3IN7_OK;

    epd3in7_reset(handle);

    epd3in7_send_begin(handle);

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SW_RESET));

    HAL_Delay(300);

    if (epd3in7_send_command(handle, EPD_CMD_AUTO_WRITE_RED_RAM_REG_PATTERN) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_data(handle, 0xF7) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_busy_wait_for_idle(handle) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_command(handle, EPD_CMD_AUTO_WRITE_BW_RAM_REG_PATTERN) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_data(handle, 0xF7) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_busy_wait_for_idle(handle) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_GATE_SETTING) != EPD3IN7_OK)
        goto fail;
    uint8_t gate_setting[] = {0xDF, 0x01, 0x00};
    if (epd3in7_send_data_many(handle, gate_setting, 3) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_GATE_VOLTAGE) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_data(handle, 0x00) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_GATE_VOLTAGE_SOURCE) != EPD3IN7_OK)
        goto fail;
    uint8_t gate_voltage_source[] = {0x41, 0xA8, 0x32};
    if (epd3in7_send_data_many(handle, gate_voltage_source, 3) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_DATA_ENTRY_SEQUENCE) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_data(handle, 0x03) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_BORDER_WAVEFORM_CONTROL) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_data(handle, 0x03) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_BOOSTER_SOFT_START_CONTROL) != EPD3IN7_OK)
        goto fail;
    uint8_t booster[] = {0xAE, 0xC7, 0xC3, 0xC0, 0xC0};
    if (epd3in7_send_data_many(handle, booster, 5) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_TEMPERATURE_SENSOR_SELECTION) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_data(handle, 0x80) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_WRITE_VCOM_REGISTER) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_data(handle, 0x44) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_DISPLAY_OPTION) != EPD3IN7_OK)
        goto fail;
    uint8_t display_option[10] = {0};
    if (epd3in7_send_data_many(handle, display_option, 10) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_SET_RAMX_START_END) != EPD3IN7_OK)
        goto fail;
    uint8_t ramx[] = {0x00, 0x00, 0x17, 0x01};
    if (epd3in7_send_data_many(handle, ramx, 4) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_SET_RAMY_START_END) != EPD3IN7_OK)
        goto fail;
    uint8_t ramy[] = {0x00, 0x00, 0xDF, 0x01};
    if (epd3in7_send_data_many(handle, ramy, 4) != EPD3IN7_OK)
        goto fail;

    if (epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING) != EPD3IN7_OK)
        goto fail;
    if (epd3in7_send_data(handle, 0xCF) != EPD3IN7_OK)
        goto fail;

    epd3in7_send_end(handle);

    handle->was_lut_sent = false;
    return EPD3IN7_OK;
fail:
    epd3in7_send_end(handle);
    return err;
}

epd3in7_status epd3in7_init_1_gray(epd3in7_handle *handle)
{
    epd3in7_status err = EPD3IN7_OK;

    epd3in7_reset(handle);

    epd3in7_send_begin(handle);

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SW_RESET));
    HAL_Delay(300);

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_AUTO_WRITE_RED_RAM_REG_PATTERN));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0xF7));
    EPD3IN7_TRY(epd3in7_busy_wait_for_idle(handle));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_AUTO_WRITE_BW_RAM_REG_PATTERN));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0xF7));
    EPD3IN7_TRY(epd3in7_busy_wait_for_idle(handle));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_GATE_SETTING));
    uint8_t gate_setting[] = {0xDF, 0x01, 0x00};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, gate_setting, 3));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_GATE_VOLTAGE));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_GATE_VOLTAGE_SOURCE));
    uint8_t gate_voltage_source[] = {0x41, 0xA8, 0x32};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, gate_voltage_source, 3));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DATA_ENTRY_SEQUENCE));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x03));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_BORDER_WAVEFORM_CONTROL));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x03));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_BOOSTER_SOFT_START_CONTROL));
    uint8_t booster[] = {0xAE, 0xC7, 0xC3, 0xC0, 0xC0};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, booster, 5));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_TEMPERATURE_SENSOR_SELECTION));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x80));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_WRITE_VCOM_REGISTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x44));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_OPTION));
    uint8_t display_option[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0xFF, 0xFF};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, display_option, 10));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_START_END));
    uint8_t ramx[] = {0x00, 0x00, 0x17, 0x01};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, ramx, 4));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_START_END));
    uint8_t ramy[] = {0x00, 0x00, 0xDF, 0x01};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, ramy, 4));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0xCF));

    epd3in7_send_end(handle);

    handle->was_lut_sent = false;
    return EPD3IN7_OK;
fail:
    epd3in7_send_end(handle);
    return err;
}

epd3in7_status epd3in7_clear_4_gray(epd3in7_handle *handle)
{
    epd3in7_status err = EPD3IN7_OK;
    uint16_t width = (EPD3IN7_WIDTH % 8 == 0) ? (EPD3IN7_WIDTH / 8) : (EPD3IN7_WIDTH / 8 + 1);
    uint16_t height = EPD3IN7_HEIGHT;

    epd3in7_send_begin(handle);

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_UNKNOWN_0x49));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_WRITE_RAM));

    uint32_t total = width * height;
    uint8_t ff_buf[32];
    memset(ff_buf, 0xFF, sizeof(ff_buf));
    uint32_t left = total;
    while (left > 0)
    {
        uint32_t chunk = left > sizeof(ff_buf) ? sizeof(ff_buf) : left;
        EPD3IN7_TRY(epd3in7_send_data_many(handle, ff_buf, chunk));
        left -= chunk;
    }

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_WRITE_RAM2));

    left = total;
    while (left > 0)
    {
        uint32_t chunk = left > sizeof(ff_buf) ? sizeof(ff_buf) : left;
        EPD3IN7_TRY(epd3in7_send_data_many(handle, ff_buf, chunk));
        left -= chunk;
    }

    EPD3IN7_TRY(epd3in7_load_lut(handle, EPD3IN7_LUT_4_GRAY_GC));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0xC7));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_send_end(handle);

    EPD3IN7_TRY(epd3in7_busy_wait_for_idle(handle));
    return EPD3IN7_OK;

fail:
    epd3in7_send_end(handle);
    return err;
}

epd3in7_status epd3in7_clear_1_gray(epd3in7_handle *handle, const epd3in7_mode mode)
{
    epd3in7_status err = EPD3IN7_OK;
    const uint16_t image_counter = EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 8;

    epd3in7_send_begin(handle);

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_WRITE_RAM));

    uint8_t ff_buf[32];
    memset(ff_buf, 0xFF, sizeof(ff_buf));
    uint32_t left = image_counter;
    while (left > 0)
    {
        uint32_t chunk = left > sizeof(ff_buf) ? sizeof(ff_buf) : left;
        EPD3IN7_TRY(epd3in7_send_data_many(handle, ff_buf, chunk));
        left -= chunk;
    }

    epd3in7_lut_type lut_type = epd3in7_mode_to_lut(mode, true);
    EPD3IN7_TRY(epd3in7_load_lut(handle, lut_type));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_send_end(handle);

    EPD3IN7_TRY(epd3in7_busy_wait_for_idle(handle));
    return EPD3IN7_OK;

fail:
    epd3in7_send_end(handle);
    return err;
}

epd3in7_status epd3in7_display_4_gray(epd3in7_handle *handle, const uint8_t *image)
{
    epd3in7_status err = EPD3IN7_OK;
    const uint16_t image_counter = EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 8;

    epd3in7_send_begin(handle);

    EPD3IN7_TRY(epd3in7_send_command(handle, 0x49));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_WRITE_RAM));

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
        EPD3IN7_TRY(epd3in7_send_data(handle, temp3));
    }

    // new  data
    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_WRITE_RAM2));

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
        EPD3IN7_TRY(epd3in7_send_data(handle, temp3));
    }

    EPD3IN7_TRY(epd3in7_load_lut(handle, EPD3IN7_LUT_4_GRAY_GC));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0xC7));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_send_end(handle);

    EPD3IN7_TRY(epd3in7_busy_wait_for_idle(handle));
    return EPD3IN7_OK;
fail:
    epd3in7_send_end(handle);
    return err;
}

epd3in7_status epd3in7_display_1_gray(epd3in7_handle *handle, const uint8_t *image, const epd3in7_mode mode)
{
    epd3in7_status err = EPD3IN7_OK;
    epd3in7_send_begin(handle);

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_START_END));
    uint8_t ramx[] = {0x00, 0x00, 0x17, 0x01};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, ramx, 4));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_START_END));
    uint8_t ramy[] = {0x00, 0x00, 0xDF, 0x01};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, ramy, 4));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x00));

    const uint16_t image_counter = EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 8;

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_WRITE_RAM));
    EPD3IN7_TRY(epd3in7_send_data_many(handle, image, image_counter));

    epd3in7_lut_type lut_type = epd3in7_mode_to_lut(mode, true);
    EPD3IN7_TRY(epd3in7_load_lut(handle, lut_type));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_send_end(handle);

    EPD3IN7_TRY(epd3in7_busy_wait_for_idle(handle));
    return EPD3IN7_OK;
fail:
    epd3in7_send_end(handle);
    return err;
}

epd3in7_status epd3in7_display_1_gray_top(epd3in7_handle *handle, const uint8_t *image, const uint16_t y_end_exclusive, const epd3in7_mode mode)
{
    epd3in7_status err = EPD3IN7_OK;
    uint16_t x_start = 0;
    uint16_t y_start = 0;
    uint16_t x_end = EPD3IN7_WIDTH;

    if (image == NULL)
    {
        return EPD3IN7_ERR_PARAM; // null pointer
    }
    else if (y_end_exclusive == 0 || y_end_exclusive > EPD3IN7_HEIGHT)
    {
        return EPD3IN7_ERR_PARAM; // invalid height
    }

    uint16_t y_end_exclusive_temp = y_end_exclusive;

    x_end -= 1;
    y_end_exclusive_temp -= 1;

    epd3in7_send_begin(handle);

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_START_END));
    uint8_t ramx[4] = {
        x_start & 0xff,
        (x_start >> 8) & 0x03,
        x_end & 0xff,
        (x_end >> 8) & 0x03};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, ramx, 4));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_START_END));
    uint8_t ramy[4] = {
        y_start & 0xff,
        (y_start >> 8) & 0x03,
        y_end_exclusive_temp & 0xff,
        (y_end_exclusive_temp >> 8) & 0x03};
    EPD3IN7_TRY(epd3in7_send_data_many(handle, ramy, 4));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER)); // SET_RAM_X_ADDRESS_COUNTER
    EPD3IN7_TRY(epd3in7_send_data(handle, x_start & 0xFF));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER)); // SET_RAM_Y_ADDRESS_COUNTER
    EPD3IN7_TRY(epd3in7_send_data(handle, y_start & 0xFF));
    EPD3IN7_TRY(epd3in7_send_data(handle, (y_start >> 8) & 0xFF));

    uint16_t width = (x_end - x_start) % 8 == 0 ? (x_end - x_start) / 8 : (x_end - x_start) / 8 + 1;
    uint16_t image_counter = width * (y_end_exclusive - y_start);

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_WRITE_RAM));

    EPD3IN7_TRY(epd3in7_send_data_many(handle, image, image_counter));

    epd3in7_lut_type lut_type = epd3in7_mode_to_lut(mode, true);
    EPD3IN7_TRY(epd3in7_load_lut(handle, lut_type));

    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE));
    epd3in7_send_end(handle);

    EPD3IN7_TRY(epd3in7_busy_wait_for_idle(handle));

    return 0;

fail:
    epd3in7_send_end(handle);
    return err;
}

epd3in7_status epd3in7_sleep(const epd3in7_handle *handle)
{
    epd3in7_status err = EPD3IN7_OK;

    epd3in7_send_begin(handle);
    EPD3IN7_TRY(epd3in7_send_command(handle, EPD_CMD_DEEP_SLEEP)); // deep sleep
    EPD3IN7_TRY(epd3in7_send_data(handle, 0x03));
    epd3in7_send_end(handle);

    return EPD3IN7_OK;

fail:
    epd3in7_send_end(handle);
    return err;
}