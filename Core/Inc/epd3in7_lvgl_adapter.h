#pragma once

#include "epd3in7_driver.h"
#include "lvgl/lvgl.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void epd3in7_lvgl_adapter_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

#ifdef __cplusplus
}
#endif