
#include "app/display.h"
#include "app/renderer.h"

#include "lvgl/lvgl.h"

#include "app/drivers/epd3in7_driver.h"
#include "app/drivers/epd3in7_lvgl_adapter.h"

#include "gpio.h"
#include "spi.h"

// Buffer for the full black/white image (1bpp). I1: 2 colors * 4 bytes (ARGB32)
#define STRIDE_BYTES ((EPD3IN7_WIDTH + 7) / 8)
#define DISPLAY_BUFFER_SIZE (STRIDE_BYTES * EPD3IN7_HEIGHT)
#define LVGL_PALETTE_BYTES 8

static LV_ATTRIBUTE_MEM_ALIGN uint8_t lvgl_buffer[LVGL_PALETTE_BYTES + DISPLAY_BUFFER_SIZE];

static uint8_t epd3in7_adapter_work_buffer[DISPLAY_BUFFER_SIZE];

static epd3in7_lvgl_adapter_handle epd3in7_adapter;
static epd3in7_driver_handle epd3in7_drv;

display_handle display_create()
{
    display_handle handle = {};

    return handle;
}

void display_init(display_handle *handle)
{
    epd3in7_drv = epd3in7_driver_create((epd3in7_driver_pins){
                                            .reset_port = DISP_RST_GPIO_Port,
                                            .reset_pin = DISP_RST_Pin,
                                            .dc_port = DISP_DC_GPIO_Port,
                                            .dc_pin = DISP_DC_Pin,
                                            .busy_port = DISP_BUSY_GPIO_Port,
                                            .busy_pin = DISP_BUSY_Pin,
                                            .cs_port = DISP_CS_GPIO_Port,
                                            .cs_pin = DISP_CS_Pin},
                                        &hspi2, true);

    epd3in7_adapter = epd3in7_lvgl_adapter_create(
        &epd3in7_drv,
        epd3in7_adapter_work_buffer,
        10,
        EPD3IN7_DRIVER_MODE_A2,
        32);

    lv_init();
    lv_tick_set_cb(HAL_GetTick);
    lv_display_t *display = lv_display_create(EPD3IN7_WIDTH, EPD3IN7_HEIGHT);
    lv_display_set_driver_data(display, &epd3in7_adapter);
    lv_display_set_buffers(display, lvgl_buffer, NULL, sizeof(lvgl_buffer), LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, epd3in7_lvgl_adapter_flush);
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
}

void display_loop(display_handle *handle, station_data *local, station_data *remote, const bool anything_changed)
{
    if (anything_changed)
    {
        renderer_execute(
            local->temperature, local->humidity, local->pressure, local->bat_in,
            0.0f, 0.0f, 1000, 51);
    }

    lv_timer_handler();
}