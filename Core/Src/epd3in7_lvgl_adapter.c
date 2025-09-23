#include "epd3in7_lvgl_adapter.h"
#include <string.h>

#define EPD3IN7_LVGL_ADAPTER_CHANGE_DETECTION_ROWS_PER_SECTION 32

// Simple, fast 32-bit FNV-1a hash for sector data
static uint32_t epd3in7_lvgl_adapter_hash32(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t h = 2166136261u;         // FNV offset basis
    const uint32_t prime = 16777619u; // FNV prime
    for (size_t i = 0; i < len; ++i)
    {
        h ^= p[i];
        h *= prime;
    }
    return h;
}

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

/**
 * @brief Split a 1bpp area buffer into vertical row-based sectors
 *
 * The buffer must point to the top-left pixel of `area` in 1bpp (I1) format,
 * with `stride` bytes per row (typically (width+7)/8). The function creates
 * about ceil(height / rows_per_sector) sectors. Each sector points into the
 * original buffer (no copy), and has an `lv_area_t` describing its absolute
 * screen rectangle.
 *
 * @param area             Area (absolute LVGL coordinates) that `buffer` represents
 * @param buffer           Pointer to 1bpp data for `area` (palette already skipped)
 * @param stride           Bytes per row of `buffer`
 * @param rows_per_sector  Preferred maximum number of rows in each sector (>=1)
 * @param out_list         Output sector list; caller must free with epd3in7_lvgl_adapter_sector_list_free()
 * @return true on success, false on invalid params or allocation failure
 */
static bool epd3in7_lvgl_adapter_make_sectors_rows(const lv_area_t *area,
                                                   const uint8_t *buffer,
                                                   uint32_t stride,
                                                   uint16_t rows_per_sector,
                                                   epd3in7_lvgl_adapter_sector_list *out_list)
{
    if (!area || !buffer || !out_list || rows_per_sector == 0)
        return false;

    const int32_t h = lv_area_get_height(area);
    const int32_t w = lv_area_get_width(area);
    if (h <= 0 || w <= 0)
        return false;

    uint16_t count = (uint16_t)((h + rows_per_sector - 1) / rows_per_sector);
    if (count == 0)
        return false;

    epd3in7_lvgl_adapter_sector *items = (epd3in7_lvgl_adapter_sector *)lv_malloc(sizeof(epd3in7_lvgl_adapter_sector) * count);
    if (!items)
        return false;

    for (uint16_t i = 0; i < count; ++i)
    {
        int32_t y_off = (int32_t)i * (int32_t)rows_per_sector;
        int32_t rows = rows_per_sector;
        if (y_off + rows > h)
            rows = h - y_off;

        epd3in7_lvgl_adapter_sector *s = &items[i];
        s->data = buffer + (size_t)stride * (size_t)y_off;
        s->area.x1 = area->x1;
        s->area.x2 = area->x2;
        s->area.y1 = area->y1 + y_off;
        s->area.y2 = s->area.y1 + rows - 1;
        s->hash = epd3in7_lvgl_adapter_hash32(s->data, (size_t)rows * (size_t)stride);
    }

    out_list->items = items;
    out_list->count = count;
    return true;
}

/**
 * @brief Free a list produced by epd3in7_lvgl_adapter_make_sectors_rows
 */
static void epd3in7_lvgl_adapter_sector_list_free(epd3in7_lvgl_adapter_sector_list *list)
{
    if (!list)
        return;

    if (list->items)
    {
        lv_free(list->items);
        list->items = NULL;
    }

    list->count = 0;
}

epd3in7_lvgl_adapter_handle epd3in7_lvgl_adapter_create(epd3in7_driver_handle *driver, uint8_t *work_buffer)
{
    epd3in7_lvgl_adapter_handle handle;
    handle.driver = driver;
    handle.work_buffer = work_buffer;
    handle.current_sectors.items = NULL;
    handle.current_sectors.count = 0;
    return handle;
}

void epd3in7_lvgl_adapter_free(epd3in7_lvgl_adapter_handle *handle)
{
    if (!handle)
        return;

    epd3in7_lvgl_adapter_sector_list_free(&handle->current_sectors);
}

void epd3in7_lvgl_adapter_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    epd3in7_lvgl_adapter_handle *h = lv_display_get_driver_data(disp);

    if (!h || !area || !px_map)
    {
        lv_display_flush_ready(disp);
        return;
    }

    // I1: 2 colors * 4 bytes (ARGB32)
    static uint8_t palette_bytes = 8;
    lv_area_t rotated_area;

    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

    // LVGL I1: first 8 bytes in LVGL buffer is palette (2 colors * 4 bytes ARGB32)
    // We need to skip it, because EPD3IN7 driver expects pure 1bpp data
    uint8_t *src = px_map + palette_bytes;

    if (rotation != LV_DISPLAY_ROTATION_0)
    {
        lv_color_format_t cf = lv_display_get_color_format(disp);

        rotated_area = *area;
        lv_display_rotate_area(disp, &rotated_area);

        uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
        uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);

        int32_t src_w = lv_area_get_width(area);
        int32_t src_h = lv_area_get_height(area);

        epd3in7_lvgl_adapter_rotate(src, h->work_buffer, src_w, src_h, src_stride, dest_stride, rotation);

        area = &rotated_area;
        src = h->work_buffer;
    }

    { // Change detection
        lv_color_format_t cf = lv_display_get_color_format(disp);
        uint32_t stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);

        if (h->current_sectors.items != NULL)
        {
            // Free previous sectors
            epd3in7_lvgl_adapter_sector_list_free(&h->current_sectors);
        }

        if (epd3in7_lvgl_adapter_make_sectors_rows(area, src, stride, EPD3IN7_LVGL_ADAPTER_CHANGE_DETECTION_ROWS_PER_SECTION, &h->current_sectors))
        {
        }
    }

    epd3in7_driver_display_1_gray(h->driver, src, EPD3IN7_DRIVER_MODE_GC);

    // epd3in7_driver_display_1_gray_top

    lv_display_flush_ready(disp);
}
