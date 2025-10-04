#include "app/radio.h"
#include "shared/drivers/rfm69.h"
#include <string.h>

static RFM69_HandleTypeDef radio_rfm69_handle;
static volatile uint16_t radio_it_di0_pin = 0;
static volatile bool radio_is_initialized = false;

radio_handle radio_create(GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *dio0_port, uint16_t dio0_pin, SPI_HandleTypeDef *hspi, hourly_clock_handle *clock)
{
    radio_handle handle;
    handle.cs_port = cs_port;
    handle.cs_pin = cs_pin;
    handle.dio0_port = dio0_port;
    handle.dio0_pin = dio0_pin;
    handle.hspi = hspi;
    handle.clock = clock;

    handle.is_initialized = false;
    handle.has_error = false;

    handle.last_send_timestamp = (hourly_clock_timestamp_t){0};

    return handle;
}

void radio_init(radio_handle *handle)
{
    radio_rfm69_handle.hspi = handle->hspi;
    radio_rfm69_handle.cs_port = handle->cs_port;
    radio_rfm69_handle.cs_pin = handle->cs_pin;
    radio_rfm69_handle.dio0_port = handle->dio0_port;
    radio_rfm69_handle.dio0_pin = handle->dio0_pin;
    radio_rfm69_handle.isRFM69HW = 1;

    radio_it_di0_pin = handle->dio0_pin;

    HAL_GPIO_WritePin(radio_rfm69_handle.cs_port, radio_rfm69_handle.cs_pin, GPIO_PIN_SET);

    const uint16_t node_id = 2;
    const uint16_t network_id = 42;

    if (!RFM69_Init(&radio_rfm69_handle, 86, node_id, network_id))
    {
        handle->has_error = true;
        return;
    }

    uint8_t v = RFM69_GetVersion(&radio_rfm69_handle);
    uint32_t freq = RFM69_GetFrequency(&radio_rfm69_handle);

    if (v != 0x24 || freq < 860000000 || freq > 890000000)
    {
        handle->has_error = true;
        return;
    }

    RFM69_SetPowerDBm(&radio_rfm69_handle, 13);

    radio_is_initialized = true;
    handle->is_initialized = true;
}

void radio_loop(radio_handle *handle)
{
    if (!radio_is_initialized || !handle->is_initialized)
        return;
}

void radio_send(radio_handle *handle, const app_device_data *data)
{
    RFM69_SetMode(&radio_rfm69_handle, RF69_MODE_STANDBY);
    RFM69_WaitModeReady(&radio_rfm69_handle, 1000);

    handle->packet[0] = 'S';
    memcpy(&handle->packet[1], &data->temperature, sizeof data->temperature);
    memcpy(&handle->packet[5], &data->humidity, sizeof data->humidity);
    memcpy(&handle->packet[9], &data->pressure, sizeof data->pressure);
    memcpy(&handle->packet[13], &data->bat_in, sizeof data->bat_in);
    handle->packet[17] = 'E';

    const uint16_t target_node_id = 1;

    RFM69_Send(&radio_rfm69_handle, target_node_id, handle->packet, sizeof handle->packet, false);

    handle->last_send_timestamp = hourly_clock_get_timestamp(handle->clock);

    RFM69_Sleep(&radio_rfm69_handle);
}

void radio_exti_interrupt_handler(const uint16_t pin)
{
    if (!radio_is_initialized)
        return;

    if (pin == radio_it_di0_pin)
    {
        RFM69_OnDIO0IRQ(&radio_rfm69_handle);
    }
}