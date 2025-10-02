#include "app/radio.h"
#include "shared/drivers/rfm69.h"

static RFM69_HandleTypeDef radio_rfm69_handle;
static volatile uint16_t radio_it_di0_pin = 0;

radio_handle radio_create(GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *dio0_port, uint16_t dio0_pin, SPI_HandleTypeDef *hspi)
{
    radio_handle handle;
    handle.cs_port = cs_port;
    handle.cs_pin = cs_pin;
    handle.dio0_port = dio0_port;
    handle.dio0_pin = dio0_pin;
    handle.hspi = hspi;
    return handle;
}

// Przy inicjalizacji:
// Domyślnie używamy 2-bajtowego SYNC (0x2D, networkID). Jeśli masz hałas/kolizje, rozważ dłuższy SYNC (np. 4 bajty) – jeszcze lepsze filtrowanie:
// RFM69_WriteReg(&radio_rfm69_handle, REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_4 | RF_SYNC_TOL_0);
// RFM69_WriteReg(&radio_rfm69_handle, REG_SYNCVALUE1, 0x2D);
// RFM69_WriteReg(&radio_rfm69_handle, REG_SYNCVALUE2, networkID);
// RFM69_WriteReg(&radio_rfm69_handle, REG_SYNCVALUE3, 0xAA);
// RFM69_WriteReg(&radio_rfm69_handle, REG_SYNCVALUE4, 0x55);

// Uwagi techniczne (ważne)
// SPI: ustaw CLKPolarity=LOW, CLKPhase=1EDGE (MODE0).
// CS: steruj ręcznie (HAL nie podnosi go automatycznie).
// DIO0: mapuj na EXTI z RISING; nie ma attachInterrupt/detachInterrupt – robisz to w CubeMX.
// Zasilanie/RF: dla HW/HCW pamiętaj o dopuszczalnym czasie TX i chłodzeniu przy wysokiej mocy.
// Czasy: używam HAL_GetTick() i HAL_Delay(); jak chcesz bezblokowo/RTOS – przerób pętle oczekiwania.
// Encrypt: klucz 16 bajtów; RFM69_Encrypt(&radio_rfm69_handle, "ABCDEFGHIJKLMNOP"); lub NULL by wyłączyć.

void radio_init(radio_handle *handle)
{
    radio_rfm69_handle.hspi = handle->hspi;

    radio_rfm69_handle.cs_port = handle->cs_port;
    radio_rfm69_handle.cs_pin = handle->cs_pin;
    radio_rfm69_handle.dio0_port = handle->dio0_port;
    radio_rfm69_handle.dio0_pin = handle->dio0_pin;

    radio_rfm69_handle.isRFM69HW = 1; // ustaw 1 dla RFM69HW/HCW, 0 dla W/CW

    // Upewnij się, że CS jest jako OUTPUT i w stanie HIGH
    HAL_GPIO_WritePin(radio_rfm69_handle.cs_port, radio_rfm69_handle.cs_pin, GPIO_PIN_SET);

    // start
    if (!RFM69_Init(&radio_rfm69_handle, /*freqBand*/ 86, /*nodeID*/ 1, /*networkID*/ 42))
    {
        // błąd inicjalizacji -> obsłuż
        Error_Handler();
    }

    radio_it_di0_pin = handle->dio0_pin;

    // przykładowa moc
    RFM69_SetPowerDBm(&radio_rfm69_handle, 13);

    uint8_t v = RFM69_GetVersion(&radio_rfm69_handle);
    uint32_t freq = RFM69_GetFrequency(&radio_rfm69_handle);
    // TODO: Dodać assert po obu stronach na freq i version
}

void radio_loop(radio_handle *handle, app_device_data *out)
{
    if (RFM69_ReceiveDone(&radio_rfm69_handle))
    {
        // dane w radio_rfm69_handle.DATA (radio_rfm69_handle.DATALEN bajtów)
        // nadawca: radio_rfm69_handle.SENDERID
        // pakiet: uint8_t packet[18]; // 1x byte 'S', 2x float, 2x int32_t, 1x byte 'E'
        //    float temperature; /**< Temperature in degrees Celsius */
        //    float humidity;    /**< Humidity in percentage */
        //    int32_t pressure;  /**< Pressure in Pascals */
        //    int32_t bat_in;    /**< Battery input in millivolts */

        if (radio_rfm69_handle.DATALEN == 18 && radio_rfm69_handle.DATA[0] == 'S' && radio_rfm69_handle.DATA[17] == 'E')
        {
            memcpy(&out->temperature, &radio_rfm69_handle.DATA[1], sizeof(float));
            memcpy(&out->humidity, &radio_rfm69_handle.DATA[5], sizeof(float));
            memcpy(&out->pressure, &radio_rfm69_handle.DATA[9], sizeof(int32_t));
            memcpy(&out->bat_in, &radio_rfm69_handle.DATA[13], sizeof(int32_t));
        }

        if (RFM69_ACKRequested(&radio_rfm69_handle))
        {
            const char ok[] = "OK";
            RFM69_SendACK(&radio_rfm69_handle, ok, sizeof ok - 1);
        }

        radio_rfm69_handle.PAYLOADLEN = 0; // <-- consume packet

        // wróć do RX (ReceiveDone już przerzuca w STANDBY, więc można ręcznie)
        RFM69_SetMode(&radio_rfm69_handle, RF69_MODE_RX);
    }

    // Wysyłka
    // const char msg[] = "hello";
    // RFM69_Send(&radio_rfm69_handle, /*toAddress*/ 2, msg, sizeof msg - 1, /*requestACK*/ true);

    // if (RFM69_SendWithRetry(&radio_rfm69_handle, 2, msg, sizeof msg - 1, /*retries*/ 2, /*retryWaitMs*/ 30))
    // {
    //     // sukces
    // }
    // else
    // {
    //     // brak ACK
    // }
}

void radio_exti_interrupt_handler(const uint16_t pin)
{
    if (pin == radio_it_di0_pin)
    {
        RFM69_OnDIO0IRQ(&radio_rfm69_handle);
    }
}