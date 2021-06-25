/**
 * Author: sascha_lammers@gmx.de
 */

#include <Arduino.h>
#include "NeoPixelEspEx.h"
#include <core_version.h>

#ifndef NEOPIXEL_OUTPUT_PIN
#define NEOPIXEL_OUTPUT_PIN 12
#endif

#ifndef NEOPIXEL_NUM_PIXELS
#define NEOPIXEL_NUM_PIXELS 2
#endif

NeoPixelEx::Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, NeoPixelEx::GRB, NeoPixelEx::TimingsWS2813<F_CPU>::type> pixels;

extern bool record_serial_output;
extern String serial_output;

#if defined(ESP8266)

extern "C" void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end);

void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end)
{
    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, 256);
}

#endif

#if 0
uart_t *_uart;

void preinit() {
    _uart = uart_init(UART0, 115200, (int) SERIAL_8N1, (int) SERIAL_FULL, 1, 64, false);
    ::printf("test0\n");
}
#endif

void setup()
{
    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, 256);
    delay(2);

    Serial.begin(115200);
    Serial.println(F("Starting..."));


    pixels.begin();
}

#if 0
NeoPixelEx::GRB color(0, 0, 0xff);
uint8_t brightness = 16;
int direction = 0;
uint32_t counter = 0;

void loop()
{
    pixels.fill(color);
    pixels.show(brightness);
    if (brightness == 0 && direction == -1) {
        direction = 1;
    }
    else if (brightness == 0xff && direction == 1) {
        direction = -1;
    }
    brightness += direction;
    if (++counter >= 100) {
        counter = 0;
        color <<= 8;
        Serial.printf_P(PSTR("%u %s %u/%u\n"), brightness, color.toString().c_str(), NeoPixel_getAbortedFrames, NeoPixel_getFrames);
    }
    delay(1000 / 100);
}

#endif

#if 1

NeoPixelEx::GRB color(0, 0, 0xff);
uint8_t brightness = 16;
uint8_t delayMillis = 10;
uint8_t *ptr = &brightness;

static PGM_P get_type_str()
{
    if (ptr == &brightness) {
        return PSTR("brightness");
    }
    if (ptr == &color.r) {
        return PSTR("red");
    }
    if (ptr == &color.g) {
        return PSTR("green");
    }
    if (ptr == &color.r) {
        return PSTR("red");
    }
    if (ptr == &delayMillis) {
        return PSTR("delay");
    }
    return PSTR("?");
}

static uint32_t get_type_incr_key_repeat_timeout = 0;

static int get_type_incr()
{
    if (ptr == &brightness) {
        return get_type_incr_key_repeat_timeout ? std::max(1, *ptr / 32) : 1;
    }
    if (
        (ptr == &color.r) ||
        (ptr == &color.g) ||
        (ptr == &color.r)
    ) {
        return 16;//get_type_incr_key_repeat_timeout ? 4 : 1;
    }
    if (ptr == &delayMillis) {
        return 1;
    }
    return 1;
}

void loop()
{
    if (get_type_incr_key_repeat_timeout && millis() > get_type_incr_key_repeat_timeout) {
        get_type_incr_key_repeat_timeout = 0;
    }
    pixels.fill(color);
    pixels.show(brightness);
    if (Serial.available()) {
        while(Serial.available()) {
            switch(Serial.read()) {
                case '+':
                    get_type_incr_key_repeat_timeout = millis() + 2000;
                    *ptr = std::min<int>(255, (uint16_t)*ptr + get_type_incr());
                    break;
                case '-':
                    get_type_incr_key_repeat_timeout = millis() + 2000;
                    *ptr = std::max<int>(0, (uint16_t)*ptr - get_type_incr());
                    break;
                case 'r':
                    ptr = &color.r;
                    break;
                case 'b':
                    ptr = &color.b;
                    break;
                case 'g':
                    ptr = &color.g;
                    break;
                case 'B':
                    ptr = &brightness;
                    break;
                case 'd':
                    ptr = &delayMillis;
                    break;
                case 'R':
                    Serial.println(F("reset"));
                    color = NeoPixelEx::GRB(0, 0, 0xff);
                    brightness = 16;
                    delayMillis = 10;
                    ptr = &brightness;
                case 'C':
                    Serial.println(F("clearing all pixels..."));
                    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, 256);
                    Serial.println(F("waiting 5 seconds..."));
                    delay(5000);
                    while(Serial.available()) {
                        Serial.read();
                    }
                    break;
                case 'o':
                    Serial.println(F("off..."));
                    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, 256);
                    for(;;) { delay(1000); }
                    break;
            #if defined(ESP8266)
                case 'T':
                    if (ESP.getCpuFreqMHz() == 80) {
                        ets_update_cpu_frequency(160);
                        Serial.println(F("CPU 160MHz"));
                    }
                    else {
                        ets_update_cpu_frequency(80);
                        Serial.println(F("CPU 80MHz"));
                    }
                    break;
                #endif
            }
            #if DEBUG_RECORD_SERIAL_OUTPUT
                record_serial_output = true;
                pixels.show(brightness);
            #endif
        }
        Serial.printf_P(PSTR("%s step=%u brightness=%u %s %u/%u\n"), get_type_str(), get_type_incr(), brightness, color.toString().c_str(), NeoPixel_getAbortedFrames, NeoPixel_getFrames);
        #if DEBUG_RECORD_SERIAL_OUTPUT
                Serial.println(serial_output);
        #endif
    }
    delay(delayMillis);
}


#endif