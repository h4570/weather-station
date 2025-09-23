#include "epd3in7_lvgl_adapter.h"
#include <string.h>

#define EPD3IN7_LVGL_ADAPTER_PALLETE_BYTES 8 // I1: 2 colors * 4 bytes (ARGB32)

static uint8_t epd3in7_lvgl_adapter_work_buffer[(EPD3IN7_WIDTH * EPD3IN7_HEIGHT) / 8];

/**
 * Rotate a 1bpp (I1) bitmap. Operates only on pixels (excludes the 8-byte palette).
 * src_w/src_h – dimensions of the source region in pixels.
 * src_stride  – bytes per source row (for I1: (src_w+7)/8).
 * dst_stride  – bytes per destination row (for I1: (dst_w+7)/8; for 90/270 dst_w = src_h).
 * rotation    – LV_DISPLAY_ROTATION_0/90/180/270.
 * Bit order: MSB-first.
 */
static inline uint8_t epd3in7_lvgl_adapter_i1_get_bit_msb_first(const uint8_t *row, int x)
{
    return (row[x >> 3] >> (7 - (x & 7))) & 0x01;
}
static inline void epd3in7_lvgl_adapter_i1_set_bit_msb_first(uint8_t *row, int x, uint8_t v)
{
    uint8_t mask = (uint8_t)(1u << (7 - (x & 7)));
    if (v)
        row[x >> 3] |= mask;
    else
        row[x >> 3] &= (uint8_t)~mask;
}

static void epd3in7_lvgl_adapter_rotate(const void *src, void *dst, int32_t src_w, int32_t src_h, int32_t src_stride,
                                        int32_t dst_stride, lv_display_rotation_t rotation)
{
    // Clear the destination buffer — we will only set bits to 1.
    // (The 0/1 values keep the palette meaning; we are just moving bits.)
    // Note: For 0/180, destination size = src_w x src_h; for 90/270 = src_h x src_w.
    int dst_w = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270) ? src_h : src_w;
    int dst_h = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270) ? src_w : src_h;
    memset(dst, 0x00, (size_t)dst_stride * (size_t)dst_h);

    if (rotation == LV_DISPLAY_ROTATION_0)
    {
        // Fast path: copy whole rows
        for (int y = 0; y < src_h; ++y)
        {
            memcpy(dst + (size_t)y * dst_stride, src + (size_t)y * src_stride, (size_t)src_stride);
        }
        return;
    }

    // General case: map coordinates (x,y) -> (x',y')
    for (int y = 0; y < src_h; ++y)
    {
        const uint8_t *src_row = src + (size_t)y * src_stride;
        for (int x = 0; x < src_w; ++x)
        {
            uint8_t bit = epd3in7_lvgl_adapter_i1_get_bit_msb_first(src_row, x);

            int xd, yd;
            switch (rotation)
            {
            case LV_DISPLAY_ROTATION_90:
                xd = src_h - 1 - y;
                yd = x;
                break;
            case LV_DISPLAY_ROTATION_180:
                xd = src_w - 1 - x;
                yd = src_h - 1 - y;
                break;
            case LV_DISPLAY_ROTATION_270:
                xd = y;
                yd = src_w - 1 - x;
                break;
            default:
                xd = x;
                yd = y; // should not happen
                break;
            }

            uint8_t *dst_row = dst + (size_t)yd * dst_stride;
            epd3in7_lvgl_adapter_i1_set_bit_msb_first(dst_row, xd, bit);
        }
    }
}

void epd3in7_lvgl_adapter_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    epd3in7_driver_handle *h = lv_display_get_driver_data(disp);

    if (!h || !area || !px_map)
    {
        lv_display_flush_ready(disp);
        return;
    }

    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

    // LVGL I1: first 8 bytes in LVGL buffer is palette (2 colors * 4 bytes ARGB32)
    // We need to skip it, because EPD3IN7 driver expects pure 1bpp data
    uint8_t *src = px_map + EPD3IN7_LVGL_ADAPTER_PALLETE_BYTES;

    if (rotation != LV_DISPLAY_ROTATION_0)
    {
        lv_color_format_t cf = lv_display_get_color_format(disp);

        lv_area_t rotated_area = *area;
        lv_display_rotate_area(disp, &rotated_area);

        uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
        uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);

        int32_t src_w = lv_area_get_width(area);
        int32_t src_h = lv_area_get_height(area);

        epd3in7_lvgl_adapter_rotate(src, epd3in7_lvgl_adapter_work_buffer, src_w, src_h, src_stride, dest_stride, rotation);

        area = &rotated_area;
        src = epd3in7_lvgl_adapter_work_buffer;
    }

    epd3in7_driver_display_1_gray(h, src, EPD3IN7_DRIVER_MODE_GC);

    lv_display_flush_ready(disp);
}
