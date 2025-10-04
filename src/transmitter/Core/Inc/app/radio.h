#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "stm32l0xx_hal.h"
#include "shared/app_device_data.h"
#include "shared/hourly_clock.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Handle structure for radio module
     */
    typedef struct
    {
        GPIO_TypeDef *cs_port;
        uint16_t cs_pin;
        GPIO_TypeDef *dio0_port;
        uint16_t dio0_pin;
        SPI_HandleTypeDef *hspi;
        hourly_clock_handle *clock;
        bool is_initialized;
        bool has_error;
        hourly_clock_timestamp_t last_send_timestamp;
        uint8_t packet[18];
    } radio_handle;

    /**
     * @brief Create and initialize a radio handle
     * @param cs_port GPIO port for Chip Select (CS)
     * @param cs_pin GPIO pin for Chip Select (CS)
     * @param dio0_port GPIO port for DI0 interrupt
     * @param dio0_pin GPIO pin for DI0 interrupt
     * @param hspi Pointer to the SPI handle
     * @param clock Pointer to the hourly clock handle for timestamping
     * @return Initialized radio handle
     */
    radio_handle radio_create(GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *dio0_port, uint16_t dio0_pin, SPI_HandleTypeDef *hspi, hourly_clock_handle *clock);

    /**
     * @brief Initialize the radio module
     */
    void radio_init(radio_handle *handle);

    /**
     * @brief Main loop function for the radio module
     */
    void radio_loop(radio_handle *handle);

    /**
     * @brief Send data using the radio module
     */
    void radio_send(radio_handle *handle, const app_device_data *data);

    /**
     * @brief EXTI interrupt handler for the radio module (to be called from main EXTI handler)
     */
    void radio_exti_interrupt_handler(const uint16_t pin);

#ifdef __cplusplus
}
#endif
