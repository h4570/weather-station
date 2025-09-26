#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "app/display_data.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Handle structure for the display
     */
    typedef struct
    {
    } display_handle;

    display_handle display_create();
    void display_init(display_handle *handle);
    void display_loop(display_handle *handle, display_data *local, display_data *remote, const bool anything_changed);

#ifdef __cplusplus
}
#endif
