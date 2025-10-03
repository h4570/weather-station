#include "app/radio.h"
#include "shared/drivers/rfm69.h"
#include <string.h>

static RFM69_HandleTypeDef radio_rfm69_handle;
static volatile uint16_t radio_it_di0_pin = 0;
static volatile bool radio_is_initialized = false;

radio_handle radio_create(GPIO_TypeDef *cs_port, uint16_t cs_pin, GPIO_TypeDef *di0_port, uint16_t di0_pin, SPI_HandleTypeDef *hspi, hourly_clock_handle *clock)
{
    radio_handle handle;
    handle.cs_port = cs_port;
    handle.cs_pin = cs_pin;
    handle.di0_port = di0_port;
    handle.di0_pin = di0_pin;
    handle.hspi = hspi;
    handle.clock = clock;

    handle.is_initialized = false;
    handle.has_error = false;

    memset(&handle.last_received_data, 0, sizeof(handle.last_received_data));
    memset(&handle.last_returned_data, 0, sizeof(handle.last_returned_data));
    handle.last_receive_timestamp = (hourly_clock_timestamp_t){0};

    return handle;
}

void radio_init(radio_handle *handle)
{
    radio_rfm69_handle.hspi = handle->hspi;
    radio_rfm69_handle.cs_port = handle->cs_port;
    radio_rfm69_handle.cs_pin = handle->cs_pin;
    radio_rfm69_handle.dio0_port = handle->di0_port;
    radio_rfm69_handle.dio0_pin = handle->di0_pin;
    radio_rfm69_handle.isRFM69HW = 1;

    radio_it_di0_pin = handle->di0_pin;

    HAL_GPIO_WritePin(radio_rfm69_handle.cs_port, radio_rfm69_handle.cs_pin, GPIO_PIN_SET);

    uint16_t node_id = 1;
    uint16_t network_id = 42;

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

    if (RFM69_ReceiveDone(&radio_rfm69_handle))
    {
        if (radio_rfm69_handle.DATALEN == 18 && radio_rfm69_handle.DATA[0] == 'S' && radio_rfm69_handle.DATA[17] == 'E')
        {
            memcpy(&handle->last_received_data.temperature, &radio_rfm69_handle.DATA[1], sizeof(float));
            memcpy(&handle->last_received_data.humidity, &radio_rfm69_handle.DATA[5], sizeof(float));
            memcpy(&handle->last_received_data.pressure, &radio_rfm69_handle.DATA[9], sizeof(int32_t));
            memcpy(&handle->last_received_data.bat_in, &radio_rfm69_handle.DATA[13], sizeof(int32_t));

            handle->last_receive_timestamp = hourly_clock_get_timestamp(handle->clock);
        }

        if (RFM69_ACKRequested(&radio_rfm69_handle))
        {
            const char ok[] = "OK";
            RFM69_SendACK(&radio_rfm69_handle, ok, sizeof ok - 1);
        }

        RFM69_Consume(&radio_rfm69_handle);
    }
}

bool radio_check_if_data_changed(radio_handle *handle)
{
    return memcmp(&handle->last_received_data, &handle->last_returned_data, sizeof(app_device_data)) != 0;
}

app_device_data radio_get_data(radio_handle *handle)
{
    handle->last_returned_data = handle->last_received_data;
    return handle->last_received_data;
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