#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "shared/app_device_data.h"
#include "shared/drivers/spi_bus_manager.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Handle structure for the display
     */
    typedef struct
    {
        bool anything_was_rendered; /**< Flag indicating if anything was rendered for the display */
        spi_bus_manager *spi_mgr;   /**< SPI bus manager for handling SPI transactions */
    } display_handle;

    /**
     * @brief Create and return a new display handle
     * @param spi_mgr Pointer to an initialized SPI bus manager for handling SPI transactions
     */
    display_handle display_create(spi_bus_manager *spi_mgr);

    /**
     * @brief Initialize the display hardware and LVGL
     *
     * @param handle Pointer to the display handle
     */
    void display_init(display_handle *handle);

    /**
     * @brief Main display loop to update the display if needed
     *
     * @param handle Pointer to the display handle
     * @param local Pointer to the local station data
     * @param remote Pointer to the remote station data (can be NULL if not used)
     * @param changes_detected Flag indicating if any changes were detected that require a display update
     */
    void display_loop(display_handle *handle, app_device_data *local, app_device_data *remote, const bool changes_detected);

#ifdef __cplusplus
}
#endif
