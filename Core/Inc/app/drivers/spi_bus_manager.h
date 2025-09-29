#pragma once

#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_gpio.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @file spi_bus_manager.h
     * @brief DMA-based SPI arbitration (bus manager) for STM32 HAL.
     *
     * Features:
     * - Single SPI instance arbitrated among multiple devices (slaves).
     * - Transaction queue (ring buffer) with user-supplied storage (no malloc).
     * - Per-transaction CS and optional DC control.
     * - TX-only or TXRX DMA transfers.
     * - Optional post-transfer wait hook (e.g., wait on device BUSY pin).
     * - Completion, half-completion and error callbacks per transaction.
     * - HAL integration via spi_bus_manager_on_* functions called from HAL callbacks.
     *
     * Typical flow:
     *  1) Create manager with spi_bus_manager_create().
     *  2) Enqueue transactions with spi_bus_manager_submit().
     *  3) In HAL SPI callbacks, call spi_bus_manager_on_tx_cplt() / _on_txrx_cplt() / _on_error().
     *  4) (Optional) Poll spi_bus_manager_is_idle() or use callbacks to chain higher-level logic.
     */

    /* --------------------------------- Status --------------------------------- */

    typedef enum
    {
        SPI_BUS_MANAGER_OK = 0,
        SPI_BUS_MANAGER_ERR_PARAM = -1,
        SPI_BUS_MANAGER_ERR_FULL = -2,
        SPI_BUS_MANAGER_ERR_BUSY = -3,
        SPI_BUS_MANAGER_ERR_HAL = -4,
    } spi_bus_manager_status;

    /* ------------------------------- GPIO helpers ------------------------------ */

    typedef struct
    {
        GPIO_TypeDef *port;
        uint16_t pin;
        bool active_low; /**< true: active = LOW; false: active = HIGH */
    } spi_bus_gpio;

    /* ---------------------------- Transaction config --------------------------- */

    /**
     * @brief Data direction for a transaction.
     */
    typedef enum
    {
        SPI_BUS_DIR_TX = 0,   /**< Transmit only (TX DMA). */
        SPI_BUS_DIR_TXRX = 1, /**< Full-duplex (TXRX DMA).   */
    } spi_bus_direction;

    /**
     * @brief Optional DC line mode for a transaction.
     */
    typedef enum
    {
        SPI_BUS_DC_UNUSED = 0, /**< DC not used for this transaction.            */
        SPI_BUS_DC_COMMAND,    /**< Set DC=0 before transfer (command phase).    */
        SPI_BUS_DC_DATA        /**< Set DC=1 before transfer (data phase).       */
    } spi_bus_dc_mode;

    /* Forward decl for handle */
    struct spi_bus_manager;

    /**
     * @brief Post-transfer wait function (e.g., wait until device BUSY de-asserts).
     * Should return true when device is ready to accept a new transaction.
     * Called repeatedly until returns true or until timeout_ms elapses (if >0).
     *
     * @param user       User pointer provided in transaction.
     * @return bool      true = ready; false = still busy (manager will retry until timeout).
     */
    typedef bool (*spi_bus_wait_ready_fn)(void *user);

    /**
     * @brief Completion/half/error callback prototype.
     * Callbacks execute in ISR context (HAL DMA/SPI IRQ). Keep them short.
     *
     * @param mgr        Pointer to manager.
     * @param user       User pointer from transaction.
     */
    typedef void (*spi_bus_done_cb)(struct spi_bus_manager *mgr, void *user);

    /* ------------------------------ Transaction ------------------------------- */

    /**
     * @brief SPI transaction descriptor. Provide a const instance per submit.
     */
    typedef struct
    {
        /* Bus lines */
        spi_bus_gpio cs;         /**< Chip Select line (required). */
        spi_bus_gpio dc;         /**< Optional DC line; set .port=NULL if unused. */
        spi_bus_dc_mode dc_mode; /**< How to set DC before transfer. */

        /* SPI config snapshot (fast switch, no re-init). Fill only fields you need.
           Manager will write CR1/CR2 directly around SPE. */
        uint32_t cr1;
        uint32_t cr2;

        /* Buffers & sizes */
        const uint8_t *tx;     /**< TX buffer (required). */
        uint8_t *rx;           /**< RX buffer (required if dir=TXRX). */
        uint16_t len;          /**< Number of SPI data units (8-bit when DS=8, 16-bit when DS=16). */
        spi_bus_direction dir; /**< TX or TXRX. */

        /* Timeouts */
        uint32_t spi_timeout; /**< Fallback timeout (usually not used with DMA, keep HAL_MAX_DELAY). */

        /* Optional post-transfer wait (e.g., BUSY pin) */
        spi_bus_wait_ready_fn wait_ready; /**< Optional ready predicate; may be NULL. */
        uint32_t wait_timeout_ms;         /**< 0 = no timeout (avoid infinite if you can't guarantee). */

        /* Callbacks (ISR context) */
        spi_bus_done_cb on_half;  /**< Half-transfer callback (optional). */
        spi_bus_done_cb on_done;  /**< Transfer complete callback (optional). */
        spi_bus_done_cb on_error; /**< Error callback (optional). */

        /* User payload passed to callbacks and wait_ready */
        void *user;
    } spi_bus_transaction;

    /* --------------------------------- Handle --------------------------------- */

    typedef struct spi_bus_manager
    {
        SPI_HandleTypeDef *spi; /**< Bound SPI handle. */
        /* Queue */
        spi_bus_transaction *q;   /**< Ring buffer storage (user-supplied). */
        uint16_t q_capacity;      /**< Ring buffer capacity. */
        volatile uint16_t q_head; /**< Pop index. */
        volatile uint16_t q_tail; /**< Push index. */
        volatile bool busy;       /**< True while a DMA transfer is in progress or waiting post hook. */
        /* Cache management (for M7 etc.): set true to clean DCache before TX DMA */
        bool clean_dcache_before_tx;
    } spi_bus_manager;

    /* ------------------------------ Public API -------------------------------- */

    /**
     * @brief Create and initialize a bus manager.
     * @param spi                  SPI handle (initialized elsewhere).
     * @param storage              Pointer to transaction array for ring buffer.
     * @param capacity             Number of entries in @p storage.
     * @return spi_bus_manager     Initialized handle.
     */
    spi_bus_manager spi_bus_manager_create(SPI_HandleTypeDef *spi,
                                           spi_bus_transaction *storage,
                                           uint16_t capacity);

    /**
     * @brief Submit a transaction to the queue. Non-blocking.
     * @param mgr      Manager.
     * @param t        Transaction descriptor (contents copied by value).
     * @return SPI_BUS_MANAGER_OK on success; *_ERR_FULL if queue full; *_ERR_PARAM on invalid args.
     */
    spi_bus_manager_status spi_bus_manager_submit(spi_bus_manager *mgr, const spi_bus_transaction *t);

    /**
     * @brief Returns true if no transfer in progress and queue empty.
     */
    bool spi_bus_manager_is_idle(const spi_bus_manager *mgr);

    /**
     * @brief Cancel all pending transactions. Does not abort current DMA.
     *        Safe to call from thread-level only (not ISR).
     */
    void spi_bus_manager_cancel_pending(spi_bus_manager *mgr);

    /**
     * @brief Enable/disable D-Cache cleaning before TX DMA (for CM7 targets).
     *        On G4 this is a no-op but kept for API compatibility.
     */
    static inline void spi_bus_manager_set_clean_dcache(spi_bus_manager *mgr, bool enable)
    {
        mgr->clean_dcache_before_tx = enable;
    }

    /* -------------------------- HAL integration hooks ------------------------- */
    /**
     * @brief Call from HAL_SPI_TxCpltCallback()
     */
    void spi_bus_manager_on_tx_cplt(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi);

    /**
     * @brief Call from HAL_SPI_TxRxCpltCallback()
     */
    void spi_bus_manager_on_txrx_cplt(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi);

    /**
     * @brief Call from HAL_SPI_TxHalfCpltCallback().
     */
    void spi_bus_manager_on_tx_half(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi);

    /**
     * @brief Call from HAL_SPI_TxRxHalfCpltCallback().
     */
    void spi_bus_manager_on_txrx_half(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi);

    /**
     * @brief Call from HAL_SPI_ErrorCallback().
     */
    void spi_bus_manager_on_error(spi_bus_manager *mgr, SPI_HandleTypeDef *hspi);

    /* ------------------------------ Usage example ------------------------------
     *
     * // Storage
     * static spi_bus_transaction spiq_storage[8];
     * static spi_bus_manager spi_mgr;
     *
     * // Optional: device BUSY wait
     * static bool epd_wait_ready(void *user) {
     *     (void)user;
     *     // Example: busy is active-high -> wait until LOW
     *     extern GPIO_TypeDef* BUSY_PORT; extern uint16_t BUSY_PIN;
     *     return HAL_GPIO_ReadPin(BUSY_PORT, BUSY_PIN) == GPIO_PIN_RESET;
     * }
     *
     * void app_spi_init(void) {
     *     extern SPI_HandleTypeDef hspi1;
     *     spi_mgr = spi_bus_manager_create(&hspi1, spiq_storage, 8);
     * }
     *
     * // Enqueue a command and then a big data block for an EPD
     * void epd_send_frame(const uint8_t *cmd, uint16_t cmd_len,
     *                     const uint8_t *frame, uint16_t frame_len_bytes,
     *                     spi_bus_gpio cs, spi_bus_gpio dc)
     * {
     *     spi_bus_transaction t_cmd = {
     *         .cs = cs,
     *         .dc = dc,
     *         .dc_mode = SPI_BUS_DC_COMMAND,
     *         .cr1 = (hspi1.Instance->CR1 & ~0U), // reuse current CR1 or precompute per device
     *         .cr2 = (hspi1.Instance->CR2 & ~0U),
     *         .tx = cmd,
     *         .rx = NULL,
     *         .len = cmd_len,
     *         .dir = SPI_BUS_DIR_TX,
     *         .spi_timeout = HAL_MAX_DELAY,
     *         .wait_ready = NULL, .wait_timeout_ms = 0,
     *         .on_done = NULL, .on_half = NULL, .on_error = NULL,
     *         .user = NULL
     *     };
     *     spi_bus_manager_submit(&spi_mgr, &t_cmd);
     *
     *     spi_bus_transaction t_data = {
     *         .cs = cs,
     *         .dc = dc,
     *         .dc_mode = SPI_BUS_DC_DATA,
     *         .cr1 = t_cmd.cr1,
     *         .cr2 = t_cmd.cr2,
     *         .tx = frame,
     *         .rx = NULL,
     *         .len = frame_len_bytes, // with DS=8; if DS=16 then count in half-words
     *         .dir = SPI_BUS_DIR_TX,
     *         .spi_timeout = HAL_MAX_DELAY,
     *         .wait_ready = epd_wait_ready,      // wait BUSY after data
     *         .wait_timeout_ms = 12000,
     *         .on_done = NULL, .on_half = NULL, .on_error = NULL,
     *         .user = NULL
     *     };
     *     spi_bus_manager_submit(&spi_mgr, &t_data);
     * }
     *
     * // HAL glue (in your stm32xx_it.c or callbacks file):
     * void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)   { spi_bus_manager_on_tx_cplt(&spi_mgr, hspi); }
     * void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *h)  { spi_bus_manager_on_tx_half(&spi_mgr, h); }
     * void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) { spi_bus_manager_on_txrx_cplt(&spi_mgr, hspi); }
     * void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *h){ spi_bus_manager_on_txrx_half(&spi_mgr, h); }
     * void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)    { spi_bus_manager_on_error(&spi_mgr, hspi); }
     *
     * Notes:
     * - Prepare CR1/CR2 snapshots per slave (prescaler, CPOL/CPHA, DS=8/16 etc.) to minimize overhead.
     * - For large frames, just enqueue one big transaction; DMA handles the bulk. If you need chunking, enqueue multiple.
     * - If you need to know when "everything drained", poll spi_bus_manager_is_idle() or set per-transaction .on_done.
     * -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
