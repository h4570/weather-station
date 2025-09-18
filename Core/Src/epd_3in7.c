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

#include "epd_3in7.h"

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
    EPD_CMD_SLEEP = 0x50
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

static void epd3in7_send_command(const epd3in7_handle *handle, const epd3in7_cmd reg)
{
    HAL_GPIO_WritePin(handle->pins.dc_port, handle->pins.dc_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(handle->pins.cs_port, handle->pins.cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle->spi_handle, (uint8_t *)&reg, 1, EPD_SPI_TIMEOUT);
    HAL_GPIO_WritePin(handle->pins.cs_port, handle->pins.cs_pin, GPIO_PIN_SET);
}

static void epd3in7_send_data(const epd3in7_handle *handle, const uint8_t data)
{
    HAL_GPIO_WritePin(handle->pins.dc_port, handle->pins.dc_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(handle->pins.cs_port, handle->pins.cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(handle->spi_handle, &data, 1, EPD_SPI_TIMEOUT);
    HAL_GPIO_WritePin(handle->pins.cs_port, handle->pins.cs_pin, GPIO_PIN_SET);
}

static void epd3in7_busy_wait_for_low(const epd3in7_handle *handle)
{
    uint8_t busy;

    do
    {
        busy = HAL_GPIO_ReadPin(handle->pins.busy_port, handle->pins.busy_pin);
    } while (busy);

    HAL_Delay(200);
}

static epd3in7_lut_type epd3in7_mode_to_lut(const epd3in7_mode mode, const uint8_t is_1_color)
{
    if (is_1_color)
    {
        if (mode == EPD_3IN7_MODE_GC)
        {
            return EPD_3IN7_LUT_1_GRAY_GC;
        }
        else if (mode == EPD_3IN7_MODE_DU)
        {
            return EPD_3IN7_LUT_1_GRAY_DU;
        }
        else if (mode == EPD_3IN7_MODE_A2)
        {
            return EPD_3IN7_LUT_1_GRAY_A2;
        }
    }
    else
    {
        return EPD_3IN7_LUT_4_GRAY_GC;
    }

    return EPD_3IN7_LUT_1_GRAY_GC;
}

epd3in7_handle epd3in7_create(const epd3in7_pins pins, const SPI_HandleTypeDef *spi_handle)
{
    epd3in7_handle handle;
    handle.width = EPD_3IN7_WIDTH;
    handle.height = EPD_3IN7_HEIGHT;
    handle.pins = pins;
    handle.spi_handle = spi_handle;
    return handle;
}

void epd3in7_load_lut(const epd3in7_handle *handle, const epd3in7_lut_type lut)
{
    epd3in7_send_command(handle, EPD_CMD_WRITE_LUT_REGISTER);

    for (uint16_t i = 0; i < 105; i++)
    {
        if (lut == EPD_3IN7_LUT_4_GRAY_GC)
        {
            epd3in7_send_data(handle, epd3in7_lut_4_gray_gc[i]);
        }
        else if (lut == EPD_3IN7_LUT_1_GRAY_GC)
        {
            epd3in7_send_data(handle, epd3in7_lut_1_gray_gc[i]);
        }
        else if (lut == EPD_3IN7_LUT_1_GRAY_DU)
        {
            epd3in7_send_data(handle, epd3in7_lut_1_gray_du[i]);
        }
        else if (lut == EPD_3IN7_LUT_1_GRAY_A2)
        {
            epd3in7_send_data(handle, epd3in7_lut_1_gray_a2[i]);
        }
    }
}

void epd3in7_init_4_gray(const epd3in7_handle *handle)
{
    epd3in7_reset(handle);

    epd3in7_send_command(handle, EPD_CMD_SW_RESET);
    HAL_Delay(300);

    epd3in7_send_command(handle, EPD_CMD_AUTO_WRITE_RED_RAM_REG_PATTERN);
    epd3in7_send_data(handle, 0xF7);
    epd3in7_busy_wait_for_low(handle);
    epd3in7_send_command(handle, EPD_CMD_AUTO_WRITE_BW_RAM_REG_PATTERN);
    epd3in7_send_data(handle, 0xF7);
    epd3in7_busy_wait_for_low(handle);

    epd3in7_send_command(handle, EPD_CMD_GATE_SETTING); // setting gaet number
    epd3in7_send_data(handle, 0xDF);
    epd3in7_send_data(handle, 0x01);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_GATE_VOLTAGE); // set gate voltage
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_GATE_VOLTAGE_SOURCE); // set source voltage
    epd3in7_send_data(handle, 0x41);
    epd3in7_send_data(handle, 0xA8);
    epd3in7_send_data(handle, 0x32);

    epd3in7_send_command(handle, EPD_CMD_DATA_ENTRY_SEQUENCE); // set data entry sequence
    epd3in7_send_data(handle, 0x03);

    epd3in7_send_command(handle, EPD_CMD_BORDER_WAVEFORM_CONTROL); // set border
    epd3in7_send_data(handle, 0x03);

    epd3in7_send_command(handle, EPD_CMD_BOOSTER_SOFT_START_CONTROL); // set booster strength
    epd3in7_send_data(handle, 0xAE);
    epd3in7_send_data(handle, 0xC7);
    epd3in7_send_data(handle, 0xC3);
    epd3in7_send_data(handle, 0xC0);
    epd3in7_send_data(handle, 0xC0);

    epd3in7_send_command(handle, EPD_CMD_TEMPERATURE_SENSOR_SELECTION); // set internal sensor on
    epd3in7_send_data(handle, 0x80);

    epd3in7_send_command(handle, EPD_CMD_WRITE_VCOM_REGISTER); // set vcom value
    epd3in7_send_data(handle, 0x44);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_OPTION); // set display option, these setting turn on previous function
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_START_END); // setting X direction start/end position of RAM
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x17);
    epd3in7_send_data(handle, 0x01);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_START_END); // setting Y direction start/end position of RAM
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0xDF);
    epd3in7_send_data(handle, 0x01);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING); // Display Update Control 2
    epd3in7_send_data(handle, 0xCF);
}

void epd3in7_init_1_gray(const epd3in7_handle *handle)
{
    epd3in7_reset(handle);

    epd3in7_send_command(handle, EPD_CMD_SW_RESET);
    HAL_Delay(300);

    epd3in7_send_command(handle, EPD_CMD_AUTO_WRITE_RED_RAM_REG_PATTERN);
    epd3in7_send_data(handle, 0xF7);
    epd3in7_busy_wait_for_low(handle);

    epd3in7_send_command(handle, EPD_CMD_AUTO_WRITE_BW_RAM_REG_PATTERN);
    epd3in7_send_data(handle, 0xF7);
    epd3in7_busy_wait_for_low(handle);

    epd3in7_send_command(handle, EPD_CMD_GATE_SETTING); // setting gaet number
    epd3in7_send_data(handle, 0xDF);
    epd3in7_send_data(handle, 0x01);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_GATE_VOLTAGE); // set gate voltage
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_GATE_VOLTAGE_SOURCE); // set source voltage
    epd3in7_send_data(handle, 0x41);
    epd3in7_send_data(handle, 0xA8);
    epd3in7_send_data(handle, 0x32);

    epd3in7_send_command(handle, EPD_CMD_DATA_ENTRY_SEQUENCE); // set data entry sequence
    epd3in7_send_data(handle, 0x03);

    epd3in7_send_command(handle, EPD_CMD_BORDER_WAVEFORM_CONTROL); // set border
    epd3in7_send_data(handle, 0x03);

    epd3in7_send_command(handle, EPD_CMD_BOOSTER_SOFT_START_CONTROL); // set booster strength
    epd3in7_send_data(handle, 0xAE);
    epd3in7_send_data(handle, 0xC7);
    epd3in7_send_data(handle, 0xC3);
    epd3in7_send_data(handle, 0xC0);
    epd3in7_send_data(handle, 0xC0);

    epd3in7_send_command(handle, EPD_CMD_TEMPERATURE_SENSOR_SELECTION); // set internal sensor on
    epd3in7_send_data(handle, 0x80);

    epd3in7_send_command(handle, EPD_CMD_WRITE_VCOM_REGISTER); // set vcom value
    epd3in7_send_data(handle, 0x44);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_OPTION); // set display option, these setting turn on previous function
    epd3in7_send_data(handle, 0x00);                      // can switch 1 gray or 4 gray
    epd3in7_send_data(handle, 0xFF);
    epd3in7_send_data(handle, 0xFF);
    epd3in7_send_data(handle, 0xFF);
    epd3in7_send_data(handle, 0xFF);
    epd3in7_send_data(handle, 0x4F);
    epd3in7_send_data(handle, 0xFF);
    epd3in7_send_data(handle, 0xFF);
    epd3in7_send_data(handle, 0xFF);
    epd3in7_send_data(handle, 0xFF);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_START_END); // setting X direction start/end position of RAM
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x17);
    epd3in7_send_data(handle, 0x01);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_START_END); // setting Y direction start/end position of RAM
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0xDF);
    epd3in7_send_data(handle, 0x01);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING); // Display Update Control 2
    epd3in7_send_data(handle, 0xCF);
}

void epd3in7_clear_4_gray(const epd3in7_handle *handle)
{
    uint16_t width = (EPD_3IN7_WIDTH % 8 == 0) ? (EPD_3IN7_WIDTH / 8) : (EPD_3IN7_WIDTH / 8 + 1);
    uint16_t height = EPD_3IN7_HEIGHT;

    epd3in7_send_command(handle, 0x49);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_WRITE_RAM);

    for (uint16_t j = 0; j < height; j++)
    {
        for (uint16_t i = 0; i < width; i++)
        {
            epd3in7_send_data(handle, 0xff);
        }
    }

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_WRITE_RAM2);

    for (uint16_t j = 0; j < height; j++)
    {
        for (uint16_t i = 0; i < width; i++)
        {
            epd3in7_send_data(handle, 0xff);
        }
    }

    epd3in7_load_lut(handle, EPD_3IN7_LUT_4_GRAY_GC);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING);
    epd3in7_send_data(handle, 0xC7);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE);
    epd3in7_busy_wait_for_low(handle);
}

void epd3in7_clear_1_gray(const epd3in7_handle *handle, const epd3in7_mode mode)
{
    const uint16_t image_counter = EPD_3IN7_WIDTH * EPD_3IN7_HEIGHT / 8;

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_WRITE_RAM);

    for (uint16_t i = 0; i < image_counter; i++)
    {
        epd3in7_send_data(handle, 0xff);
    }

    epd3in7_lut_type lut_type = epd3in7_mode_to_lut(mode, 1);
    epd3in7_load_lut(handle, lut_type);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE);
    epd3in7_busy_wait_for_low(handle);
}

void epd3in7_display_4_gray(const epd3in7_handle *handle, const uint8_t *image)
{
    epd3in7_send_command(handle, 0x49);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_WRITE_RAM);

    for (uint32_t i = 0; i < 16800; i++)
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

        epd3in7_send_data(handle, temp3);
    }

    // new  data
    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_WRITE_RAM2);

    for (uint32_t i = 0; i < 16800; i++)
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

        epd3in7_send_data(handle, temp3);
    }

    epd3in7_load_lut(handle, EPD_3IN7_LUT_4_GRAY_GC);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE_SETTING);
    epd3in7_send_data(handle, 0xC7);

    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE);

    epd3in7_busy_wait_for_low(handle);
}

void epd3in7_display_1_gray(const epd3in7_handle *handle, const uint8_t *image, const epd3in7_mode mode)
{
    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_START_END);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x17);
    epd3in7_send_data(handle, 0x01);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_START_END);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0xDF);
    epd3in7_send_data(handle, 0x01);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER);
    epd3in7_send_data(handle, 0x00);
    epd3in7_send_data(handle, 0x00);

    const uint16_t image_counter = EPD_3IN7_WIDTH * EPD_3IN7_HEIGHT / 8;

    epd3in7_send_command(handle, EPD_CMD_WRITE_RAM);

    for (uint16_t i = 0; i < image_counter; i++)
    {
        epd3in7_send_data(handle, image[i]);
    }

    epd3in7_lut_type lut_type = epd3in7_mode_to_lut(mode, 1);
    epd3in7_load_lut(handle, lut_type);
    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE);
    epd3in7_busy_wait_for_low(handle);
}

int epd3in7_display_1_gray_top(const epd3in7_handle *handle, const uint8_t *image, const uint16_t y_end_exclusive)
{
    uint16_t x_start = 0;
    uint16_t y_start = 0;
    uint16_t x_end = EPD_3IN7_WIDTH;

    if (image == NULL)
    {
        return -1; // null pointer
    }
    else if (y_end_exclusive == 0 || y_end_exclusive > EPD_3IN7_HEIGHT)
    {
        return -2; // invalid height
    }

    uint16_t y_end_exclusive_temp = y_end_exclusive;

    x_end -= 1;
    y_end_exclusive_temp -= 1;

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_START_END);
    epd3in7_send_data(handle, x_start & 0xff);
    epd3in7_send_data(handle, (x_start >> 8) & 0x03);
    epd3in7_send_data(handle, x_end & 0xff);
    epd3in7_send_data(handle, (x_end >> 8) & 0x03);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_START_END);
    epd3in7_send_data(handle, y_start & 0xff);
    epd3in7_send_data(handle, (y_start >> 8) & 0x03);
    epd3in7_send_data(handle, y_end_exclusive_temp & 0xff);
    epd3in7_send_data(handle, (y_end_exclusive_temp >> 8) & 0x03);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMX_COUNTER); // SET_RAM_X_ADDRESS_COUNTER
    epd3in7_send_data(handle, x_start & 0xFF);

    epd3in7_send_command(handle, EPD_CMD_SET_RAMY_COUNTER); // SET_RAM_Y_ADDRESS_COUNTER
    epd3in7_send_data(handle, y_start & 0xFF);
    epd3in7_send_data(handle, (y_start >> 8) & 0xFF);

    uint16_t width = (x_end - x_start) % 8 == 0 ? (x_end - x_start) / 8 : (x_end - x_start) / 8 + 1;
    uint16_t image_counter = width * (y_end_exclusive - y_start);

    epd3in7_send_command(handle, EPD_CMD_WRITE_RAM);

    for (uint16_t i = 0; i < image_counter; i++)
    {
        epd3in7_send_data(handle, image[i]);
    }

    epd3in7_load_lut(handle, EPD_3IN7_LUT_1_GRAY_A2);
    epd3in7_send_command(handle, EPD_CMD_DISPLAY_UPDATE_SEQUENCE);
    epd3in7_busy_wait_for_low(handle);

    return 0;
}

void epd3in7_sleep(const epd3in7_handle *handle)
{
    epd3in7_send_command(handle, EPD_CMD_DEEP_SLEEP); // deep sleep
    epd3in7_send_data(handle, 0x03);
}
