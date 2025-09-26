#pragma once

#include <stdint.h>
#include "adc.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Handle structure for the battery measurement system
     */
    typedef struct
    {
        uint16_t current_adc_value;       /**< Current ADC value read from the battery voltage divider */
        uint16_t previous_adc_value;      /**< Previous ADC value for filtering purposes */
        uint16_t current_bat_level;       /**< Current battery level percentage (0-100%) */
        uint16_t last_returned_bat_level; /**< Last returned battery level percentage (0-100%) */
        float temperature;                /**< Current temperature in degrees Celsius for temperature compensation */
    } battery_handle;

    /**
     * @brief Create and return a new battery handle
     */
    battery_handle battery_create();

    /**
     * @brief Refresh the battery measurement system (check if new ADC value from interrupt is available)
     * @param handle Pointer to the battery handle
     */
    void battery_refresh(battery_handle *handle);

    /**
     * @brief Request a battery voltage read (starts ADC conversion)
     * @param handle Pointer to the battery handle
     */
    void battery_request_read(battery_handle *handle);

    /**
     * @brief Check if the battery level has changed since the last check
     *
     * @param handle Pointer to the battery handle
     * @return true if the battery level has changed, false otherwise
     */
    bool battery_check_if_level_changed(battery_handle *handle);

    /**
     * @brief Get the current battery level percentage (0-100%) and updates internal state
     *
     * @param handle Pointer to the battery handle
     * @return Current battery level percentage
     */
    uint16_t battery_get_level(battery_handle *handle);

    /**
     * @brief Update the temperature used in battery calculations
     *
     * @param temperature_c The current temperature in degrees Celsius
     */
    void battery_update_temperature(battery_handle *handle, float temperature_c);

    /**
     * @brief Process ADC interrupt to read battery voltage
     */
    void battery_adc_interrupt_handler(ADC_HandleTypeDef *hadc);

#ifdef __cplusplus
}
#endif
