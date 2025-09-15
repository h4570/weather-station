#pragma once
#include "stm32g4xx_hal.h"
#include <stdint.h>

extern SPI_HandleTypeDef hspi2;

// Wymiary jak w Twoim kodzie Rust:
#define EPD_W 280u
#define EPD_H 480u

// --- Piny (dopasuj do CubeMX / okablowania) ---
#define EPD_DC_GPIO_Port GPIOB
#define EPD_DC_Pin GPIO_PIN_0
#define EPD_RST_GPIO_Port GPIOB
#define EPD_RST_Pin GPIO_PIN_1
#define EPD_BUSY_GPIO_Port GPIOB
#define EPD_BUSY_Pin GPIO_PIN_2
#define EPD_CS_GPIO_Port GPIOB
#define EPD_CS_Pin GPIO_PIN_14

// Jeśli BUSY==LOW gdy ZAJĘTY -> ustaw 1; jeśli BUSY==HIGH gdy ZAJĘTY -> 0
// W Twoim kodzie Rust było IS_BUSY_LOW=false; zacznij od 0 i zmień, jeśli wisi.
#define EPD_BUSY_ACTIVE_LOW 0

// --- Kody komend (jak w Twoim enumie) ---
#define CMD_GATE_SETTING 0x01
#define CMD_POWEROFF 0x02
#define CMD_SLEEP2 0x07
#define CMD_GATE_VOLTAGE 0x03
#define CMD_GATE_VOLTAGE_SOURCE 0x04
#define CMD_BOOSTER_SOFT_START_CONTROL 0x0C
#define CMD_DEEP_SLEEP 0x10
#define CMD_DATA_ENTRY_SEQUENCE 0x11
#define CMD_SW_RESET 0x12
#define CMD_TEMPERATURE_SENSOR_SELECTION 0x18
#define CMD_TEMPERATURE_SENSOR_WRITE 0x1A
#define CMD_TEMPERATURE_SENSOR_READ 0x1B
#define CMD_DISPLAY_UPDATE_SEQUENCE 0x20
#define CMD_DISPLAY_UPDATE_SEQUENCE_SETTING 0x22
#define CMD_WRITE_RAM 0x24
#define CMD_WRITE_RAM2 0x26
#define CMD_WRITE_VCOM_REGISTER 0x2C
#define CMD_WRITE_LUT_REGISTER 0x32
#define CMD_DISPLAY_OPTION 0x37
#define CMD_BORDER_WAVEFORM_CONTROL 0x3C
#define CMD_SET_RAMX_START_END 0x44
#define CMD_SET_RAMY_START_END 0x45
#define CMD_AUTO_WRITE_RED_RAM_REG_PATTERN 0x46
#define CMD_AUTO_WRITE_BW_RAM_REG_PATTERN 0x47
#define CMD_SET_RAMX_COUNTER 0x4E
#define CMD_SET_RAMY_COUNTER 0x4F
#define CMD_SLEEP 0x50

void EPD_Init(void);
void EPD_Clear(uint8_t color /*0x00=czarny, 0xFF=biały*/);
void EPD_WriteBW(const uint8_t *buf /* (EPD_W*EPD_H)/8 */);
void EPD_Refresh(void);
void EPD_Sleep(void);

// tryb aktualizacji
typedef enum
{
    EPD_UPDATE_FULL, // LUT_1GRAY_GC (pełne odświeżenie)
    EPD_UPDATE_QUICK // LUT_1GRAY_DU (szybkie/DU)
} EPD_UpdateMode;

void EPD_SetUpdateMode(EPD_UpdateMode mode);
void EPD_SetLUT_Full(void);
void EPD_SetLUT_Quick(void);

// partial update (x,y,w,h w pikselach; buf = 1bpp, wiersze kolejno, MSB first)
HAL_StatusTypeDef EPD_UpdateWindow(uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h,
                                   const uint8_t *buf);

// pomocnicze: pełne okno (przywraca zakres całego ekranu)
void EPD_SetFullWindow(void);