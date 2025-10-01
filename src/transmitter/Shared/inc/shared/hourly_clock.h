#pragma once

#include "rtc.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Handle structure for hourly clock
     */
    typedef struct
    {
        RTC_TimeTypeDef time;     /**< RTC time structure */
        RTC_DateTypeDef date;     /**< RTC date structure */
        RTC_HandleTypeDef *hrtc;  /**< Pointer to RTC handle */
        uint8_t prev_second;      /**< Previous second value for change detection */
        uint8_t prev_minute;      /**< Previous minute value for tracking hour changes */
        uint8_t prev_hour;        /**< Previous hour value for tracking hour changes */
        uint32_t elapsed_seconds; /**< Seconds elapsed in the current hour (0-3599) */
        bool is_initialized;      /**< Flag indicating if clock has been initialized */
    } hourly_clock_handle;

    /**
     * @brief Timestamp type for checking elapsed time
     */
    typedef uint32_t hourly_clock_timestamp_t;

    /**
     * @brief Create and initialize an hourly clock handle
     *
     * @param hrtc Pointer to the RTC handle
     * @return hourly_clock_handle The initialized hourly clock handle
     */
    hourly_clock_handle hourly_clock_create(RTC_HandleTypeDef *hrtc);

    /**
     * @brief Update the elapsed seconds counter
     *
     * This function should be called periodically in the main loop.
     * It reads the current RTC time and updates the elapsed_seconds counter.
     * The counter will reset to 0 every hour.
     *
     * @param handle Pointer to the hourly clock handle
     */
    void hourly_clock_update(hourly_clock_handle *handle);

    /**
     * @brief Get the current elapsed seconds in the hour
     *
     * @param handle Pointer to the hourly clock handle
     * @return uint32_t Current elapsed seconds (0-3599)
     */
    uint32_t hourly_clock_get_elapsed_seconds(const hourly_clock_handle *handle);

    /**
     * @brief Get current timestamp for timing operations
     *
     * @param handle Pointer to the hourly clock handle
     * @return hourly_clock_timestamp_t Current timestamp value
     */
    hourly_clock_timestamp_t hourly_clock_get_timestamp(const hourly_clock_handle *handle);

    /**
     * @brief Check if specified number of seconds have elapsed since the timestamp
     *
     * @param handle Pointer to the hourly clock handle
     * @param timestamp Starting timestamp to compare against
     * @param seconds Number of seconds that should elapse
     * @return true if at least the specified number of seconds have elapsed
     * @return false if the specified time has not yet elapsed
     */
    bool hourly_clock_check_elapsed(const hourly_clock_handle *handle,
                                    hourly_clock_timestamp_t timestamp,
                                    uint32_t seconds);

#ifdef __cplusplus
}
#endif
