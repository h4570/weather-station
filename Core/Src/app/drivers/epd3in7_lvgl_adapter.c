#include "app/drivers/epd3in7_lvgl_adapter.h"
#include <string.h>

/* ---- 1bpp helpers for MSB-first bit access (used by the rotator) ---- */

static inline uint8_t epd3in7_lvgl_adapter_i1_get_bit_msb_first(const uint8_t *row, int x)
{
    return (row[x >> 3] >> (7 - (x & 7))) & 0x01;
}

static inline void epd3in7_lvgl_adapter_i1_set_bit_msb_first(uint8_t *row, int x, uint8_t v)
{
    const uint8_t mask = (uint8_t)(1u << (7 - (x & 7)));
    if (v)
        row[x >> 3] |= mask;
    else
        row[x >> 3] &= (uint8_t)~mask;
}

/**
 * @brief Rotate a 1bpp (I1) bitmap. Operates only on pixels.
 * src_w/src_h – source size in pixels.
 * src_stride  – bytes per source row ((src_w+7)/8).
 * dst_stride  – bytes per destination row.
 * rotation    – LV_DISPLAY_ROTATION_0/90/180/270.
 * Bit order   – MSB-first.
 */
static void epd3in7_lvgl_adapter_rotate_i1(const uint8_t *src, uint8_t *dst,
                                           int32_t src_w, int32_t src_h,
                                           int32_t src_stride, int32_t dst_stride,
                                           lv_display_rotation_t rotation)
{
    /* Clear destination — we only set bits. */
    const int dst_h = (rotation == LV_DISPLAY_ROTATION_90 || rotation == LV_DISPLAY_ROTATION_270) ? src_w : src_h;
    memset(dst, 0x00, (size_t)dst_stride * (size_t)dst_h);

    if (rotation == LV_DISPLAY_ROTATION_0)
    {
        /* Fast path: copy rows as-is. */
        for (int y = 0; y < src_h; ++y)
        {
            memcpy(dst + (size_t)y * dst_stride, src + (size_t)y * src_stride, (size_t)src_stride);
        }
        return;
    }

    for (int y = 0; y < src_h; ++y)
    {
        const uint8_t *src_row = src + (size_t)y * src_stride;
        for (int x = 0; x < src_w; ++x)
        {
            const uint8_t bit = epd3in7_lvgl_adapter_i1_get_bit_msb_first(src_row, x);
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
                yd = y;
                break;
            }
            uint8_t *dst_row = dst + (size_t)yd * dst_stride;
            epd3in7_lvgl_adapter_i1_set_bit_msb_first(dst_row, xd, bit);
        }
    }
}

/* ---- Public API ---- */

epd3in7_lvgl_adapter_handle epd3in7_lvgl_adapter_create(epd3in7_driver_handle *driver,
                                                        uint8_t *work_buffer,
                                                        int8_t refresh_cycles_before_gc,
                                                        epd3in7_driver_mode default_mode)
{
    epd3in7_lvgl_adapter_handle h;
    h.driver = driver;
    h.work_buffer = work_buffer;
    h.refresh_counter = 99; /* Force GC on first use */
    h.is_initialized = false;
    h.is_sleeping = false;
    h.refresh_cycles_before_gc = refresh_cycles_before_gc;
    h.default_mode = default_mode;
    return h;
}

void epd3in7_lvgl_adapter_free(epd3in7_lvgl_adapter_handle *handle)
{
    if (!handle)
        return;

    /* Put display to sleep before freeing the handle. */
    if (handle->is_initialized)
    {
        epd3in7_driver_sleep(handle->driver, EPD3IN7_DRIVER_SLEEP_NORMAL);
        handle->is_sleeping = true;
        handle->is_initialized = false;
    }
}

/**
 * @note This implementation assumes LVGL provides I1 pixel data (no palette header)
 *       and the e-paper driver accepts a full-frame 1bpp buffer.
 */
void epd3in7_lvgl_adapter_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    epd3in7_lvgl_adapter_handle *h = lv_display_get_driver_data(disp);
    if (!h || !px_map)
    {
        lv_display_flush_ready(disp);
        return;
    }

    /* Initialize display on first use (1-gray pipeline). */
    if (!h->is_initialized)
    {
        if (epd3in7_driver_init_1_gray(h->driver) != EPD3IN7_DRIVER_OK)
        {
            lv_display_flush_ready(disp);
            return;
        }
        h->refresh_counter = 99; /* Force GC on first transfer */
        h->is_initialized = true;
        h->is_sleeping = false;
    }

    /* Choose refresh mode:
     * - Every Nth cycle -> GC
     * - Otherwise -> default_mode (A2 or DU)
     */
    epd3in7_driver_mode mode;
    if (h->refresh_counter >= (uint8_t)(h->refresh_cycles_before_gc - 1))
    {
        mode = EPD3IN7_DRIVER_MODE_GC;
        h->refresh_counter = 0;
    }
    else
    {
        mode = h->default_mode;
        h->refresh_counter++;
    }

    /* Handle rotation if LVGL display rotation is set. We always send a full frame.
     * For FULL mode LVGL typically passes 'area' spanning the whole screen, but we
     * don't rely on that: we derive sizes from 'area' for correct rotation geometry.
     */
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);
    lv_color_format_t cf = lv_display_get_color_format(disp);

    /* Expect I1; compute strides for the given geometry. */
    const lv_area_t *src_area = area;
    lv_area_t rotated_area;

    const int32_t src_w = lv_area_get_width(src_area);
    const int32_t src_h = lv_area_get_height(src_area);
    const uint32_t src_stride = lv_draw_buf_width_to_stride(src_w, cf);

    const uint8_t *src = px_map;

    if (rotation != LV_DISPLAY_ROTATION_0)
    {
        /* Rotate the incoming buffer into work_buffer. Destination geometry depends on rotation. */
        rotated_area = *src_area;
        lv_display_rotate_area(disp, &rotated_area);

        const int32_t dst_w = lv_area_get_width(&rotated_area);
        const uint32_t dst_stride = lv_draw_buf_width_to_stride(dst_w, cf);

        /* Require a valid work buffer for rotation. */
        if (!h->work_buffer)
        {
            /* If no work buffer was provided, fail gracefully with GC to keep panel consistent. */
            (void)epd3in7_driver_display_1_gray(h->driver, src, EPD3IN7_DRIVER_MODE_GC);
            (void)epd3in7_driver_sleep(h->driver, EPD3IN7_DRIVER_SLEEP_NORMAL);
            h->is_sleeping = true;
            lv_display_flush_ready(disp);
            return;
        }

        epd3in7_lvgl_adapter_rotate_i1(src, h->work_buffer, src_w, src_h, (int32_t)src_stride, (int32_t)dst_stride, rotation);
        src = h->work_buffer;
    }

    /* Send the full-frame buffer in the selected mode. */
    if (epd3in7_driver_display_1_gray(h->driver, (uint8_t *)src, mode) != EPD3IN7_DRIVER_OK)
    {
        /* Mark as not initialized to force re-init on next flush. */
        h->is_initialized = false;
    }

    /* Put display to sleep after each refresh to protect the panel. */
    if (epd3in7_driver_sleep(h->driver, EPD3IN7_DRIVER_SLEEP_NORMAL) != EPD3IN7_DRIVER_OK)
    {
        h->is_initialized = false;
    }
    h->is_sleeping = true;

    lv_display_flush_ready(disp);
}
