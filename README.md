# STM32 Driver for Waveshare 3.7" E-Paper Display (20123)

This repository contains a quasi-async alternative driver for the official 3.7-inch e-paper display (Waveshare 20123) for STM32 microcontrollers using the HAL library.

![Waveshare 3.7" E-Paper Display](https://hasto.pl/wp-content/uploads/2025/09/stm32-nucleo-waveshare-caly-zestaw-Large.jpg)

## Features

- **Grayscale Support**: 4-level grayscale mode and 1-bit (black & white) modes
- **Multiple Refresh Modes**:
  - **GC (Global Clear)**: Full refresh with best quality
  - **DU (Direct Update)**: Partial refresh for most scenarios
  - **A2 Mode**: Fast partial refresh (with quality tradeoff)
- **HAL Compatible**: Designed to work with STM32 HAL library
- **Error Handling**: Comprehensive status codes and error reporting
- **Power Efficient**: Includes sleep mode for power conservation
- **Flexible Configuration**: Configurable GPIO pins for hardware interface

## Display Specifications

- **Resolution**: 280 × 480 pixels
- **Interface**: SPI
- **Colors**: Black, white, and two levels of gray (in 4-gray mode)
- **Viewing Angle**: Nearly 180°
- **Full Refresh Time**: Approximately 4-6 seconds
- **Partial Refresh Time**: Approximately 0.3-1 seconds

## Hardware Requirements

- STM32 microcontroller (tested on STM32G474)
- 3.3V power supply
- SPI interface
- GPIO pins for control signals

## Pin Configuration

The driver requires the following pins to be connected:

| E-Paper Pin | Function  | STM32 Connection |
|-------------|-----------|------------------|
| BUSY        | Busy Status | GPIO Input      |
| RST         | Reset       | GPIO Output     |
| DC          | Data/Command| GPIO Output     |
| CS          | Chip Select | GPIO Output     |
| CLK         | SPI Clock   | SPI SCK        |
| DIN         | SPI MOSI    | SPI MOSI       |
| GND         | Ground      | GND            |
| VCC         | Power       | 3.3V           |

## Installation

1. Copy the driver files ([epd3in7_driver.h](https://github.com/h4570/3in7-epaper-driver-for-stm32-waveshare-20123/blob/main/Core/Inc/epd3in7_driver.h) and [epd3in7_driver.c](https://github.com/h4570/3in7-epaper-driver-for-stm32-waveshare-20123/blob/main/Core/Src/epd3in7_driver.c)) to your project
2. Include the header file in your application code: `#include "epd3in7_driver.h"`
3. Configure your SPI peripheral and GPIO pins
4. Initialize the driver with your pin configuration

## Basic Usage

### Initialize the Display

```c
// Define pin configuration
epd3in7_driver_pins pins = {
    .reset_port = GPIOA,
    .reset_pin = GPIO_PIN_1,
    .dc_port = GPIOA,
    .dc_pin = GPIO_PIN_2,
    .busy_port = GPIOA,
    .busy_pin = GPIO_PIN_3,
    .cs_port = GPIOA,
    .cs_pin = GPIO_PIN_4
};

// Create driver handle
SPI_HandleTypeDef hspi1; // Your SPI handle
epd3in7_driver_handle epd_handle = epd3in7_driver_create(pins, &hspi1, false);

// Initialize for black & white mode
epd3in7_driver_init_1_gray(&epd_handle);
```

### Display an Image (Black & White)

```c
// Buffer containing your image data (must be 280*480/8 = 16800 bytes)
uint8_t image_buffer[EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 8] = {/* your image data */};

// Display image with global refresh (full quality)
epd3in7_driver_display_1_gray(&epd_handle, image_buffer, EPD3IN7_DRIVER_MODE_GC);

// Wait for refresh to complete (optional if doing other tasks)
epd3in7_driver_busy_wait_for_idle(&epd_handle);
```

### Display Grayscale Image

```c
// Initialize for grayscale mode
epd3in7_driver_init_4_gray(&epd_handle);

// Buffer for 4-level grayscale (must be 280*480/4 = 33600 bytes)
uint8_t grayscale_image[EPD3IN7_WIDTH * EPD3IN7_HEIGHT / 4] = {/* your grayscale data */};

// Display grayscale image
epd3in7_driver_display_4_gray(&epd_handle, grayscale_image);
```

### Sleep Mode

```c
// Always put the display in sleep mode when not in use to prevent damage
epd3in7_driver_sleep(&epd_handle);
```

## Usage Notes and Precautions

### Important Warning

⚠️ **NEVER LEAVE THE DISPLAY POWERED ON WITHOUT REFRESHING**

The e-paper display can be permanently damaged if left powered on in a high voltage state without refreshing or entering sleep mode. Always call `epd3in7_driver_sleep()` when not actively refreshing the display.

### Partial Refresh Guidelines

- Do not use partial refresh (DU mode) continuously; occasionally refresh with GC mode
- For A2 mode (fast refresh), first display a full image in DU mode, then use A2 for partial updates
- After several partial refreshes, perform a full refresh to maintain display quality

## Error Handling

The driver returns status codes to indicate success or failure:

```c
epd3in7_driver_status status = epd3in7_driver_display_1_gray(&epd_handle, image_buffer, EPD3IN7_DRIVER_MODE_GC);
if (status != EPD3IN7_DRIVER_OK) {
    // Handle error
    switch (status) {
        case EPD3IN7_DRIVER_ERR_HAL:
            // HAL error occurred
            break;
        case EPD3IN7_DRIVER_ERR_TIMEOUT:
            // Operation timed out
            break;
        case EPD3IN7_DRIVER_ERR_PARAM:
            // Invalid parameter
            break;
    }
}
```

## Example Project

This repository includes an example STM32 project configured for a NUCLEO-G474RE board.

## License

This driver is based on Waveshare sample code with significant modifications for better performance and reliability.

## Credits

- Original driver by Waveshare
- Modified and enhanced by h4570

Check my blog at <a href="https://hasto.pl">hasto.pl</a>  
