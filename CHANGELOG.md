# Changelog

## Version 0.0.2

- GPIO16 seems to have some issues when running with 160MHz
- Wrapper class for plain data pointers
- Context object for each LED strip and global context object for legacy functions
- Support for espressif8266@3.0.0, 2.6.3 and 2.3.3
- Templates for timings
- GRB, RGB and Array types
- Strip class
- Clear function that does not require to allocate any memory

## Version 0.0.1

First version based on this is a mash-up of the Due show() code + insights from Michael Miller's
ESP8266 work for the NeoPixelBus library: github.com/Makuna/NeoPixelBus and
(https://github.com/adafruit/Adafruit_NeoPixel/blob/master/esp8266.c)[https://github.com/adafruit/Adafruit_NeoPixel/blob/master/esp8266.c]

