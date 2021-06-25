# NeoPixelEspEx

Library for WS281x and compatible LEDs for ESP8266 and ESP32. It can be used as slim standalone version or together with FastLED, which currently does not work properly with `framework-arduinoespressif8266 3.0.0`

## Features

- Support for dynamic CPU speed (ESP8266 80/160MHHz) with runtime switching
- Support for ESP8266/GPIO16
- Support for ESP32
- Option for ESP8266 to use precaching instead of IRAM
- Support for brightness scaling
- Support for interrupts and retries if interrupted (ESP8266)
- Support for GRB, RGB and other types
- Function to safely clear pixels without using any memory, for example during boot, restart, crash...
- Safety functions to protect LEDs and controller during shutdown
- Can be integrated into FastLED instead of show() or used as slim standalone version that does not need IRAM

## Examples

Examples for PlatformIO can be found in the `examples/` folder.

## Basic usage / Legacy function

```c++
#include <NeoPixelEspEx.h>

#define NEOPIXEL_OUTPUT_PIN 12
uint8_t one_pixel[3] = { 0x20, 0, 0 };

NeoPixel_espShow(NEOPIXEL_OUTPUT_PIN, one_pixel, sizeof(one_pixel));
```

## Advanced usage


## Pins ESP8266

Usable pins for LED strips. Depending on the levelshifter, GPIO 1, 3, 9 and 10 can be used. GPIO 9 and 10 require to run the flash memory in DIO mode.

  NodeMCU | PIN | Description |
|---|---|---|
| D0 | GPIO16 | floating (or connected to reset, directly or via low value series resistor = pullup) |
| D1 | GPIO5 | floating
| D2 | GPIO4 | floating
| D3 | GPIO0 | pullup
| D4 | GPIO2 | pullup
| D5 | GPIO14 | floating
| D6 | GPIO12 | floating
| D7 | GPIO13 | floating
| D8 | GPIO15 | pulldown
| RX | GPIO3 | serial2usb converter
| TX | GPIO1 | serial2usb converter
| S2 | GPIO9 | flash memory (QIO mode)
| S3 | GPIO10 | flash memory (QIO mode)

## Based on NeoPixel_esp

This library is an extended version based on

```c++
// This is a mash-up of the Due show() code + insights from Michael Miller's
// ESP8266 work for the NeoPixelBus library: github.com/Makuna/NeoPixelBus
```

and

(https://github.com/adafruit/Adafruit_NeoPixel/blob/master/esp8266.c)[https://github.com/adafruit/Adafruit_NeoPixel/blob/master/esp8266.c]

