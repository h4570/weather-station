#include "app/hourly_clock.h"

hourly_clock_handle hourly_clock_create(RTC_HandleTypeDef *hrtc)
{
    hourly_clock_handle handle = {
        .hrtc = hrtc,
        .prev_second = 0,
        .prev_minute = 0,
        .prev_hour = 0,
        .elapsed_seconds = 0,
        .is_initialized = false};

    // Get initial RTC time
    HAL_RTC_GetTime(hrtc, &handle.time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(hrtc, &handle.date, RTC_FORMAT_BIN); // Must be called after HAL_RTC_GetTime to unlock date register

    handle.prev_second = handle.time.Seconds;
    handle.prev_minute = handle.time.Minutes;
    handle.prev_hour = handle.time.Hours;

    // Initialize elapsed seconds based on current time within the hour
    handle.elapsed_seconds = handle.time.Seconds + (handle.time.Minutes * 60);
    handle.is_initialized = true;

    return handle;
}

void hourly_clock_update(hourly_clock_handle *handle)
{
    if (!handle->is_initialized)
    {
        return;
    }

    // Get current RTC time
    HAL_RTC_GetTime(handle->hrtc, &handle->time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(handle->hrtc, &handle->date, RTC_FORMAT_BIN);

    // Check if we're in a new second
    if (handle->time.Seconds != handle->prev_second ||
        handle->time.Minutes != handle->prev_minute ||
        handle->time.Hours != handle->prev_hour)
    {

        // Hour change detection
        if (handle->time.Hours != handle->prev_hour)
        {
            // Reset counter at hour change
            handle->elapsed_seconds = 0;
            // Add the seconds and minutes of the new hour
            handle->elapsed_seconds = handle->time.Seconds + (handle->time.Minutes * 60);
        }
        else
        {
            // Minute change detection
            if (handle->time.Minutes != handle->prev_minute)
            {
                // Seconds from new minute, reset seconds portion
                handle->elapsed_seconds = (handle->elapsed_seconds / 60) * 60;
                // Add the seconds of the new minute
                handle->elapsed_seconds += handle->time.Seconds;
            }
            else
            {
                // Just second change, increment counter
                handle->elapsed_seconds++;

                // Ensure we don't exceed 3599 seconds (59:59)
                if (handle->elapsed_seconds >= 3600)
                {
                    handle->elapsed_seconds = 0;
                }
            }
        }

        // Update previous values
        handle->prev_second = handle->time.Seconds;
        handle->prev_minute = handle->time.Minutes;
        handle->prev_hour = handle->time.Hours;
    }
}

uint32_t hourly_clock_get_elapsed_seconds(const hourly_clock_handle *handle)
{
    if (!handle->is_initialized)
    {
        return 0;
    }

    return handle->elapsed_seconds;
}

hourly_clock_timestamp_t hourly_clock_get_timestamp(const hourly_clock_handle *handle)
{
    if (!handle->is_initialized)
    {
        return 0;
    }

    return handle->elapsed_seconds;
}

bool hourly_clock_check_elapsed(const hourly_clock_handle *handle,
                                hourly_clock_timestamp_t timestamp,
                                uint32_t seconds)
{
    if (!handle->is_initialized)
    {
        return false;
    }

    uint32_t current = handle->elapsed_seconds;

    // Handle hour rollover
    if (current < timestamp)
    {
        // Elapsed time spans across hour boundary
        return (current + (3600 - timestamp)) >= seconds;
    }
    else
    {
        // Simple case - current time is ahead of timestamp
        return (current - timestamp) >= seconds;
    }
}
