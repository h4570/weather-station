#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "stm32g4xx_hal.h"

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
    } radio_handle;

    /**
     * @brief Create and initialize a radio handle
     */
    radio_handle radio_create(GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *dio0_port, uint16_t dio0_pin, SPI_HandleTypeDef *hspi);

    /**
     * @brief Initialize the radio module
     */
    void radio_init(radio_handle *handle);

    /**
     * @brief Main loop function for the radio module
     */
    void radio_loop(radio_handle *handle);

    /**
     * @brief EXTI interrupt handler for the radio module (to be called from main EXTI handler)
     */
    void radio_exti_interrupt_handler(const uint16_t pin);

#ifdef __cplusplus
}
#endif
