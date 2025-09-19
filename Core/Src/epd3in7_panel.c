#include "epd3in7_panel.h"

epd3in7_panel_handle epd3in7_panel_init(epd3in7_driver_handle *driver_handle)
{
    epd3in7_panel_handle panel_handle;
    panel_handle.driver_handle = driver_handle;
    return panel_handle;
}