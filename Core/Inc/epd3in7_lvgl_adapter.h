#pragma once

#include "epd3in7_driver.h"
#include "lvgl/lvgl.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief One display sector: data pointer + screen area
     */
    typedef struct
    {
        const uint8_t *data; /**< Pointer to the start of this sector's pixel data (I1 format) */
        lv_area_t area;      /**< Absolute screen area covered by this sector */
        uint32_t hash;       /**< Hash of the data for change detection */
    } epd3in7_lvgl_adapter_sector;

    /**
     * @brief Dynamically allocated list of sectors
     */
    typedef struct
    {
        epd3in7_lvgl_adapter_sector *items; /**< Array of sectors */
        uint16_t count;                     /**< Number of valid items in the array */
    } epd3in7_lvgl_adapter_sector_list;

    /**
     * @brief Handle structure for the e-Paper display
     */
    typedef struct
    {
        epd3in7_driver_handle *driver;                    /**< Pointer to the e-Paper driver handle */
        uint8_t *work_buffer;                             /**< Pointer to the work buffer for image processing */
        epd3in7_lvgl_adapter_sector_list current_sectors; /**< Currently rendered, active sectors (for change detection) */
    } epd3in7_lvgl_adapter_handle;

    /**
     * @brief Create and initialize an e-Paper LVGL adapter handle
     * @param driver Pointer to the initialized e-Paper driver handle
     * @param work_buffer Pointer to a work buffer for image processing (size: width * height / 8 bytes)
     * @return epd3in7_lvgl_adapter_handle The initialized adapter handle
     */
    epd3in7_lvgl_adapter_handle epd3in7_lvgl_adapter_create(epd3in7_driver_handle *driver, uint8_t *work_buffer);

    /**
     * @brief Free resources associated with the e-Paper LVGL adapter handle (currently `current_sectors` only)
     * @param handle Pointer to the adapter handle to be freed
     */
    void epd3in7_lvgl_adapter_free(epd3in7_lvgl_adapter_handle *handle);

    /**
     * @brief Flush the given area of the display with the provided pixel map
     *
     * This function is intended to be used as the flush callback for LVGL display driver.
     * It converts the LVGL pixel map to a format suitable for the e-Paper display and sends it.
     *
     * @param disp Pointer to the LVGL display object
     * @param area Pointer to the area to be flushed (in LVGL coordinates)
     * @param px_map Pointer to the pixel map buffer (in LVGL format)
     */
    void epd3in7_lvgl_adapter_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

#ifdef __cplusplus
}
#endif
