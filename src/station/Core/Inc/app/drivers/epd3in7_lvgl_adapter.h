#pragma once

#include "app/drivers/epd3in7_driver.h"
#include "shared/drivers/spi_bus_manager.h"
#include "lvgl/lvgl.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Handle structure for the e-Paper display (no change detection).
     */
    typedef struct
    {
        epd3in7_driver_handle *driver;    /**< Pointer to the e-Paper driver handle */
        uint8_t *work_buffer;             /**< Work buffer used e.g. for rotation (size: width * height / 8) */
        uint8_t refresh_counter;          /**< Refresh cycle counter (resets on GC) */
        bool is_initialized;              /**< Display initialization flag */
        bool is_sleeping;                 /**< Sleep state flag */
        int8_t refresh_cycles_before_gc;  /**< Number of cycles before forced GC (e.g. 10) */
        epd3in7_driver_mode default_mode; /**< Default partial mode between GC cycles: DU or A2 */

        /* ---- DMA / SPI bus manager (optional) ---- */
        spi_bus_manager *spi_mgr; /**< Optional SPI bus manager for DMA; NULL means "blocking HAL". */
        /* Cached GPIO roles for the manager (derived from driver pins). */
        spi_bus_gpio cs_gpio;          /**< CS line descriptor for bus manager (active_low = true). */
        spi_bus_gpio dc_gpio;          /**< DC line descriptor for bus manager (active_low = false). */
        volatile bool dma_in_progress; /**< true while a frame is enqueued and not yet completed */
        lv_display_t *pending_disp;    /**< display to call flush_ready for when DMA completes */
    } epd3in7_lvgl_adapter_handle;

    /**
     * @brief Create and initialize an e-Paper LVGL adapter handle (no change detection).
     *        This variant does not use spi_bus_manager (blocking HAL path).
     */
    epd3in7_lvgl_adapter_handle epd3in7_lvgl_adapter_create(epd3in7_driver_handle *driver,
                                                            uint8_t *work_buffer,
                                                            int8_t refresh_cycles_before_gc,
                                                            epd3in7_driver_mode default_mode);

    /**
     * @brief Create an e-Paper LVGL adapter handle with an SPI bus manager for DMA transfers.
     *        Initialization (epd init) stays blocking; frame transfers and sleep use DMA via manager.
     *
     * @param driver Pointer to the initialized e-Paper driver handle
     * @param work_buffer Pointer to a work buffer for rotation (size: width * height / 8 bytes)
     * @param refresh_cycles_before_gc Number of refresh cycles before forcing a GC refresh
     * @param default_mode Default refresh mode: EPD3IN7_DRIVER_MODE_A2 or EPD3IN7_DRIVER_MODE_DU
     * @param spi_mgr Pointer to a configured SPI bus manager (may not be NULL here)
     */
    epd3in7_lvgl_adapter_handle epd3in7_lvgl_adapter_create_with_bus_manager(epd3in7_driver_handle *driver,
                                                                             uint8_t *work_buffer,
                                                                             int8_t refresh_cycles_before_gc,
                                                                             epd3in7_driver_mode default_mode,
                                                                             spi_bus_manager *spi_mgr);

    /**
     * @brief Free resources associated with the e-Paper LVGL adapter handle.
     */
    void epd3in7_lvgl_adapter_free(epd3in7_lvgl_adapter_handle *handle);

    /**
     * @brief LVGL flush callback — blocking full-frame path (legacy HAL).
     */
    void epd3in7_lvgl_adapter_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

    /**
     * @brief LVGL flush callback — non-blocking full-frame path via SPI bus manager (DMA).
     *        If the panel is not initialized yet, waits until SPI is idle and runs blocking init,
     *        then enqueues display+sleep transactions via the manager and returns immediately.
     */
    void epd3in7_lvgl_adapter_flush_dma(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

    /**
     * @brief Convenience: query whether the adapter's SPI bus manager is currently idle.
     *        Returns true for legacy (blocking) handle with no manager.
     */
    static inline bool epd3in7_lvgl_adapter_is_idle(const epd3in7_lvgl_adapter_handle *h)
    {
        return (h && (!h->spi_mgr || spi_bus_manager_is_idle(h->spi_mgr)));
    }

#ifdef __cplusplus
}
#endif
