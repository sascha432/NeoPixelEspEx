/**
 * Author: sascha_lammers@gmx.de
 */

//
// example of a custom crash handler that turns all LEDs off
//

#include <Arduino.h>
#include "NeoPixelEspEx.h"
#include <core_version.h>

NeoPixelEx::Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, NeoPixelEx::GRB, NEOPIXEL_CHIPSET> pixels;

extern "C" void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end);

void custom_crash_callback(struct rst_info *rst_info, uint32_t stack, uint32_t stack_end)
{
    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, NEOPIXEL_NUM_PIXELS);
}

void count_down(PGM_P msg, int n)
{
    for(int i = n; i > 0; i--) {
        Serial.printf_P(PSTR("%s in %u seconds...\n"), msg, i);
        delay(1000);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println(F("Starting..."));
    count_down(PSTR("Turning LEDs on"), 10);

    pixels.begin();
    pixels.fill(0xff00ff);
    pixels.show(16);
}

void loop()
{
    count_down(PSTR("Invoking panic()"), 10);
    panic();
}
