#include "app/drivers/rfm69.h"
#include <string.h>
#include <math.h>

// ---- LOW-LEVEL SPI CS ----
void RFM69_Select(RFM69_HandleTypeDef *hrf) { HAL_GPIO_WritePin(hrf->cs_port, hrf->cs_pin, GPIO_PIN_RESET); }
void RFM69_Unselect(RFM69_HandleTypeDef *hrf) { HAL_GPIO_WritePin(hrf->cs_port, hrf->cs_pin, GPIO_PIN_SET); }

// ---- REG R/W ----
uint8_t RFM69_ReadReg(RFM69_HandleTypeDef *hrf, uint8_t addr)
{
    uint8_t tx[2] = {(uint8_t)(addr & 0x7F), 0x00};
    uint8_t rx[2] = {0};
    RFM69_Select(hrf);
    HAL_SPI_TransmitReceive(hrf->hspi, tx, rx, 2, HAL_MAX_DELAY);
    RFM69_Unselect(hrf);
    return rx[1];
}

void RFM69_WriteReg(RFM69_HandleTypeDef *hrf, uint8_t addr, uint8_t value)
{
    uint8_t tx[2] = {(uint8_t)(addr | 0x80), value};
    RFM69_Select(hrf);
    HAL_SPI_Transmit(hrf->hspi, tx, 2, HAL_MAX_DELAY);
    RFM69_Unselect(hrf);
}

// ---- MODE ----
void RFM69_SetMode(RFM69_HandleTypeDef *hrf, RFM69_Mode newMode)
{
    if (hrf->mode == newMode)
        return;

    uint8_t op = RFM69_ReadReg(hrf, REG_OPMODE) & 0xE3;
    switch (newMode)
    {
    case RF69_MODE_TX:
        RFM69_WriteReg(hrf, REG_OPMODE, op | RF_OPMODE_TRANSMITTER);
        if (hrf->isRFM69HW)
        { // HiPower rejestry tylko w TX
            if (hrf->powerLevel >= 20)
            {
                RFM69_WriteReg(hrf, REG_TESTPA1, 0x5D);
                RFM69_WriteReg(hrf, REG_TESTPA2, 0x7C);
            }
        }
        break;
    case RF69_MODE_RX:
        RFM69_WriteReg(hrf, REG_OPMODE, op | RF_OPMODE_RECEIVER);
        if (hrf->isRFM69HW)
        { // wyłącz HiPower w RX
            RFM69_WriteReg(hrf, REG_TESTPA1, 0x55);
            RFM69_WriteReg(hrf, REG_TESTPA2, 0x70);
        }
        break;
    case RF69_MODE_SYNTH:
        RFM69_WriteReg(hrf, REG_OPMODE, op | RF_OPMODE_SYNTHESIZER);
        break;
    case RF69_MODE_STANDBY:
        RFM69_WriteReg(hrf, REG_OPMODE, op | RF_OPMODE_STANDBY);
        break;
    case RF69_MODE_SLEEP:
        RFM69_WriteReg(hrf, REG_OPMODE, op | RF_OPMODE_SLEEP);
        break;
    default:
        return;
    }

    if (hrf->mode == RF69_MODE_SLEEP)
    {
        // po wyjściu ze SLEEP poczekaj na MODEREADY
        while ((RFM69_ReadReg(hrf, REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0)
            ;
    }
    hrf->mode = newMode;
}

void RFM69_Sleep(RFM69_HandleTypeDef *hrf) { RFM69_SetMode(hrf, RF69_MODE_SLEEP); }

// ---- GETTERS/SETTERS ----
uint8_t RFM69_GetVersion(RFM69_HandleTypeDef *hrf) { return RFM69_ReadReg(hrf, REG_VERSION); }

uint32_t RFM69_GetFrequency(RFM69_HandleTypeDef *hrf)
{
    uint32_t frf = ((uint32_t)RFM69_ReadReg(hrf, REG_FRFMSB) << 16) |
                   ((uint32_t)RFM69_ReadReg(hrf, REG_FRFMID) << 8) |
                   RFM69_ReadReg(hrf, REG_FRFLSB);
    return (uint32_t)(RF69_FSTEP * frf);
}

void RFM69_SetFrequency(RFM69_HandleTypeDef *hrf, uint32_t freqHz)
{
    RFM69_Mode old = hrf->mode;
    if (old == RF69_MODE_TX)
        RFM69_SetMode(hrf, RF69_MODE_RX);
    uint32_t frf = (uint32_t)(freqHz / RF69_FSTEP);
    RFM69_WriteReg(hrf, REG_FRFMSB, (frf >> 16) & 0xFF);
    RFM69_WriteReg(hrf, REG_FRFMID, (frf >> 8) & 0xFF);
    RFM69_WriteReg(hrf, REG_FRFLSB, frf & 0xFF);
    if (old == RF69_MODE_RX)
        RFM69_SetMode(hrf, RF69_MODE_SYNTH);
    RFM69_SetMode(hrf, old);
}

uint32_t RFM69_GetFreqDev(RFM69_HandleTypeDef *hrf)
{
    uint16_t v = ((uint16_t)RFM69_ReadReg(hrf, REG_FDEVMSB) << 8) | RFM69_ReadReg(hrf, REG_FDEVLSB);
    return (uint32_t)(RF69_FSTEP * v);
}

uint32_t RFM69_GetBitrate(RFM69_HandleTypeDef *hrf)
{
    uint16_t br = ((uint16_t)RFM69_ReadReg(hrf, REG_BITRATEMSB) << 8) | RFM69_ReadReg(hrf, REG_BITRATELSB);
    return RF69_FXOSC / (br ? br : 1);
}

void RFM69_SetAddress(RFM69_HandleTypeDef *hrf, uint16_t addr)
{
    hrf->address = addr;
    RFM69_WriteReg(hrf, REG_NODEADRS, (uint8_t)addr);
}
uint16_t RFM69_GetAddress(RFM69_HandleTypeDef *hrf) { return hrf->address; }

void RFM69_SetNetwork(RFM69_HandleTypeDef *hrf, uint8_t networkID)
{
    hrf->networkID = networkID;
    RFM69_WriteReg(hrf, REG_SYNCVALUE2, networkID);
}
uint8_t RFM69_GetNetwork(RFM69_HandleTypeDef *hrf) { return hrf->networkID; }

void RFM69_Encrypt(RFM69_HandleTypeDef *hrf, const char *key16_or_null)
{
    RFM69_SetMode(hrf, RF69_MODE_STANDBY);
    uint8_t ena = (key16_or_null && key16_or_null[0]);
    if (ena)
    {
        RFM69_Select(hrf);
        uint8_t reg = REG_AESKEY1 | 0x80;
        HAL_SPI_Transmit(hrf->hspi, &reg, 1, HAL_MAX_DELAY);
        HAL_SPI_Transmit(hrf->hspi, (uint8_t *)key16_or_null, 16, HAL_MAX_DELAY);
        RFM69_Unselect(hrf);
    }
    uint8_t pc2 = RFM69_ReadReg(hrf, REG_PACKETCONFIG2);
    pc2 = (pc2 & 0xFE) | (ena ? 1 : 0);
    RFM69_WriteReg(hrf, REG_PACKETCONFIG2, pc2);
}

int16_t RFM69_ReadRSSI(RFM69_HandleTypeDef *hrf, bool forceTrigger)
{
    if (forceTrigger)
    {
        RFM69_WriteReg(hrf, REG_RSSICONFIG, RF_RSSI_START);
        while ((RFM69_ReadReg(hrf, REG_RSSICONFIG) & RF_RSSI_DONE) == 0)
            ;
    }
    int16_t r = -(int16_t)RFM69_ReadReg(hrf, REG_RSSIVALUE);
    return r >> 1;
}

// ---- POWER ----
void RFM69_SetHighPower(RFM69_HandleTypeDef *hrf, uint8_t isHW)
{
    hrf->isRFM69HW = isHW ? 1 : 0;
    RFM69_WriteReg(hrf, REG_OCP, isHW ? RF_OCP_OFF : RF_OCP_ON);
    RFM69_SetPowerLevel(hrf, hrf->powerLevel ? hrf->powerLevel : 31);
}

uint8_t RFM69_GetPowerLevel(RFM69_HandleTypeDef *hrf) { return hrf->powerLevel; }

void RFM69_SetPowerLevel(RFM69_HandleTypeDef *hrf, uint8_t level)
{
    uint8_t pa = 0;
    if (hrf->isRFM69HW)
    {
        if (level > 23)
            level = 23;
        hrf->powerLevel = level;
        uint8_t regVal = level;
        if (level < 16)
        {
            regVal += 16; // 16..31
            pa = RF_PALEVEL_PA1_ON;
        }
        else
        {
            regVal += (level < 20) ? 10 : 8; // 26..31 lub 28..31
            pa = RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON;
        }
        RFM69_WriteReg(hrf, REG_PALEVEL, pa | (regVal & 0x1F));
    }
    else
    {
        if (level > 31)
            level = 31;
        hrf->powerLevel = level;
        pa = RF_PALEVEL_PA0_ON;
        RFM69_WriteReg(hrf, REG_PALEVEL, pa | (level & 0x1F));
    }
}

int8_t RFM69_SetPowerDBm(RFM69_HandleTypeDef *hrf, int8_t dBm)
{
    if (hrf->isRFM69HW)
    {
        if (dBm < -2)
            dBm = -2;
        else if (dBm > 20)
            dBm = 20;
        if (dBm < 12)
            RFM69_SetPowerLevel(hrf, 2 + dBm);
        else if (dBm < 16)
            RFM69_SetPowerLevel(hrf, 4 + dBm);
        else
            RFM69_SetPowerLevel(hrf, 3 + dBm);
    }
    else
    {
        if (dBm < -18)
            dBm = -18;
        else if (dBm > 13)
            dBm = 13;
        RFM69_SetPowerLevel(hrf, 18 + dBm);
    }
    return dBm;
}

// ---- TX/RX core ----
static void RFM69_ReceiveBegin(RFM69_HandleTypeDef *hrf)
{
    hrf->DATALEN = 0;
    hrf->SENDERID = 0;
    hrf->TARGETID = 0;
    hrf->PAYLOADLEN = 0;
    hrf->ACK_REQUESTED = 0;
    hrf->ACK_RECEIVED = 0;
    hrf->RSSI = 0;

    if (RFM69_ReadReg(hrf, REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY)
    {
        uint8_t pc2 = RFM69_ReadReg(hrf, REG_PACKETCONFIG2);
        RFM69_WriteReg(hrf, REG_PACKETCONFIG2, (pc2 & 0xFB) | RF_PACKET2_RXRESTART);
    }
    RFM69_WriteReg(hrf, REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01); // PAYLOADREADY
    RFM69_SetMode(hrf, RF69_MODE_RX);
}

bool RFM69_CanSend(RFM69_HandleTypeDef *hrf)
{
    if (hrf->mode == RF69_MODE_RX && hrf->PAYLOADLEN == 0 && RFM69_ReadRSSI(hrf, false) < -90)
    {
        RFM69_SetMode(hrf, RF69_MODE_STANDBY);
        return true;
    }
    return false;
}

static void RFM69_SendFrame(RFM69_HandleTypeDef *hrf, uint16_t to, const void *buf, uint8_t len, bool reqACK, bool sendACK)
{
    if (len > RFM69_MAX_DATA_LEN)
        len = RFM69_MAX_DATA_LEN;

    RFM69_SetMode(hrf, RF69_MODE_STANDBY);
    while ((RFM69_ReadReg(hrf, REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0)
        ;

    uint8_t ctl = 0;
    if (sendACK)
        ctl = 0x80;
    else if (reqACK)
        ctl = 0x40;
    if (to > 0xFF)
        ctl |= (to & 0x300) >> 6;
    if (hrf->address > 0xFF)
        ctl |= (hrf->address & 0x300) >> 8;

    // FIFO write
    RFM69_Select(hrf);
    uint8_t reg = REG_FIFO | 0x80;
    HAL_SPI_Transmit(hrf->hspi, &reg, 1, HAL_MAX_DELAY);
    uint8_t total = len + 3;
    HAL_SPI_Transmit(hrf->hspi, &total, 1, HAL_MAX_DELAY);
    uint8_t to8 = (uint8_t)to;
    uint8_t me8 = (uint8_t)hrf->address;
    HAL_SPI_Transmit(hrf->hspi, &to8, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(hrf->hspi, &me8, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(hrf->hspi, &ctl, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(hrf->hspi, (uint8_t *)buf, len, HAL_MAX_DELAY);
    RFM69_Unselect(hrf);

    RFM69_SetMode(hrf, RF69_MODE_TX);
    while ((RFM69_ReadReg(hrf, REG_IRQFLAGS2) & RF_IRQFLAGS2_PACKETSENT) == 0)
        ;
    RFM69_SetMode(hrf, RF69_MODE_STANDBY);
}

void RFM69_Send(RFM69_HandleTypeDef *hrf, uint16_t to, const void *buf, uint8_t len, bool requestACK)
{
    uint32_t start = HAL_GetTick();
    // rozwiąż RX deadlock
    uint8_t pc2 = RFM69_ReadReg(hrf, REG_PACKETCONFIG2);
    RFM69_WriteReg(hrf, REG_PACKETCONFIG2, (pc2 & 0xFB) | RF_PACKET2_RXRESTART);

    while (!RFM69_CanSend(hrf) && (HAL_GetTick() - start < RF69_CSMA_LIMIT_MS))
    {
        (void)RFM69_ReceiveDone(hrf); // pompka
    }
    RFM69_SendFrame(hrf, to, buf, len, requestACK, false);
}

bool RFM69_SendWithRetry(RFM69_HandleTypeDef *hrf, uint16_t to, const void *buf, uint8_t len, uint8_t retries, uint8_t retryWaitMs)
{
    for (uint8_t i = 0; i <= retries; i++)
    {
        RFM69_Send(hrf, to, buf, len, true);
        uint32_t t0 = HAL_GetTick();
        while ((HAL_GetTick() - t0) < retryWaitMs)
        {
            if (RFM69_ACKReceived(hrf, to))
                return true;
        }
    }
    return false;
}

bool RFM69_ACKRequested(RFM69_HandleTypeDef *hrf)
{
    return hrf->ACK_REQUESTED && (hrf->TARGETID == hrf->address);
}

void RFM69_SendACK(RFM69_HandleTypeDef *hrf, const void *buf, uint8_t len)
{
    hrf->ACK_REQUESTED = 0;
    uint16_t sender = hrf->SENDERID;
    int16_t rssi = hrf->RSSI;

    uint8_t pc2 = RFM69_ReadReg(hrf, REG_PACKETCONFIG2);
    RFM69_WriteReg(hrf, REG_PACKETCONFIG2, (pc2 & 0xFB) | RF_PACKET2_RXRESTART);

    uint32_t start = HAL_GetTick();
    while (!RFM69_CanSend(hrf) && (HAL_GetTick() - start < RF69_CSMA_LIMIT_MS))
    {
        (void)RFM69_ReceiveDone(hrf);
    }
    hrf->SENDERID = sender;
    RFM69_SendFrame(hrf, sender, buf, len, false, true);
    hrf->RSSI = rssi;
}

// ---- IRQ ----
// Wywołuj to z HAL_GPIO_EXTI_Callback dla pinu DIO0
void RFM69_OnDIO0IRQ(RFM69_HandleTypeDef *hrf)
{
    hrf->haveData = 1;
    if (hrf->isr_cb)
        hrf->isr_cb();
}

// Przetwarzanie payloadu – wołaj cyklicznie albo zaraz po haveData=1
static void RFM69_InterruptHandler(RFM69_HandleTypeDef *hrf)
{
    if (hrf->mode == RF69_MODE_RX && (RFM69_ReadReg(hrf, REG_IRQFLAGS2) & RF_IRQFLAGS2_PAYLOADREADY))
    {
        RFM69_SetMode(hrf, RF69_MODE_STANDBY);

        // odczyt FIFO
        RFM69_Select(hrf);
        uint8_t reg = REG_FIFO & 0x7F;
        HAL_SPI_Transmit(hrf->hspi, &reg, 1, HAL_MAX_DELAY);
        uint8_t len;
        HAL_SPI_Receive(hrf->hspi, &len, 1, HAL_MAX_DELAY);
        len = (len > 66) ? 66 : len;

        uint8_t target, sender, ctl;
        HAL_SPI_Receive(hrf->hspi, &target, 1, HAL_MAX_DELAY);
        HAL_SPI_Receive(hrf->hspi, &sender, 1, HAL_MAX_DELAY);
        HAL_SPI_Receive(hrf->hspi, &ctl, 1, HAL_MAX_DELAY);

        hrf->PAYLOADLEN = len;
        hrf->TARGETID = target;
        hrf->SENDERID = sender;
        hrf->TARGETID |= ((uint16_t)(ctl & 0x0C)) << 6;
        hrf->SENDERID |= ((uint16_t)(ctl & 0x03)) << 8;

        if ((hrf->PAYLOADLEN < 3) ||
            !((hrf->TARGETID == hrf->address) || (hrf->TARGETID == RF69_BROADCAST_ADDR)))
        {
            hrf->PAYLOADLEN = 0;
            RFM69_Unselect(hrf);
            RFM69_ReceiveBegin(hrf);
            return;
        }

        hrf->DATALEN = hrf->PAYLOADLEN - 3;
        hrf->ACK_RECEIVED = (ctl & 0x80) ? 1 : 0;
        hrf->ACK_REQUESTED = (ctl & 0x40) ? 1 : 0;

        if (hrf->DATALEN)
        {
            HAL_SPI_Receive(hrf->hspi, hrf->DATA, hrf->DATALEN, HAL_MAX_DELAY);
        }
        if (hrf->DATALEN < RFM69_MAX_DATA_LEN)
            hrf->DATA[hrf->DATALEN] = 0;

        RFM69_Unselect(hrf);
        RFM69_SetMode(hrf, RF69_MODE_RX);
    }
    hrf->RSSI = RFM69_ReadRSSI(hrf, false);
}

bool RFM69_ReceiveDone(RFM69_HandleTypeDef *hrf)
{
    if (hrf->haveData)
    {
        hrf->haveData = 0;
        RFM69_InterruptHandler(hrf);
    }
    if (hrf->mode == RF69_MODE_RX && hrf->PAYLOADLEN > 0)
    {
        RFM69_SetMode(hrf, RF69_MODE_STANDBY); // pozwól na wysyłkę ACK
        return true;
    }
    else if (hrf->mode == RF69_MODE_RX)
    {
        return false;
    }
    RFM69_ReceiveBegin(hrf);
    return false;
}

// ---- INIT ----
bool RFM69_Init(RFM69_HandleTypeDef *hrf, uint8_t freqBand, uint16_t nodeID, uint8_t networkID)
{
    if (!hrf || !hrf->hspi)
        return false;

    // CS HIGH
    HAL_GPIO_WritePin(hrf->cs_port, hrf->cs_pin, GPIO_PIN_SET);

    hrf->mode = RF69_MODE_STANDBY;
    hrf->powerLevel = 31;
    hrf->haveData = 0;
    hrf->isr_cb = NULL;

    // Soft-probe SPI (SYNCVALUE1 echo test AA->55)
    uint32_t t0 = HAL_GetTick();
    do
    {
        RFM69_WriteReg(hrf, REG_SYNCVALUE1, 0xAA);
    } while (RFM69_ReadReg(hrf, REG_SYNCVALUE1) != 0xAA && (HAL_GetTick() - t0) < 50);
    if ((HAL_GetTick() - t0) >= 50)
        return false;
    t0 = HAL_GetTick();
    do
    {
        RFM69_WriteReg(hrf, REG_SYNCVALUE1, 0x55);
    } while (RFM69_ReadReg(hrf, REG_SYNCVALUE1) != 0x55 && (HAL_GetTick() - t0) < 50);
    if ((HAL_GetTick() - t0) >= 50)
        return false;

    // Konfiguracja jak w oryginale
    const uint8_t CONFIG[][2] = {
        {REG_OPMODE, RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY},
        {REG_DATAMODUL, RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00},
        {REG_BITRATEMSB, RF_BITRATEMSB_55555},
        {REG_BITRATELSB, RF_BITRATELSB_55555},
        {REG_FDEVMSB, RF_FDEVMSB_50000},
        {REG_FDEVLSB, RF_FDEVLSB_50000},

        {REG_FRFMSB, (uint8_t)(freqBand == 31 ? RF_FRFMSB_315 : (freqBand == 43 ? RF_FRFMSB_433 : (freqBand == 86 ? RF_FRFMSB_868 : RF_FRFMSB_915)))},
        {REG_FRFMID, (uint8_t)(freqBand == 31 ? RF_FRFMID_315 : (freqBand == 43 ? RF_FRFMID_433 : (freqBand == 86 ? RF_FRFMID_868 : RF_FRFMID_915)))},
        {REG_FRFLSB, (uint8_t)(freqBand == 31 ? RF_FRFLSB_315 : (freqBand == 43 ? RF_FRFLSB_433 : (freqBand == 86 ? RF_FRFLSB_868 : RF_FRFLSB_915)))},

        {REG_RXBW, RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2},
        {REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01},
        {REG_DIOMAPPING2, RF_DIOMAPPING2_CLKOUT_OFF},
        {REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN},
        {REG_RSSITHRESH, 220},
        {REG_SYNCCONFIG, RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0},
        {REG_SYNCVALUE1, 0x2D},
        {REG_SYNCVALUE2, networkID},
        {REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_ON | RF_PACKET1_CRCAUTOCLEAR_ON | RF_PACKET1_ADRSFILTERING_OFF},
        {REG_PAYLOADLENGTH, 66},
        {REG_FIFOTHRESH, RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE},
        {REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_OFF | RF_PACKET2_AES_OFF},
        {REG_TESTDAGC, RF_DAGC_IMPROVED_LOWBETA0},
        {0xFF, 0}};

    for (uint8_t i = 0; CONFIG[i][0] != 0xFF; i++)
    {
        RFM69_WriteReg(hrf, CONFIG[i][0], CONFIG[i][1]);
    }

    // encryption off na start
    RFM69_Encrypt(hrf, NULL);

    // PA/OCP zgodnie z typem modułu
    RFM69_SetHighPower(hrf, hrf->isRFM69HW);

    // STANDBY -> czekamy na MODE READY
    RFM69_SetMode(hrf, RF69_MODE_STANDBY);
    t0 = HAL_GetTick();
    while ((RFM69_ReadReg(hrf, REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0 && (HAL_GetTick() - t0) < 50)
        ;
    if ((HAL_GetTick() - t0) >= 50)
        return false;

    // adresy/sieć
    hrf->address = nodeID;
    hrf->networkID = networkID;

    // wejdź w RX
    RFM69_WriteReg(hrf, REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01);
    RFM69_SetMode(hrf, RF69_MODE_RX);

    return true;
}

// ---- ACK gates ----
bool RFM69_ACKReceived(RFM69_HandleTypeDef *hrf, uint16_t fromNodeID)
{
    if (RFM69_ReceiveDone(hrf))
    {
        if ((hrf->SENDERID == fromNodeID || fromNodeID == RF69_BROADCAST_ADDR) && hrf->ACK_RECEIVED)
            return true;
    }
    return false;
}
