#pragma once

#include "epd3in7_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Handle structure for the e-Paper display
     */
    typedef struct
    {
        epd3in7_driver_handle *driver_handle; /**< Pointer to the driver handle */
    } epd3in7_panel_handle;

    epd3in7_panel_handle epd3in7_panel_init(epd3in7_driver_handle *driver_handle);

#ifdef __cplusplus
}
#endif