#pragma once

#include "app/drivers/epd3in7_driver.h"
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
    } epd3in7_lvgl_adapter_handle;

    /**
     * @brief Create and initialize an e-Paper LVGL adapter handle (no change detection).
     * @param driver Pointer to the initialized e-Paper driver handle
     * @param work_buffer Pointer to a work buffer for rotation (size: width * height / 8 bytes)
     * @param refresh_cycles_before_gc Number of refresh cycles before forcing a GC refresh
     * @param default_mode Default refresh mode: EPD3IN7_DRIVER_MODE_A2 or EPD3IN7_DRIVER_MODE_DU
     * @return epd3in7_lvgl_adapter_handle The initialized adapter handle
     */
    epd3in7_lvgl_adapter_handle epd3in7_lvgl_adapter_create(epd3in7_driver_handle *driver,
                                                            uint8_t *work_buffer,
                                                            int8_t refresh_cycles_before_gc,
                                                            epd3in7_driver_mode default_mode);

    /**
     * @brief Free resources associated with the e-Paper LVGL adapter handle.
     * @param handle Pointer to the adapter handle to be freed
     */
    void epd3in7_lvgl_adapter_free(epd3in7_lvgl_adapter_handle *handle);

    /**
     * @brief LVGL flush callback — always sends the full frame to the display.
     * @param disp Pointer to the LVGL display object
     * @param area Area to flush (ignored for transfer size; used only for rotation geometry)
     * @param px_map Pointer to the pixel map buffer (I1 format expected)
     */
    void epd3in7_lvgl_adapter_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

#ifdef __cplusplus
}
#endif
