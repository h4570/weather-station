#ifndef RFM69_H
#define RFM69_H

#include "app/shared_glue/rfm69_glue.h"
#include <stdint.h>
#include <stdbool.h>
#include "rfm69_registers.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RFM69_MAX_DATA_LEN 61
#define RF69_CSMA_LIMIT_MS 1000
#define RF69_TX_LIMIT_MS 1000
#define RF69_FXOSC 32000000UL
#define RF69_FSTEP 61.03515625 // 32MHz / 2^19
#define RF69_BROADCAST_ADDR 0

    typedef enum
    {
        RF69_MODE_SLEEP = 0,
        RF69_MODE_STANDBY = 1,
        RF69_MODE_SYNTH = 2,
        RF69_MODE_RX = 3,
        RF69_MODE_TX = 4
    } RFM69_Mode;

    typedef struct
    {
        SPI_HandleTypeDef *hspi;

        GPIO_TypeDef *cs_port;
        uint16_t cs_pin;

        GPIO_TypeDef *dio0_port;
        uint16_t dio0_pin;

        // konfiguracja
        uint8_t isRFM69HW; // 0=W/CW, 1=HW/HCW
        uint16_t address;
        uint8_t networkID;

        // stan
        volatile uint8_t mode;
        volatile uint8_t haveData;

        // RX/TX bufor/stany kompatybilne z oryginałem
        uint8_t DATA[RFM69_MAX_DATA_LEN + 1];
        uint8_t DATALEN;
        uint16_t SENDERID;
        uint16_t TARGETID;
        uint8_t PAYLOADLEN;
        uint8_t ACK_REQUESTED;
        uint8_t ACK_RECEIVED;
        int16_t RSSI;
        uint8_t powerLevel;

        // callback użytkownika na IRQ (opcjonalny)
        void (*isr_cb)(void);
    } RFM69_HandleTypeDef;

    // API
    bool RFM69_Init(RFM69_HandleTypeDef *hrf, uint8_t freqBand, uint16_t nodeID, uint8_t networkID);
    uint8_t RFM69_ReadReg(RFM69_HandleTypeDef *hrf, uint8_t addr);
    void RFM69_WriteReg(RFM69_HandleTypeDef *hrf, uint8_t addr, uint8_t value);
    void RFM69_SetMode(RFM69_HandleTypeDef *hrf, RFM69_Mode newMode);
    void RFM69_Sleep(RFM69_HandleTypeDef *hrf);

    uint8_t RFM69_GetVersion(RFM69_HandleTypeDef *hrf);
    uint32_t RFM69_GetFrequency(RFM69_HandleTypeDef *hrf);
    void RFM69_SetFrequency(RFM69_HandleTypeDef *hrf, uint32_t freqHz);
    uint32_t RFM69_GetFreqDev(RFM69_HandleTypeDef *hrf);
    uint32_t RFM69_GetBitrate(RFM69_HandleTypeDef *hrf);

    void RFM69_SetAddress(RFM69_HandleTypeDef *hrf, uint16_t addr);
    uint16_t RFM69_GetAddress(RFM69_HandleTypeDef *hrf);
    void RFM69_SetNetwork(RFM69_HandleTypeDef *hrf, uint8_t networkID);
    uint8_t RFM69_GetNetwork(RFM69_HandleTypeDef *hrf);

    void RFM69_Encrypt(RFM69_HandleTypeDef *hrf, const char *key16_or_null);

    int16_t RFM69_ReadRSSI(RFM69_HandleTypeDef *hrf, bool forceTrigger);
    void RFM69_SetHighPower(RFM69_HandleTypeDef *hrf, uint8_t isHW);
    uint8_t RFM69_GetPowerLevel(RFM69_HandleTypeDef *hrf);
    void RFM69_SetPowerLevel(RFM69_HandleTypeDef *hrf, uint8_t level);
    int8_t RFM69_SetPowerDBm(RFM69_HandleTypeDef *hrf, int8_t dBm);

    bool RFM69_CanSend(RFM69_HandleTypeDef *hrf);
    void RFM69_Send(RFM69_HandleTypeDef *hrf, uint16_t toAddress, const void *buffer, uint8_t size, bool requestACK);
    bool RFM69_SendWithRetry(RFM69_HandleTypeDef *hrf, uint16_t toAddress, const void *buffer, uint8_t size, uint8_t retries, uint8_t retryWaitMs);
    bool RFM69_ReceiveDone(RFM69_HandleTypeDef *hrf);
    bool RFM69_ACKReceived(RFM69_HandleTypeDef *hrf, uint16_t fromNodeID);
    bool RFM69_ACKRequested(RFM69_HandleTypeDef *hrf);
    void RFM69_SendACK(RFM69_HandleTypeDef *hrf, const void *buffer, uint8_t size);

    // ISR bridge – wywołaj to z HAL_GPIO_EXTI_Callback dla pinu DIO0:
    void RFM69_OnDIO0IRQ(RFM69_HandleTypeDef *hrf);

    // pomocnicze
    void RFM69_Select(RFM69_HandleTypeDef *hrf);
    void RFM69_Unselect(RFM69_HandleTypeDef *hrf);

#ifdef __cplusplus
}
#endif
#endif // RFM69_H
