#include "epd3v7.h"

// --- LUTy z Twojego kodu (105 bajtów) ---
static const uint8_t LUT_1GRAY_GC[105] = {
    0x2A, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2A, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x03, 0x0A, 0x00, 0x02, 0x06, 0x0A, 0x05, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x22, 0x22, 0x22, 0x22, 0x22};

static const uint8_t LUT_1GRAY_DU[105] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0A, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x05, 0x00, 0x05, 0x03, 0x05, 0x05, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x22, 0x22, 0x22, 0x22, 0x22};

// --- prymitywy GPIO/SPI ---
static inline void cs_low(void) { HAL_GPIO_WritePin(EPD_CS_GPIO_Port, EPD_CS_Pin, GPIO_PIN_RESET); }
static inline void cs_high(void) { HAL_GPIO_WritePin(EPD_CS_GPIO_Port, EPD_CS_Pin, GPIO_PIN_SET); }
static inline void dc_cmd(void) { HAL_GPIO_WritePin(EPD_DC_GPIO_Port, EPD_DC_Pin, GPIO_PIN_RESET); }
static inline void dc_data(void) { HAL_GPIO_WritePin(EPD_DC_GPIO_Port, EPD_DC_Pin, GPIO_PIN_SET); }
static inline void rst_low(void) { HAL_GPIO_WritePin(EPD_RST_GPIO_Port, EPD_RST_Pin, GPIO_PIN_RESET); }
static inline void rst_high(void) { HAL_GPIO_WritePin(EPD_RST_GPIO_Port, EPD_RST_Pin, GPIO_PIN_SET); }

static void WaitBusy(void)
{
#if EPD_BUSY_ACTIVE_LOW
    // Zajęty = LOW
    while (HAL_GPIO_ReadPin(EPD_BUSY_GPIO_Port, EPD_BUSY_Pin) == GPIO_PIN_RESET)
    {
        HAL_Delay(1);
    }
#else
    // Zajęty = HIGH
    while (HAL_GPIO_ReadPin(EPD_BUSY_GPIO_Port, EPD_BUSY_Pin) == GPIO_PIN_SET)
    {
        HAL_Delay(1);
    }
#endif
}

static void HardReset(void)
{
    rst_low();
    HAL_Delay(30);
    rst_high();
    HAL_Delay(10);
}

static void SendCommand(uint8_t cmd)
{
    cs_low();
    dc_cmd();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    cs_high();
}

static void SendData(const uint8_t *data, uint32_t len)
{
    cs_low();
    dc_data();
    HAL_SPI_Transmit(&hspi2, (uint8_t *)data, len, HAL_MAX_DELAY);
    cs_high();
}

static void SendDataRepeat(uint8_t value, uint32_t len)
{
    cs_low();
    dc_data();
    for (uint32_t i = 0; i < len; ++i)
    {
        HAL_SPI_Transmit(&hspi2, &value, 1, HAL_MAX_DELAY);
    }
    cs_high();
}

// --- INIT dopasowany do Twojego Rust ---
void EPD_Init(void)
{
    HardReset();

    // SW reset + 300 ms
    SendCommand(CMD_SW_RESET);
    HAL_Delay(300);

    // Auto write patterns (jak u Ciebie) + czekanie
    {
        uint8_t d[] = {0xF7};
        SendCommand(CMD_AUTO_WRITE_RED_RAM_REG_PATTERN);
        SendData(d, 1);
    }
    WaitBusy();
    {
        uint8_t d[] = {0xF7};
        SendCommand(CMD_AUTO_WRITE_BW_RAM_REG_PATTERN);
        SendData(d, 1);
    }
    WaitBusy();

    // Power/Gate ustawienia
    {
        uint8_t d[] = {0xDF, 0x01, 0x00};
        SendCommand(CMD_GATE_SETTING);
        SendData(d, sizeof(d));
    }
    {
        uint8_t d[] = {0x00};
        SendCommand(CMD_GATE_VOLTAGE);
        SendData(d, sizeof(d));
    }
    {
        uint8_t d[] = {0x41, 0xA8, 0x32};
        SendCommand(CMD_GATE_VOLTAGE_SOURCE);
        SendData(d, sizeof(d));
    }

    {
        uint8_t d[] = {0x03};
        SendCommand(CMD_DATA_ENTRY_SEQUENCE);
        SendData(d, sizeof(d));
    }
    {
        uint8_t d[] = {0x03};
        SendCommand(CMD_BORDER_WAVEFORM_CONTROL);
        SendData(d, sizeof(d));
    }

    {
        uint8_t d[] = {0xAE, 0xC7, 0xC3, 0xC0, 0xC0};
        SendCommand(CMD_BOOSTER_SOFT_START_CONTROL);
        SendData(d, sizeof(d));
    }

    {
        uint8_t d[] = {0x80};
        SendCommand(CMD_TEMPERATURE_SENSOR_SELECTION);
        SendData(d, sizeof(d));
    }

    // VCOM – w razie „zwiechy” sprawdź polaryzację BUSY (patrz makro u góry)
    {
        uint8_t d[] = {0x44};
        SendCommand(CMD_WRITE_VCOM_REGISTER);
        SendData(d, sizeof(d));
    }

    { // Display Option (10 bajtów)
        uint8_t d[] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x4F, 0xFF, 0xFF, 0xFF, 0xFF};
        SendCommand(CMD_DISPLAY_OPTION);
        SendData(d, sizeof(d));
    }

    // Okno RAM (X i Y) – dokładnie jak u Ciebie
    // {
    //     uint8_t d[] = {0x00, 0x00, 0x17, 0x01};
    //     SendCommand(CMD_SET_RAMX_START_END);
    //     SendData(d, sizeof(d));
    // }
    // {
    //     uint8_t d[] = {0x00, 0x00, 0xDF, 0x01};
    //     SendCommand(CMD_SET_RAMY_START_END);
    //     SendData(d, sizeof(d));
    // }
    EPD_SetFullWindow();

    // // Display Update Sequence Setting (0xCF)
    // {
    //     uint8_t d[] = {0xCF};
    //     SendCommand(CMD_DISPLAY_UPDATE_SEQUENCE_SETTING);
    //     SendData(d, 1);
    // }

    // // LUT Full
    // SendCommand(CMD_WRITE_LUT_REGISTER);
    // SendData(LUT_1GRAY_GC, sizeof(LUT_1GRAY_GC));
    EPD_SetLUT_Full();
}

// Zapis pełnej ramki BW i odświeżenie
void EPD_WriteBW(const uint8_t *buf)
{
    // Ustaw liczniki adresów
    {
        uint8_t d[] = {0x00, 0x00};
        SendCommand(CMD_SET_RAMX_COUNTER);
        SendData(d, 2);
    }
    {
        uint8_t d[] = {0x00, 0x00};
        SendCommand(CMD_SET_RAMY_COUNTER);
        SendData(d, 2);
    }

    // Zapis RAM (0x24)
    SendCommand(CMD_WRITE_RAM);
    // Uwaga: w Twoim Rust „data_x_times” liczy w PIXELACH. My wysyłamy bajty.
    // Na tym kontrolerze dane to 1bpp, więc bajtów jest (W*H)/8
    const uint32_t bytes = (EPD_W * EPD_H) / 8u;
    SendData(buf, bytes);
}

void EPD_Clear(uint8_t color)
{
    {
        uint8_t d[] = {0x00, 0x00};
        SendCommand(CMD_SET_RAMX_COUNTER);
        SendData(d, 2);
    }
    {
        uint8_t d[] = {0x00, 0x00};
        SendCommand(CMD_SET_RAMY_COUNTER);
        SendData(d, 2);
    }

    SendCommand(CMD_WRITE_RAM);
    // UWAGA: tu kontroler oczekuje ilości „pikseli” w Twojej bibliotece, ale
    // po SPI i tak idą bajty – powielamy bajt po bajcie:
    const uint32_t bytes = (EPD_W * EPD_H) / 8u;
    SendDataRepeat(color, bytes);
}

void EPD_Refresh(void)
{
    // Wg Twojego kodu: po wgraniu ramki -> 0x20 i czekamy na BUSY
    SendCommand(CMD_DISPLAY_UPDATE_SEQUENCE);
    WaitBusy();
}

void EPD_Sleep(void)
{
    {
        uint8_t d[] = {0xF7};
        SendCommand(CMD_SLEEP);
        SendData(d, 1);
    }
    SendCommand(CMD_POWEROFF);
    {
        uint8_t d[] = {0xA5};
        SendCommand(CMD_SLEEP2);
        SendData(d, 1);
    }
}

static EPD_UpdateMode s_mode = EPD_UPDATE_FULL;

void EPD_SetLUT_Full(void)
{
    SendCommand(CMD_WRITE_LUT_REGISTER);
    SendData(LUT_1GRAY_GC, sizeof(LUT_1GRAY_GC));
    s_mode = EPD_UPDATE_FULL;

    // Ustaw „Display Update Sequence Setting”.
    // W wielu driverach 0xCF działa dla GC i DU, ale często dla DU używa się 0xC7.
    // Zaczynamy od 0xCF – jeśli zobaczysz smużenie przy DU, podmień na 0xC7.
    uint8_t d[] = {0xCF};
    SendCommand(CMD_DISPLAY_UPDATE_SEQUENCE_SETTING);
    SendData(d, sizeof(d));
}

void EPD_SetLUT_Quick(void)
{
    SendCommand(CMD_WRITE_LUT_REGISTER);
    SendData(LUT_1GRAY_DU, sizeof(LUT_1GRAY_DU));
    s_mode = EPD_UPDATE_QUICK;

    // „Szybsza” sekwencja – spróbuj 0xC7 jeśli 0xCF nie daje efektu DU.
    uint8_t d[] = {0xC7};
    SendCommand(CMD_DISPLAY_UPDATE_SEQUENCE_SETTING);
    SendData(d, sizeof(d));
}

void EPD_SetUpdateMode(EPD_UpdateMode mode)
{
    if (mode == EPD_UPDATE_QUICK)
        EPD_SetLUT_Quick();
    else
        EPD_SetLUT_Full();
}

void EPD_SetFullWindow(void)
{
    uint8_t xw[] = {0x00, 0x00, (uint8_t)((EPD_W - 1) & 0xFF), (uint8_t)(((EPD_W - 1) >> 8) & 0xFF)};
    uint8_t yw[] = {0x00, 0x00, (uint8_t)((EPD_H - 1) & 0xFF), (uint8_t)(((EPD_H - 1) >> 8) & 0xFF)};

    SendCommand(CMD_SET_RAMX_START_END);
    SendData(xw, sizeof(xw));

    SendCommand(CMD_SET_RAMY_START_END);
    SendData(yw, sizeof(yw));

    uint8_t xc[] = {0x00, 0x00};
    uint8_t yc[] = {0x00, 0x00};

    SendCommand(CMD_SET_RAMX_COUNTER);
    SendData(xc, sizeof(xc));

    SendCommand(CMD_SET_RAMY_COUNTER);
    SendData(yc, sizeof(yc));
}

static inline uint16_t ceil_div8(uint16_t v) { return (v + 7u) >> 3; }

HAL_StatusTypeDef EPD_UpdateWindow(uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h,
                                   const uint8_t *buf)
{
    // Granice
    if (x >= EPD_W || y >= EPD_H)
        return HAL_ERROR;
    if (!w || !h)
        return HAL_OK; // nic do zrobienia
    if ((uint32_t)x + w > EPD_W)
        w = EPD_W - x;
    if ((uint32_t)y + h > EPD_H)
        h = EPD_H - y;

    // WYMAGANE: x wyrównany do 8 pikseli (1 bajt) – uproszczenie.
    if (x & 0x7)
        return HAL_ERROR;

    // 1) Ustaw okno RAM
    uint8_t x_start_lo = (uint8_t)(x & 0xFF);
    uint8_t x_start_hi = (uint8_t)((x >> 8) & 0xFF);
    uint16_t x_end = x + w - 1;
    uint8_t x_end_lo = (uint8_t)(x_end & 0xFF);
    uint8_t x_end_hi = (uint8_t)((x_end >> 8) & 0xFF);

    uint8_t y_start_lo = (uint8_t)(y & 0xFF);
    uint8_t y_start_hi = (uint8_t)((y >> 8) & 0xFF);
    uint16_t y_end = y + h - 1;
    uint8_t y_end_lo = (uint8_t)(y_end & 0xFF);
    uint8_t y_end_hi = (uint8_t)((y_end >> 8) & 0xFF);

    uint8_t xw[] = {x_start_lo, x_start_hi, x_end_lo, x_end_hi};
    uint8_t yw[] = {y_start_lo, y_start_hi, y_end_lo, y_end_hi};
    SendCommand(CMD_SET_RAMX_START_END);
    SendData(xw, sizeof(xw));
    SendCommand(CMD_SET_RAMY_START_END);
    SendData(yw, sizeof(yw));

    // 2) Ustaw liczniki na początek okna
    uint8_t xc[] = {x_start_lo, x_start_hi};
    uint8_t yc[] = {y_start_lo, y_start_hi};
    SendCommand(CMD_SET_RAMX_COUNTER);
    SendData(xc, sizeof(xc));
    SendCommand(CMD_SET_RAMY_COUNTER);
    SendData(yc, sizeof(yc));

    // 3) Zapis danych do 0x24
    SendCommand(CMD_WRITE_RAM);
    const uint32_t bytes_per_line = ceil_div8(w);
    const uint32_t total_bytes = bytes_per_line * (uint32_t)h;
    SendData(buf, total_bytes);

    // 4) Odśwież wg aktualnego LUT/trybu
    EPD_Refresh();

    return HAL_OK;
}
