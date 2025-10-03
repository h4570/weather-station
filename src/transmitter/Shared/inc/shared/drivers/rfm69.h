#ifndef RFM69_H
#define RFM69_H

/**
 * @file rfm69.h
 * @brief RFM69 Radio Module Driver
 *
 * This driver provides a comprehensive interface for the RFM69 series radio modules
 * including RFM69W/CW and RFM69HW/HCW variants. It supports packet-based communication
 * with automatic acknowledgment, encryption, and power management features.
 *
 * Key features:
 * - Support for both standard and high-power variants
 * - Automatic packet handling with CRC
 * - ACK/NACK protocol support
 * - AES encryption capability
 * - RSSI measurement
 * - Multiple frequency bands (315, 433, 868, 915 MHz)
 * - Interrupt-driven operation
 *
 * @author h4570
 * @version 1.0
 */

#include "app/shared_glue/rfm69_glue.h"
#include <stdint.h>
#include <stdbool.h>
#include "rfm69_registers.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @defgroup RFM69_Constants Configuration Constants
 * @{
 */
#define RFM69_MAX_DATA_LEN 61   /**< Maximum payload data length in bytes */
#define RF69_CSMA_LIMIT_MS 1000 /**< CSMA timeout in milliseconds */
#define RF69_TX_LIMIT_MS 1000   /**< Transmission timeout in milliseconds */
#define RF69_TX_TIMEOUT_MS 1000 /**< TX operation timeout in milliseconds */
#define RF69_FXOSC 32000000UL   /**< Crystal oscillator frequency in Hz */
#define RF69_FSTEP 61.03515625  /**< Frequency step size (32MHz / 2^19) */
#define RF69_BROADCAST_ADDR 0   /**< Broadcast address for all nodes */
                                /** @} */

    /**
     * @brief RFM69 Operating Modes
     *
     * Defines the available operating modes for the RFM69 radio module.
     * Each mode has different power consumption and functionality characteristics.
     */
    typedef enum
    {
        RF69_MODE_SLEEP = 0,   /**< Sleep mode - lowest power consumption */
        RF69_MODE_STANDBY = 1, /**< Standby mode - ready for operation */
        RF69_MODE_SYNTH = 2,   /**< Frequency synthesizer mode */
        RF69_MODE_RX = 3,      /**< Receive mode - listening for packets */
        RF69_MODE_TX = 4       /**< Transmit mode - sending packets */
    } RFM69_Mode;

    /**
     * @brief RFM69 Handle Structure
     *
     * Contains all necessary configuration, state, and data buffers for RFM69 operation.
     * This structure must be initialized before use and passed to all driver functions.
     */
    typedef struct
    {
        /** @defgroup RFM69_Hardware Hardware Configuration
         * @{
         */
        SPI_HandleTypeDef *hspi; /**< SPI handle for communication */

        GPIO_TypeDef *cs_port; /**< Chip Select GPIO port */
        uint16_t cs_pin;       /**< Chip Select GPIO pin */

        GPIO_TypeDef *dio0_port; /**< DIO0 interrupt GPIO port */
        uint16_t dio0_pin;       /**< DIO0 interrupt GPIO pin */
        /** @} */

        /** @defgroup RFM69_Config Module Configuration
         * @{
         */
        uint8_t isRFM69HW; /**< Module type: 0=RFM69W/CW, 1=RFM69HW/HCW */
        uint16_t address;  /**< Node address (0-1023) */
        uint8_t networkID; /**< Network ID for packet filtering */
        /** @} */

        /** @defgroup RFM69_State Runtime State
         * @{
         */
        volatile uint8_t mode;     /**< Current operating mode */
        volatile uint8_t haveData; /**< Flag indicating new data received */
        /** @} */

        /** @defgroup RFM69_Buffers Communication Buffers
         * @{
         */
        uint8_t DATA[RFM69_MAX_DATA_LEN + 1]; /**< Received payload data buffer */
        uint8_t DATALEN;                      /**< Length of received payload data */
        uint16_t SENDERID;                    /**< Sender node address of last packet */
        uint16_t TARGETID;                    /**< Target node address of last packet */
        uint8_t PAYLOADLEN;                   /**< Total payload length including headers */
        uint8_t ACK_REQUESTED;                /**< Flag: ACK was requested by sender */
        uint8_t ACK_RECEIVED;                 /**< Flag: ACK was received for sent packet */
        int16_t RSSI;                         /**< RSSI value of last received packet */
        uint8_t powerLevel;                   /**< Current transmission power level */
        /** @} */

        /** @defgroup RFM69_Callback Interrupt Callback
         * @{
         */
        void (*isr_cb)(void); /**< Optional user callback for interrupt handling */
        /** @} */
    } RFM69_HandleTypeDef;

    /** @defgroup RFM69_Init Initialization Functions
     * @{
     */

    /**
     * @brief Initialize RFM69 module
     * @param hrf Pointer to RFM69 handle structure
     * @param freqBand Frequency band (31=315MHz, 43=433MHz, 86=868MHz, 91=915MHz)
     * @param nodeID Node address for this device (0-1023)
     * @param networkID Network ID for packet filtering (0-255)
     * @return true if initialization successful, false otherwise
     */
    bool RFM69_Init(RFM69_HandleTypeDef *hrf, uint8_t freqBand, uint16_t nodeID, uint8_t networkID);

    /** @} */

    /** @defgroup RFM69_LowLevel Low-Level Register Access
     * @{
     */

    /**
     * @brief Read register value
     * @param hrf Pointer to RFM69 handle
     * @param addr Register address
     * @return Register value
     */
    uint8_t RFM69_ReadReg(RFM69_HandleTypeDef *hrf, uint8_t addr);

    /**
     * @brief Write register value
     * @param hrf Pointer to RFM69 handle
     * @param addr Register address
     * @param value Value to write
     */
    void RFM69_WriteReg(RFM69_HandleTypeDef *hrf, uint8_t addr, uint8_t value);

    /**
     * @brief Assert chip select (select device)
     * @param hrf Pointer to RFM69 handle
     */
    void RFM69_Select(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Deassert chip select (deselect device)
     * @param hrf Pointer to RFM69 handle
     */
    void RFM69_Unselect(RFM69_HandleTypeDef *hrf);

    /** @} */

    /** @defgroup RFM69_Mode Mode Control Functions
     * @{
     */

    /**
     * @brief Set operating mode
     * @param hrf Pointer to RFM69 handle
     * @param newMode New operating mode
     */
    void RFM69_SetMode(RFM69_HandleTypeDef *hrf, RFM69_Mode newMode);

    /**
     * @brief Enter sleep mode
     * @param hrf Pointer to RFM69 handle
     */
    void RFM69_Sleep(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Wait for mode ready flag
     * @param hrf Pointer to RFM69 handle
     * @param timeout_ms Timeout in milliseconds (0 = wait forever)
     * @return true if mode ready, false if timeout
     */
    bool RFM69_WaitModeReady(RFM69_HandleTypeDef *hrf, uint32_t timeout_ms);

    /** @} */

    /** @defgroup RFM69_Info Information Functions
     * @{
     */

    /**
     * @brief Get chip version
     * @param hrf Pointer to RFM69 handle
     * @return Chip version register value
     */
    uint8_t RFM69_GetVersion(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Get current frequency
     * @param hrf Pointer to RFM69 handle
     * @return Frequency in Hz
     */
    uint32_t RFM69_GetFrequency(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Set carrier frequency
     * @param hrf Pointer to RFM69 handle
     * @param freqHz Frequency in Hz
     */
    void RFM69_SetFrequency(RFM69_HandleTypeDef *hrf, uint32_t freqHz);

    /**
     * @brief Get frequency deviation
     * @param hrf Pointer to RFM69 handle
     * @return Frequency deviation in Hz
     */
    uint32_t RFM69_GetFreqDev(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Get bit rate
     * @param hrf Pointer to RFM69 handle
     * @return Bit rate in bits per second
     */
    uint32_t RFM69_GetBitrate(RFM69_HandleTypeDef *hrf);

    /** @} */

    /** @defgroup RFM69_Address Address Management
     * @{
     */

    /**
     * @brief Set node address
     * @param hrf Pointer to RFM69 handle
     * @param addr Node address (0-1023)
     */
    void RFM69_SetAddress(RFM69_HandleTypeDef *hrf, uint16_t addr);

    /**
     * @brief Get node address
     * @param hrf Pointer to RFM69 handle
     * @return Current node address
     */
    uint16_t RFM69_GetAddress(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Set network ID
     * @param hrf Pointer to RFM69 handle
     * @param networkID Network ID (0-255)
     */
    void RFM69_SetNetwork(RFM69_HandleTypeDef *hrf, uint8_t networkID);

    /**
     * @brief Get network ID
     * @param hrf Pointer to RFM69 handle
     * @return Current network ID
     */
    uint8_t RFM69_GetNetwork(RFM69_HandleTypeDef *hrf);

    /** @} */

    /** @defgroup RFM69_Security Security Functions
     * @{
     */

    /**
     * @brief Enable/disable AES encryption
     * @param hrf Pointer to RFM69 handle
     * @param key16_or_null 16-byte encryption key or NULL to disable encryption
     */
    void RFM69_Encrypt(RFM69_HandleTypeDef *hrf, const char *key16_or_null);

    /** @} */

    /** @defgroup RFM69_RSSI Signal Strength
     * @{
     */

    /**
     * @brief Read RSSI value
     * @param hrf Pointer to RFM69 handle
     * @param forceTrigger If true, trigger new RSSI measurement
     * @return RSSI value in dBm
     */
    int16_t RFM69_ReadRSSI(RFM69_HandleTypeDef *hrf, bool forceTrigger);

    /** @} */

    /** @defgroup RFM69_Power Power Management
     * @{
     */

    /**
     * @brief Configure high power mode
     * @param hrf Pointer to RFM69 handle
     * @param isHW Set to 1 for RFM69HW/HCW, 0 for RFM69W/CW
     */
    void RFM69_SetHighPower(RFM69_HandleTypeDef *hrf, uint8_t isHW);

    /**
     * @brief Get current power level
     * @param hrf Pointer to RFM69 handle
     * @return Power level (0-31)
     */
    uint8_t RFM69_GetPowerLevel(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Set power level
     * @param hrf Pointer to RFM69 handle
     * @param level Power level (0-31, actual range depends on module type)
     */
    void RFM69_SetPowerLevel(RFM69_HandleTypeDef *hrf, uint8_t level);

    /**
     * @brief Set power level in dBm
     * @param hrf Pointer to RFM69 handle
     * @param dBm Power level in dBm
     * @return Actual power level set in dBm
     */
    int8_t RFM69_SetPowerDBm(RFM69_HandleTypeDef *hrf, int8_t dBm);

    /** @} */

    /** @defgroup RFM69_Communication Communication Functions
     * @{
     */

    /**
     * @brief Check if channel is clear for transmission
     * @param hrf Pointer to RFM69 handle
     * @return true if ready to send, false otherwise
     */
    bool RFM69_CanSend(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Send packet
     * @param hrf Pointer to RFM69 handle
     * @param toAddress Destination node address
     * @param buffer Data to send
     * @param size Number of bytes to send
     * @param requestACK Request acknowledgment from receiver
     * @return true if packet sent successfully, false otherwise
     */
    bool RFM69_Send(RFM69_HandleTypeDef *hrf, uint16_t toAddress, const void *buffer, uint8_t size, bool requestACK);

    /**
     * @brief Send packet with automatic retry
     * @param hrf Pointer to RFM69 handle
     * @param toAddress Destination node address
     * @param buffer Data to send
     * @param size Number of bytes to send
     * @param retries Number of retry attempts
     * @param retryWaitMs Wait time between retries in milliseconds
     * @return true if packet sent and acknowledged, false otherwise
     */
    bool RFM69_SendWithRetry(RFM69_HandleTypeDef *hrf, uint16_t toAddress, const void *buffer, uint8_t size, uint8_t retries, uint8_t retryWaitMs);

    /**
     * @brief Consume received packet and reset state. Clear "packet present" state and go back to RX mode.
     * @param hrf Pointer to RFM69 handle
     */
    void RFM69_Consume(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Check if packet was received
     * @param hrf Pointer to RFM69 handle
     * @return true if new packet available, false otherwise
     */
    bool RFM69_ReceiveDone(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Check if ACK was received
     * @param hrf Pointer to RFM69 handle
     * @param fromNodeID Expected sender node ID
     * @return true if ACK received from specified node, false otherwise
     */
    bool RFM69_ACKReceived(RFM69_HandleTypeDef *hrf, uint16_t fromNodeID);

    /**
     * @brief Check if ACK was requested in received packet
     * @param hrf Pointer to RFM69 handle
     * @return true if ACK was requested, false otherwise
     */
    bool RFM69_ACKRequested(RFM69_HandleTypeDef *hrf);

    /**
     * @brief Send acknowledgment packet
     * @param hrf Pointer to RFM69 handle
     * @param buffer Optional data to include in ACK
     * @param size Size of optional data
     */
    void RFM69_SendACK(RFM69_HandleTypeDef *hrf, const void *buffer, uint8_t size);

    /** @} */

    /** @defgroup RFM69_Interrupt Interrupt Handling
     * @{
     */

    /**
     * @brief DIO0 interrupt handler
     *
     * Call this function from HAL_GPIO_EXTI_Callback when DIO0 interrupt occurs.
     *
     * @param hrf Pointer to RFM69 handle
     */
    void RFM69_OnDIO0IRQ(RFM69_HandleTypeDef *hrf);

    /** @} */

#ifdef __cplusplus
}
#endif
#endif // RFM69_H
