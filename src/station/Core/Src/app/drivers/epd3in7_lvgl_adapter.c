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

    /* Legacy path: no bus manager */
    h.spi_mgr = NULL;
    h.cs_gpio = (spi_bus_gpio){0};
    h.dc_gpio = (spi_bus_gpio){0};
    return h;
}

epd3in7_lvgl_adapter_handle epd3in7_lvgl_adapter_create_with_bus_manager(epd3in7_driver_handle *driver,
                                                                         uint8_t *work_buffer,
                                                                         int8_t refresh_cycles_before_gc,
                                                                         epd3in7_driver_mode default_mode,
                                                                         spi_bus_manager *spi_mgr)
{
    epd3in7_lvgl_adapter_handle h = epd3in7_lvgl_adapter_create(driver, work_buffer,
                                                                refresh_cycles_before_gc, default_mode);
    h.spi_mgr = spi_mgr;

    /* Map driver pins to spi_bus_gpio roles. */
    h.cs_gpio.port = driver->pins.cs_port;
    h.cs_gpio.pin = driver->pins.cs_pin;
    h.cs_gpio.active_low = true; /* CS active low */

    h.dc_gpio.port = driver->pins.dc_port;
    h.dc_gpio.pin = driver->pins.dc_pin;
    h.dc_gpio.active_low = false; /* DC=0 -> command, DC=1 -> data */

    h.dma_in_progress = false;
    h.pending_disp = NULL;

    return h;
}

void epd3in7_lvgl_adapter_free(epd3in7_lvgl_adapter_handle *handle)
{
    if (!handle)
        return;

    /* If a DMA frame is still pending, wait for bus idle to avoid cutting power mid-transfer. */
    if (handle->spi_mgr)
    {
        while (!spi_bus_manager_is_idle(handle->spi_mgr))
        { /* busy wait */
        }
    }

    /* Put display to sleep before freeing the handle. */
    if (handle->is_initialized)
    {
        if (handle->spi_mgr)
        {
            (void)epd3in7_driver_sleep_dma(handle->driver, handle->spi_mgr, EPD3IN7_DRIVER_SLEEP_NORMAL);
        }
        else
        {
            (void)epd3in7_driver_sleep(handle->driver, EPD3IN7_DRIVER_SLEEP_NORMAL);
        }
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

    static uint8_t palette_bytes = 8;

    // LVGL I1: first 8 bytes in LVGL buffer is palette (2 colors * 4 bytes ARGB32)
    // We need to skip it, because EPD3IN7 driver expects pure 1bpp data
    uint8_t *src = px_map + palette_bytes;

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

/* ---- Safe handoff to LVGL context ---- */
static void epd3in7_lvgl_adapter_lvgl_flush_ready_async(void *p)
{
    lv_display_t *disp = (lv_display_t *)p;
    if (disp)
        lv_display_flush_ready(disp);
}

/* Adapter's internal completion (user-level) */
static void epd3in7_lvgl_adapter_dma_done_cb(void *user)
{
    epd3in7_lvgl_adapter_handle *h = (epd3in7_lvgl_adapter_handle *)user;
    if (!h)
        return;
    h->dma_in_progress = false;

    /* Hand-off to LVGL context (never call LVGL directly from ISR) */
    lv_async_call(epd3in7_lvgl_adapter_lvgl_flush_ready_async, h->pending_disp);
}

/* Wrapper to match spi_bus_done_cb signature (mgr, user) */
// NOTE: keep ISR-safe and very short.
static void epd3in7_lvgl_adapter_dma_done_cb_mgr(struct spi_bus_manager *mgr, void *user)
{
    (void)mgr; /* unused */
    epd3in7_lvgl_adapter_dma_done_cb(user);
}

void epd3in7_lvgl_adapter_flush_dma(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    epd3in7_lvgl_adapter_handle *h = lv_display_get_driver_data(disp);
    if (!h || !px_map)
    {
        lv_display_flush_ready(disp);
        return;
    }

    if (!h->spi_mgr)
    {
        /* Safety net: if manager not provided, fallback to legacy path. */
        epd3in7_lvgl_adapter_flush(disp, area, px_map);
        return;
    }

    /* Ensure panel is initialized once (blocking init on first use). */
    if (!h->is_initialized)
    {
        /* Wait for the SPI bus to become idle before a blocking init. */
        while (!spi_bus_manager_is_idle(h->spi_mgr))
        { /* busy wait */
        }
        if (epd3in7_driver_init_1_gray(h->driver) != EPD3IN7_DRIVER_OK)
        {
            lv_display_flush_ready(disp);
            return;
        }
        h->refresh_counter = 99; /* Force GC on first transfer */
        h->is_initialized = true;
        h->is_sleeping = false;
    }

    /* Decide refresh mode (GC vs A2/DU) */
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

    lv_display_rotation_t rotation = lv_display_get_rotation(disp);
    lv_color_format_t cf = lv_display_get_color_format(disp);

    const lv_area_t *src_area = area;
    lv_area_t rotated_area;

    const int32_t src_w = lv_area_get_width(src_area);
    const int32_t src_h = lv_area_get_height(src_area);
    const uint32_t src_stride = lv_draw_buf_width_to_stride(src_w, cf);

    static uint8_t palette_bytes = 8;

    // LVGL I1: first 8 bytes in LVGL buffer is palette (2 colors * 4 bytes ARGB32)
    // We need to skip it, because EPD3IN7 driver expects pure 1bpp data
    uint8_t *src = px_map + palette_bytes;

    if (!h->work_buffer)
    {
        /* No work buffer -> we cannot DMA safely (source may vanish). Fallback to blocking path. */
        epd3in7_lvgl_adapter_flush(disp, area, px_map);
        return;
    }

    if (rotation != LV_DISPLAY_ROTATION_0)
    {
        rotated_area = *src_area;
        lv_display_rotate_area(disp, &rotated_area);
        const int32_t dst_w = lv_area_get_width(&rotated_area);
        const uint32_t dst_stride = lv_draw_buf_width_to_stride(dst_w, cf);

        epd3in7_lvgl_adapter_rotate_i1(src, h->work_buffer, src_w, src_h,
                                       (int32_t)src_stride, (int32_t)dst_stride, rotation);
    }
    else
    {
        /* Fast path: full-frame memcpy into work_buffer */
        const uint32_t frame_bytes = (uint32_t)src_stride * (uint32_t)src_h;
        memcpy(h->work_buffer, src, frame_bytes);
    }

    /* Enqueue frame (non-blocking) + sleep afterwards. */
    (void)epd3in7_driver_display_1_gray_dma(h->driver, h->spi_mgr, (const uint8_t *)h->work_buffer, mode);
    (void)epd3in7_driver_sleep_dma(h->driver, h->spi_mgr, EPD3IN7_DRIVER_SLEEP_NORMAL);
    h->is_sleeping = true;

    /* ---- Register completion callback AFTER enqueuing last txn ---- */
    h->dma_in_progress = true;
    h->pending_disp = disp;
    spi_bus_manager_enqueue_callback(h->spi_mgr,
                                     epd3in7_lvgl_adapter_dma_done_cb_mgr,
                                     (void *)h);
}