/*****************************************************************************
 * | File          :   epd3in7_driver.h
 * | Author        :   h4570 (based on Waveshare code)
 * | Function      :   quasi-async alternative driver for official 3.7inch e-paper display (Waveshare 20123)
 *----------------
 * | Version       :   V1.0
 * | Date          :   2025-09-19
 ******************************************************************************/

#pragma once

#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_gpio.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "app/drivers/spi_bus_manager.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Display resolution
 */
#define EPD3IN7_WIDTH 280
#define EPD3IN7_HEIGHT 480

/**
 * @brief Timeouts
 */
#define EPD3IN7_DRIVER_BUSY_TIMEOUT 12000
#define EPD3IN7_DRIVER_SPI_TIMEOUT HAL_MAX_DELAY

    /**
     * @brief Display modes
     */
    typedef enum
    {
        EPD3IN7_DRIVER_MODE_GC = 1, /**< Global clear, full refresh */
        EPD3IN7_DRIVER_MODE_DU = 2, /**< Direct update (partial refresh) */
        EPD3IN7_DRIVER_MODE_A2 = 3  /**< Fast, but lower quality partial refresh */
    } epd3in7_driver_mode;

    /**
     * @brief Look-up table types
     */
    typedef enum
    {
        EPD3IN7_DRIVER_LUT_4_GRAY_GC = 0, /**< 4-level grayscale, global clear */
        EPD3IN7_DRIVER_LUT_1_GRAY_GC = 1, /**< 1-bit (black & white), global clear */
        EPD3IN7_DRIVER_LUT_1_GRAY_DU = 2, /**< 1-bit (black & white), direct update */
        EPD3IN7_DRIVER_LUT_1_GRAY_A2 = 3  /**< 1-bit (black & white), A2 fast mode */
    } epd3in7_driver_lut_type;

    /**
     * @brief Sleep modes
     */
    typedef enum
    {
        EPD3IN7_DRIVER_SLEEP_DEEP = 0,  /**< Deep sleep mode, lowest power consumption (about 5µA). Data is lost and a full reset is needed. */
        EPD3IN7_DRIVER_SLEEP_NORMAL = 1 /**< Normal sleep mode, higher power consumption (about 20µA). Data is retained and no reset is needed. */
    } epd3in7_driver_sleep_mode;

    /**
     * @brief Status codes
     */
    typedef enum
    {
        EPD3IN7_DRIVER_OK = 0,           /**< Operation completed successfully */
        EPD3IN7_DRIVER_ERR_HAL = -1,     /**< HAL error occurred */
        EPD3IN7_DRIVER_ERR_TIMEOUT = -2, /**< Operation timed out */
        EPD3IN7_DRIVER_ERR_PARAM = -3,   /**< Invalid parameter provided */
        EPD3IN7_DRIVER_SPI_BUS_ERR = -4, /**< SPI bus manager error */
    } epd3in7_driver_status;

    /**
     * @brief GPIO pin configuration for the e-Paper display
     */
    typedef struct
    {
        GPIO_TypeDef *reset_port; /**< RESET pin port */
        uint16_t reset_pin;       /**< RESET pin number */

        GPIO_TypeDef *dc_port; /**< DC pin port (Data/Command control) */
        uint16_t dc_pin;       /**< DC pin number */

        GPIO_TypeDef *busy_port; /**< BUSY pin port */
        uint16_t busy_pin;       /**< BUSY pin number */

        GPIO_TypeDef *cs_port; /**< CS pin port (Chip Select) */
        uint16_t cs_pin;       /**< CS pin number */
    } epd3in7_driver_pins;

    /**
     * @brief Handle structure for the e-Paper display
     */
    typedef struct
    {
        uint16_t width;                   /**< Display width */
        uint16_t height;                  /**< Display height */
        epd3in7_driver_pins pins;         /**< GPIO pin configuration */
        SPI_HandleTypeDef *spi_handle;    /**< SPI handle */
        bool busy_active_high;            /**< BUSY pin polarity: 0 = active low (wait for low), 1 = active high (wait for high) */
        bool is_cs_low;                   /**< Flag indicating CS pin state */
        bool is_cs_low_has_value;         /**< Flag indicating CS pin state is defined */
        epd3in7_driver_lut_type last_lut; /**< Last LUT type that was sent */
        bool last_lut_has_value;          /**< Flag indicating if LUT was already sent */
    } epd3in7_driver_handle;

    /**
     * @brief Create the e-Paper display handle with given pin configuration
     *
     * @param pins The GPIO pin configuration
     * @param spi_handle Handle to the SPI peripheral
     * @param busy_active_high BUSY pin polarity (0 = active low, 1 = active high)
     * @return epd3in7_driver_handle Handle for the e-Paper display
     */
    epd3in7_driver_handle epd3in7_driver_create(const epd3in7_driver_pins pins, SPI_HandleTypeDef *spi_handle, const bool busy_active_high);

    /**
     * @brief Check if the display is currently busy
     *
     * @param handle Pointer to the e-Paper display handle
     * @return true Display is busy
     * @return false Display is idle
     */
    bool epd3in7_driver_is_busy(const epd3in7_driver_handle *handle);

    /**
     * @brief Wait until the display is no longer busy (wait for draw to finish)
     *
     * @param handle Pointer to the e-Paper display handle
     * @return epd3in7_driver_status Operation status
     */
    epd3in7_driver_status epd3in7_driver_busy_wait_for_idle(const epd3in7_driver_handle *handle);

    /**
     * @brief Enter sleep mode to conserve power and protect the display
     *
     * @param handle Pointer to the e-Paper display handle
     * @return epd3in7_driver_status Operation status
     *
     * @note IMPORTANT: The display cannot be powered on for extended periods.
     * When the display is not being refreshed, always put it into sleep mode or power it off.
     * Otherwise, the display will remain in a high voltage state for a long time,
     * which will permanently damage the e-Paper and cannot be repaired!
     * Note, You should only hardware reset or use initialize function to wake up e-Paper from sleep mode.
     */
    epd3in7_driver_status epd3in7_driver_sleep(epd3in7_driver_handle *handle, const epd3in7_driver_sleep_mode mode);

    /**
     * @brief Perform a hardware reset of the e-Paper display
     *
     * @param handle Pointer to the e-Paper display handle
     */
    void epd3in7_driver_reset(const epd3in7_driver_handle *handle);

    /* ---- 4-Gray Level Functions ---- */

    /**
     * @brief Initialize the e-Paper display for 4-gray level operation
     *
     * @param handle Pointer to the e-Paper display handle
     * @return epd3in7_driver_status Operation status
     */
    epd3in7_driver_status epd3in7_driver_init_4_gray(epd3in7_driver_handle *handle);

    /**
     * @brief Clear screen using 4-gray level mode (GC LUT)
     *
     * @param handle Pointer to the e-Paper display handle
     * @return epd3in7_driver_status Operation status
     */
    epd3in7_driver_status epd3in7_driver_clear_4_gray(epd3in7_driver_handle *handle);

    /**
     * @brief Send the 4-gray level image buffer to e-Paper and refresh the display
     *
     * @param handle Pointer to the e-Paper display handle
     * @param image Pointer to the 4-gray level image buffer
     * @return epd3in7_driver_status Operation status
     */
    epd3in7_driver_status epd3in7_driver_display_4_gray(epd3in7_driver_handle *handle, const uint8_t *image);

    /* ---- 1-Gray Level (Black & White) Functions ---- */

    /**
     * @brief Initialize the e-Paper display for 1-gray level (black & white) operation
     *
     * @param handle Pointer to the e-Paper display handle
     * @return epd3in7_driver_status Operation status
     */
    epd3in7_driver_status epd3in7_driver_init_1_gray(epd3in7_driver_handle *handle);

    /**
     * @brief Clear screen using 1-gray level (black & white) mode
     *
     * @param handle Pointer to the e-Paper display handle
     * @param mode Refresh mode:
     *             - EPD3IN7_DRIVER_MODE_GC: Global clear (full refresh)
     *             - EPD3IN7_DRIVER_MODE_DU: Direct update (partial refresh)
     *             - EPD3IN7_DRIVER_MODE_A2: Fast but lower quality partial refresh
     * @return epd3in7_driver_status Operation status
     */
    epd3in7_driver_status epd3in7_driver_clear_1_gray(epd3in7_driver_handle *handle, const epd3in7_driver_mode mode);

    /**
     * @brief Send the 1-gray level (black & white) image buffer to e-Paper and refresh the display
     *
     * @param handle Pointer to the e-Paper display handle
     * @param image Pointer to the 1-gray level image buffer
     * @param mode Refresh mode:
     *             - EPD3IN7_DRIVER_MODE_GC: Global clear (full refresh)
     *             - EPD3IN7_DRIVER_MODE_DU: Direct update (partial refresh)
     *             - EPD3IN7_DRIVER_MODE_A2: Fast but lower quality partial refresh
     * @return epd3in7_driver_status Operation status
     */
    epd3in7_driver_status epd3in7_driver_display_1_gray(epd3in7_driver_handle *handle, const uint8_t *image, const epd3in7_driver_mode mode);

    /**
     * @brief Send the top part of the 1-gray level (black & white) image buffer to e-Paper
     *        and refresh the display
     *
     * @param handle Pointer to the e-Paper display handle
     * @param image Pointer to the 1-gray level image buffer
     * @param y_end_exclusive The exclusive Y-coordinate endpoint (height) of the area to update
     * @param mode Refresh mode:
     *             - EPD3IN7_DRIVER_MODE_GC: Global clear (full refresh)
     *             - EPD3IN7_DRIVER_MODE_DU: Direct update (partial refresh)
     *             - EPD3IN7_DRIVER_MODE_A2: Fast but lower quality partial refresh
     * @return epd3in7_driver_status Operation status
     *
     * @note Important usage guidelines:
     * - You cannot refresh using partial refresh mode all the time.
     *   After refreshing partially several times, you need to fully refresh the display once.
     *   Otherwise, the display quality will deteriorate and cannot be recovered!
     *
     * - Before A2 partial update, you must run at least once a full width/height background
     *   image in DU mode. You can use any of the DU display functions for this:
     *   `epd3in7_driver_display_1_gray()` or `epd3in7_driver_display_1_gray_top()`.
     *   Otherwise, after partial update the rest of the image will be blank.
     *
     * - This function should not be called after epd3in7_driver_clear_1_gray() without a full epd3in7_driver_display_1_gray() in between.
     */
    epd3in7_driver_status epd3in7_driver_display_1_gray_top(epd3in7_driver_handle *handle, const uint8_t *image, const uint16_t y_end_exclusive, const epd3in7_driver_mode mode);

    // === DMA / SPI BUS MANAGER DECLARATIONS ===

    /**
     * @brief Send the 1-gray level (black & white) image buffer via DMA using spi_bus_manager.
     *        Non-blocking: the function only enqueues transactions and returns.
     *
     * @param handle Driver handle
     * @param mgr    SPI bus manager (must be configured for the same SPI)
     * @param image  Pointer to I1 full-frame buffer (size: WIDTH*HEIGHT/8)
     * @param mode   GC / DU / A2
     * @return epd3in7_driver_status Operation status (enqueue-time only)
     */
    epd3in7_driver_status epd3in7_driver_display_1_gray_dma(epd3in7_driver_handle *handle,
                                                            spi_bus_manager *mgr,
                                                            const uint8_t *image,
                                                            const epd3in7_driver_mode mode);

    /**
     * @brief Put the display to sleep using DMA transactions (non-blocking).
     *        Enqueued after display update to protect the panel.
     */
    epd3in7_driver_status epd3in7_driver_sleep_dma(epd3in7_driver_handle *handle,
                                                   spi_bus_manager *mgr,
                                                   const epd3in7_driver_sleep_mode mode);

#ifdef __cplusplus
}
#endif