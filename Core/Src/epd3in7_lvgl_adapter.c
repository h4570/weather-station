#include "epd3in7_lvgl_adapter.h"
#include <string.h>

static uint8_t adapter_buffer[(EPD3IN7_WIDTH * EPD3IN7_HEIGHT) / 8];

void epd3in7_lvgl_adapter_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    epd3in7_driver_handle *h = lv_display_get_driver_data(disp);

    if (!h || !area || !px_map)
    {
        lv_display_flush_ready(disp);
        return;
    }

    // LVGL I1: first 8 bytes are palette (ARGB32), we ignore them
    px_map += 8;

    // Organized map
    memcpy(adapter_buffer, px_map, sizeof(adapter_buffer));

    epd3in7_driver_display_1_gray(h, adapter_buffer, EPD3IN7_DRIVER_MODE_GC);

    lv_display_flush_ready(disp);
}