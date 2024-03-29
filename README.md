# NeoPixelEspEx

Library for WS281x and compatible LEDs for ESP8266 and ESP32. It can be used as slim standalone version or together with FastLED, <s>which currently does not work properly with `framework-arduinoespressif8266 3.0.0`</s> The current master branch fixes the issue.

## FastLED Fork

Since the current FastLED 3.5 still has issues with WS281x (ESP8266), I updated my fork [https://github.com/sascha432/FastLED](https://github.com/sascha432/FastLED)

## Features

- Support for ESP8266/GPIO16
- Option to use precaching instead of IRAM
- Support for brightness scaling
- Support for interrupts and retries if interrupted (ESP8266)
- Support for GRB, RGB, CRGB (FastLED) and other types
- Function to safely clear pixels without allocating any memory, for example during boot, restart, crash...

## Examples

Examples for PlatformIO can be found in the `examples/` folder.

## FastLED

A workaround for FastLED and v3.0.0 is to replace `FastLED.show()` with `Strip::show()`. Only brightness is supported.

```c++
CRGB pixelData[NEOPIXEL_NUM_PIXELS];
NeoPixelEx::Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, NeoPixelEx::CRGB, NeoPixelEx::TimingsWS2812, NeoPixelEx::DataWrapper<NEOPIXEL_NUM_PIXELS, NeoPixelEx::CRGB>> pixels(&pixelData);

fill_rainbow(pixelData, pixels.getNumPixels(), beat8(10, 255), 10);

pixels.show(FastLED.getBrightness());
// or
FastLED.show();
```


## Basic usage

### Legacy functions

It is recommended to use the class `NeoPixelEx::Strip` instead of `NeoPixel_espShow()`.

```c++
#include <NeoPixelEspEx.h>

#define NEOPIXEL_OUTPUT_PIN 12

void setup() {
  pinMode(NEOPIXEL_OUTPUT_PIN, OUTPUT);
  ...
}

void loop() {
  ...
  // set color of first pixel to 0x20000
  uint8_t one_pixel[3] = { 0x20, 0, 0 };
  NeoPixel_espShow(NEOPIXEL_OUTPUT_PIN, one_pixel, sizeof(one_pixel));
  ...
  // fill 8 pixels with color #220011 and display
  uint8_t pixels[8 * 3];
  NeoPixel_fill(pixels, sizeof(pixels), 0x220011);
  NeoPixel_espShow(NEOPIXEL_OUTPUT_PIN, pixels, sizeof(pixels));
  ...
  // turn 256 LEDs off without reading the data from tmp
  // brightness must be 0
  NeoPixel_espShow(NEOPIXEL_OUTPUT_PIN, nullptr, 3 * 256, 0);
}

```

## Strip class

```c++
#include <NeoPixelEspEx.h>

#define NEOPIXEL_OUTPUT_PIN 12

NeoPixelEx::Strip<NEOPIXEL_OUTPUT_PIN, 8, NeoPixelEx::GRB, NeoPixelEx::TimingsWS2812> pixels;

void setup() {
  ...
  pixels.begin();
  ...
  // set all pixels color 0x000000 and update with 0 brightness
  pixels.clear();
  ...
  // set all LEDs to 0x100010 and update with full brightness
  pixels.fill(0x100010);
  pixels.show();
  ...
}

void loop() {
  ...
  // set the color of pixel 1 and 2, and update the LEDs with brightness level 32
  pixels[0] = 0xff0000;
  pixels[1] = 0x00ff00;
  pixels.show(32);
  ...
  // turn 256 LEDs off without Strip object or allocating any memory
  // NeoPixelEx::DefaultTimings is used
  NeoPixelEx::forceClear(256);
  ...
}
```

### Changing pixel data

`pixels.data()` provides access to the underlying data object. The default is `PixelData` which is an array with some additional functions to manipulare the data.

### DataWrapper

To use existing data with the Strip class, the DataWrapper template can be used. It provides direct access to the data without any additional overhead.

```c++

uint8_t pixelData[NEOPIXEL_NUM_PIXELS * sizeof(NeoPixelEx::GRB)];
NeoPixelEx::Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, NeoPixelEx::GRB, NeoPixelEx::TimingsWS2812, DataWrapper<NEOPIXEL_NUM_PIXELS, NeoPixelEx::GRB>> pixels(&pixelData);

```

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

This library is based on

```c++
// This is a mash-up of the Due show() code + insights from Michael Miller's
// ESP8266 work for the NeoPixelBus library: github.com/Makuna/NeoPixelBus
```
and

[https://github.com/adafruit/Adafruit_NeoPixel/blob/master/esp8266.c](https://github.com/adafruit/Adafruit_NeoPixel/blob/master/esp8266.c)

