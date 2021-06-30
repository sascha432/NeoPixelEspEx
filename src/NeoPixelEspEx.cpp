/**
 * Author: sascha_lammers@gmx.de
 */

#include <Arduino.h>
#include <StreamString.h>
#if defined(ESP8266)
#include <coredecls.h>
#include <ESP8266WiFi.h>
#include <user_interface.h>
#endif
#if defined(ESP32)
#endif

#include "NeoPixelEspEx.h"

#pragma GCC optimize ("O2")

NeoPixelEx::Context NeoPixelEx::_globalContext;

void NeoPixel_fillColor(uint8_t *pixels, uint16_t numBytes, uint32_t color)
{
    uint8_t *ptr = pixels;
    for(uint8_t i = 0; i < numBytes; i += 3) {
        *ptr++ = color >> 8;
        *ptr++ = color >> 16;
        *ptr++ = color;
    }
}

#if defined(ESP8266)

__attribute__((always_inline)) inline static void gpio_set_level_high(uint8_t pin, uint32_t pinMask, uint32_t invertedPinMask)
{
    if (pin == 16) {
        GP16O |= 1;
    }
    else {
        GPOS = pinMask;
    }
}

__attribute__((always_inline)) inline static void gpio_set_level_low(uint8_t pin, uint32_t pinMask, uint32_t invertedPinMask)
{
    if (pin == 16) {
        GP16O &= ~1;
    }
    else {
        GPOC = pinMask; // 23 cycles incl
    }
}

#else

__attribute__((always_inline)) inline static void gpio_set_level_high(uint8_t pin, uint32_t pinMask, uint32_t invertedPinMask)
{
    gpio_set_level(pin, true);
}

__attribute__((always_inline)) inline static void gpio_set_level_low(uint8_t pin, uint32_t pinMask, uint32_t invertedPinMask)
{
    gpio_set_level(pin, false);
}

#endif

__attribute__((always_inline)) inline static uint32_t _getCycleCount(void)
{
    uint32_t cyleCount;
    __asm__ __volatile__("rsr %0,ccount" : "=a"(cyleCount));
    return cyleCount;
}

#if NEOPIXEL_HAVE_BRIGHTHNESS

__attribute__((always_inline)) inline static uint8_t loadPixel(const uint8_t *&ptr, uint16_t brightness)
{
    if (brightness == 0) {
        ptr++;
        return 0;
    }
    return *ptr++;
}

__attribute__((always_inline)) inline static uint8_t applyBrightness(uint8_t pixel, uint16_t brightness)
{
    // if (brightness == 0) {
    //     return 0;
    // }
    return (pixel * brightness) >> 8;
}

#else

__attribute__((always_inline)) inline static uint8_t loadPixel(const uint8_t *&ptr, uint16_t brightness)
{
    if (brightness == 0) {
        ptr++;
        return 0;
    }
    return *ptr++;
}

__attribute__((always_inline)) inline static uint8_t applyBrightness(uint8_t pixel, uint16_t brightness)
{
    return pixel;
}

#endif

// extra function to keep the IRAM usage low
void NEOPIXEL_ESPSHOW_FUNC_ATTR _espShow(uint8_t pin, uint16_t brightness, uint8_t pix, uint8_t mask, const uint8_t *p, const uint8_t *end, uint32_t time0, uint32_t time1, uint32_t &period, uint32_t wait, uint32_t minWaitPeriod, uint32_t pinMask, uint32_t invPinMask)
{
#if NEOPIXEL_USE_PRECACHING
    PRECACHE_START(NeoPixel_espShow);
#endif
    uint32_t startTime = 0;
    uint32_t c, t;

    for (;;) {
        t = (pix & mask) ? time1 : time0;

        #if NEOPIXEL_ALLOW_INTERRUPTS
            if (((c = _getCycleCount()) - startTime) <= period) {
        #endif
                while (((c = _getCycleCount()) - startTime) < period) {
                    // wait for bit start
                }
        #if NEOPIXEL_ALLOW_INTERRUPTS
            }
            else if (startTime) {
                // set period to 0 to remove the wait time and marker for timeout
                period = 0;
                break;
            }
        #endif

        gpio_set_level_high(pin, pinMask, invPinMask);
        startTime = c; // Save start time

        if (!(mask >>= 1)) {
            if (p < end) {
                mask = 0x80; // load next byte indicator
                pix = loadPixel(p, brightness);
            }
            else {
                mask = 0; // end of frame indicator
            }
        }

        #if NEOPIXEL_ALLOW_INTERRUPTS
            if (((c = _getCycleCount()) - startTime) <= t) {
        #endif
                while (((c = _getCycleCount()) - startTime) < t) {
                    // t0h/t1h wait
                }
        #if NEOPIXEL_ALLOW_INTERRUPTS
            }
            else {
                gpio_set_level_low(pin, pinMask, invPinMask);
                period = 0;
                break;
            }
        #endif

        gpio_set_level_low(pin, pinMask, invPinMask);
        if (mask == 0) { // end of frame
            break;
        }
        if (mask == 0x80) {
            pix = applyBrightness(pix, brightness);
        }

    }
    while ((_getCycleCount() - startTime) < period) {
        // t0l/t1l wait
    }

#if NEOPIXEL_USE_PRECACHING
    PRECACHE_END(NeoPixel_espShow);
#endif
}

bool espShow(uint8_t pin, uint16_t brightness, const uint8_t *p, const uint8_t *end, uint32_t time0, uint32_t time1, uint32_t period, uint32_t wait, uint32_t minWaitPeriod, uint32_t pinMask, void *contextPtr)
{
    auto &context = *reinterpret_cast<NeoPixelEx::Context *>(contextPtr);
    brightness &= 0xff;
    if (brightness) { // change range to 1-256 to avoid divison by 255 in applyBrightness()
        brightness++;
    }

    uint8_t pix = loadPixel(p, brightness);
    pix = applyBrightness(pix, brightness);
    uint8_t mask = 0x80;

    context.waitRefreshTime(minWaitPeriod);

    #if defined(ESP8266) && !NEOPIXEL_ALLOW_INTERRUPTS
        ets_intr_lock();
    #endif

    #if NEOPIXEL_DEBUG
        context.getDebugContext().togglePin();
    #endif

    // this part must be in IRAM/ICACHE
    _espShow(pin, brightness, pix, mask, p, end, time0, time1, period, wait, minWaitPeriod, pinMask, ~pinMask);

    #if NEOPIXEL_HAVE_STATS
        context.getStats().increment(period != 0);
    #endif

    context.setLastDisplayTime(micros());

    #if defined(ESP8266) && !NEOPIXEL_ALLOW_INTERRUPTS
        ets_intr_unlock();
    #endif

    return (period != 0);
}
