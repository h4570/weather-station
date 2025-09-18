/*****************************************************************************
* | File      	:   epd_3in7.h
* | Author      :   Waveshare team, Sandro
* | Function    :   3.7inch e-paper
* | Info        :
*----------------
* |	This version:   V1.1
* | Date        :   2025-09-15
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
#pragma once

#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_gpio.h"
#include <stdint.h>
#include <stdio.h>

typedef struct
{
    GPIO_TypeDef *reset_port;
    uint16_t reset_pin;

    GPIO_TypeDef *dc_port;
    uint16_t dc_pin;

    GPIO_TypeDef *busy_port;
    uint16_t busy_pin;

    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} epd3in7_pins;

typedef struct
{
    uint16_t width;
    uint16_t height;
    epd3in7_pins pins;
    SPI_HandleTypeDef *spi_handle;
} epd3in7_handle;

typedef enum
{
    EPD_3IN7_MODE_GC = 1,
    EPD_3IN7_MODE_DU = 2,
    EPD_3IN7_MODE_A2 = 3
} epd3in7_mode;

typedef enum
{
    EPD_3IN7_LUT_4_GRAY_GC = 0,
    EPD_3IN7_LUT_1_GRAY_GC = 1,
    EPD_3IN7_LUT_1_GRAY_DU = 2,
    EPD_3IN7_LUT_1_GRAY_A2 = 3
} epd3in7_lut_type;

#define EPD_3IN7_WIDTH 280
#define EPD_3IN7_HEIGHT 480
#define EPD_SPI_TIMEOUT 1000

/**
 * Create the e-Paper handle with given pin configuration
 */
epd3in7_handle epd3in7_create(const epd3in7_pins pins, const SPI_HandleTypeDef *spi_handle);

/**
 * Clear screen using GC LUT
 */
void epd3in7_clear_4_gray(const epd3in7_handle *handle);

/**
 * Initialize the e-Paper registers for 4-gray level display
 */
void epd3in7_init_4_gray(const epd3in7_handle *handle);

/**
 * Send the 4-gray level image buffer to e-Paper and refresh the display
 */
void epd3in7_display_4_gray(const epd3in7_handle *handle, const uint8_t *image);

/**
 * Clear screen using GC LUT
 * @param mode:
 *   1 = GC; Global clear,
 *   2 = DU; Direct update (partial),
 *   3 = A2; Fast, but worse direct update (partial),
 */
void epd3in7_clear_1_gray(const epd3in7_handle *handle, const epd3in7_mode mode);

/**
 * Initialize the e-Paper registers for 1-gray level display
 */
void epd3in7_init_1_gray(const epd3in7_handle *handle);

/**
 * Send the 1-gray level image buffer to e-Paper and refresh the display
 *   1 = GC; Global clear,
 *   2 = DU; Direct update (partial),
 *   3 = A2; Fast, but worse direct update (partial),
 */
void epd3in7_display_1_gray(const epd3in7_handle *handle, const uint8_t *image, const epd3in7_mode mode);

/**
 * Send the top part of the 1-gray level image buffer to e-Paper and refresh the display
 *
 * Notes:
 * - You cannot refresh them with the partial refresh mode all the time.
 *   After refreshing partially several times, you need to fully refresh EPD once.
 *   Otherwise, the display effect will be abnormal, which cannot be repaired!
 *
 * - Before partial update you need to run at least once at least once full width/height background image in DU mode.
 *   You can use for this any of the DU display funcs: `epd3in7_display_1_gray()`, `epd3in7_display_1_gray_top()`.
 *   Otherwise after partial update the rest of the image will be blank.
 *   After this "initial run", you can run it multiple times.
 *
 * - Should'nt be called after Clear() without a full Display() in between.
 */
int epd3in7_display_1_gray_top(const epd3in7_handle *handle, const uint8_t *image, const uint16_t y_end_exclusive);

/**
 * Enter sleep mode
 *
 * Note: Note that the screen cannot be powered on for a long time.
 * When the screen is not refreshed, please set the screen to sleep mode or power off it.
 * Otherwise, the screen will remain in a high voltage state for a long time, which will damage the e-Paper and cannot be repaired!
 */
void epd3in7_sleep(const epd3in7_handle *handle);
