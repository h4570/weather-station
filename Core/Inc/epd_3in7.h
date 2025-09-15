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

// ===============================
// === CONFIGURATION SECTION
// ===============================

// GPIO pins used to interface with the e-Paper
#define EPD_RST_PIN GPIOB, GPIO_PIN_1
#define EPD_DC_PIN GPIOB, GPIO_PIN_0
#define EPD_BUSY_PIN GPIOB, GPIO_PIN_2
#define EPD_MOSI_PIN GPIOB, GPIO_PIN_15
#define EPD_SCLK_PIN GPIOB, GPIO_PIN_13
#define EPD_CS_PIN GPIOB, GPIO_PIN_14

// ===============================
// === STM32 INTEGRATION
// ===============================

// Waveshare data types:
#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

// Waveshare funcs:
#define DEV_Digital_Write(_pin, _value) HAL_GPIO_WritePin(_pin, _value == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET)
#define DEV_Digital_Read(_pin) HAL_GPIO_ReadPin(_pin)
#define DEV_Delay_ms(__xms) HAL_Delay(__xms);

// SPI write func to be implemented in .c file
void DEV_SPI_WriteByte(UBYTE value);

// ===============================
// === DRIVER
// ===============================

// Display resolution
#define EPD_3IN7_WIDTH 280
#define EPD_3IN7_HEIGHT 480

/**
 * Clear screen using GC LUT
 */
void EPD_3IN7_4Gray_Clear(void);

/**
 * Initialize the e-Paper registers for 4-gray level display
 */
void EPD_3IN7_4Gray_Init(void);

/**
 * Send the 4-gray level image buffer to e-Paper and refresh the display
 */
void EPD_3IN7_4Gray_Display(const UBYTE *Image);

/**
 * Clear screen using GC LUT
 * @param mode:
 *   1 = GC; Global clear,
 *   2 = DU; Direct update (partial),
 *   3 = A2; Fast, but worse direct update (partial),
 */
void EPD_3IN7_1Gray_Clear(UBYTE mode);

/**
 * Initialize the e-Paper registers for 1-gray level display
 */
void EPD_3IN7_1Gray_Init(void);

/**
 * Send the 1-gray level image buffer to e-Paper and refresh the display
 *   1 = GC; Global clear,
 *   2 = DU; Direct update (partial),
 *   3 = A2; Fast, but worse direct update (partial),
 */
void EPD_3IN7_1Gray_Display(const UBYTE *Image, UBYTE mode);

/**
 * Send the part of the 1-gray level image buffer to e-Paper and refresh the display
 *
 * Notes from Waveshare:
 * You can send a part of data to e-Paper,But this function is not recommended
 * 1. Xsize must be as big as EPD_3IN7_WIDTH
 * 2. Ypointer must be start at 0
 * 3. You cannot refresh them with the partial refresh mode all the time.
 *    After refreshing partially several times, you need to fully refresh EPD once.
 *    Otherwise, the display effect will be abnormal, which cannot be repaired!
 *
 * Notes from Sandro:
 * 1. Before partial update you need to run at least once at least once full width/height background image in DU mode.
 *    You can use for this any of the DU display funcs: `EPD_3IN7_1Gray_Display()`, `EPD_3IN7_1Gray_Display_Top()`, or `EPD_3IN7_1Gray_Display_Part()`.
 *    Otherwise after partial update the rest of the image will be blank.
 *    After this "initial run", you can run it multiple times.
 * 2. Should'nt be called after Clear() without a full Display() in between.
 * 3. Because of these problems on 3IN7 (thank you Waveshare), consider using simpler alternative `int EPD_3IN7_1Gray_Display_Top()`.
 */
int EPD_3IN7_1Gray_Display_Part(const UBYTE *Image, UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend);

/**
 * Send the top part of the 1-gray level image buffer to e-Paper and refresh the display
 */
int EPD_3IN7_1Gray_Display_Top(const UBYTE *Image, UWORD Yend_exclusive);

/**
 * Enter sleep mode
 *
 * Note: Note that the screen cannot be powered on for a long time.
 * When the screen is not refreshed, please set the screen to sleep mode or power off it.
 * Otherwise, the screen will remain in a high voltage state for a long time, which will damage the e-Paper and cannot be repaired!
 */
void EPD_3IN7_Sleep(void);
