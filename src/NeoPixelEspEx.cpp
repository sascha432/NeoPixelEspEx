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

// #if NEOPIXEL_ALLOW_INTERRUPTS

uint32_t NeoPixel_getAbortedFrames = 0;
uint32_t NeoPixel_getFrames = 0;

// #endif

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
        GPOC = pinMask;
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
    uint32_t ccount;
    __asm__ __volatile__("rsr %0,ccount" : "=a"(ccount));
    return ccount;
}

#if NEOPIXEL_HAVE_BRIGHTHNESS

__attribute__((always_inline)) inline static uint8_t applyBrightness(const uint8_t *&ptr, uint16_t brightness)
{
    if (brightness == 0) {
        ptr++;
        return 0;
    }
    // else if (brightness == 256) {
    //     return *ptr++;
    // }
    return (*ptr++ * brightness) >> 8;
}

#else

__attribute__((always_inline)) inline static uint8_t applyBrightness(const uint8_t *&ptr, uint16_t brightness)
{
    if (brightness == 0) {
        ptr++;
        return 0;
    }
    return *ptr++;
}

#endif

#if DEBUG_RECORD_SERIAL_OUTPUT
bool record_serial_output = true;
String serial_output;
#endif

#if defined(ESP8266)
bool NEOPIXEL_ESPSHOW_FUNC_ATTR espShow(uint8_t pin, uint16_t brightness, const uint8_t *p, const uint8_t *end, uint32_t time0, uint32_t time1, uint32_t period, uint32_t wait, uint32_t pinMask)
#else
bool IRAM_ATTR espShow(uint8_t pin, uint16_t brightness, const uint8_t *p, const uint8_t *end, uint32_t time0, uint32_t time1, uint32_t wait, uint32_t period)
#endif
{
#if NEOPIXEL_USE_PRECACHING
    PRECACHE_START(NeoPixel_espShow);
#endif
    uint32_t c, t;
    uint32_t startTime = 0;
    bool result = true;
#if defined(ESP8266)
    uint32_t invPinMask = ~pinMask;
#else
    constexpr uint32_t invPinMask = 0; // assuming the compiler removes the code
#endif

#if NEOPIXEL_ALLOW_INTERRUPTS
    bool allowInterrupts = (brightness & NeoPixelEx::kNeoPixelNoRetries) == 0;
#endif
    brightness &= 0xff;
    if (brightness) { // change range to 1-256 to avoid divison in applyBrightness()
        brightness++;
    }

    uint8_t pix = applyBrightness(p, brightness);
    uint8_t mask = 0x80;

#if NEOPIXEL_ALLOW_INTERRUPTS
    if (allowInterrupts) {
        NeoPixel_getFrames++;
        ets_intr_lock();
    }
#endif

#if DEBUG_RECORD_SERIAL_OUTPUT
    uint32_t n = 0;
    StreamString output;
#endif

    for (t = time0;; t = time0) {
        if (pix & mask) {
            t = time1; // Bit high duration

        }
#if DEBUG_RECORD_SERIAL_OUTPUT == 1
        if (record_serial_output) {
            output.printf("bit %u(%u)=%u %02x(%u)\n", n, n / 24, (pix & mask) ? 1 : 0, pix, brightness);
            n++;
        }
#endif
#if DEBUG_RECORD_SERIAL_OUTPUT == 2
        if (record_serial_output) {
            if (n % 24 == 0) {
                output.printf("[%u]", (n + 47) / 24);
            }
            output.print((pix & mask) ? 1 : 0);
            n++;
            if (n % 24 == 0) {
                output.print(' ');
            }
            else if (n % 8 == 0)
            {
                output.print('-');
            }
        }
#endif
        while (((c = _getCycleCount()) - startTime) < period)
            ; // Wait for bit start

        gpio_set_level_high(pin, pinMask, invPinMask);
        startTime = c; // Save start time
#if NEOPIXEL_ALLOW_INTERRUPTS
        if (allowInterrupts) {
            ets_intr_unlock();
        }
#endif

        if (!(mask >>= 1)) {
            if (p < end) {
                mask = 0x80; // load next byte
            }
            else {
                mask = 0; // end of frame
            }
        }

        while (((c = _getCycleCount()) - startTime) < t)
            ; // Wait high duration

        if (mask == 0x80) {
            pix =  applyBrightness(p, brightness);
        }

#if NEOPIXEL_ALLOW_INTERRUPTS
        if (allowInterrupts) {
            ets_intr_lock();
            if ((_getCycleCount() - startTime) > period + wait) {

                NeoPixel_getAbortedFrames++;
                result = false;
                period += wait;
                break;
            }
        }
#endif

        gpio_set_level_low(pin, pinMask, invPinMask);
        if (mask == 0) {//} && p >= end) {
            break;
        }

#if NEOPIXEL_ALLOW_INTERRUPTS
        // allow interrupts
        // ets_intr_unlock();
        // ets_intr_lock();
        // if ((_getCycleCount() - startTime) > period - wait) {
        //     // abort if the interrupts took too much time
        //     NeoPixel_getAbortedFrames++;
        //     period += wait;
        //    1 result = false;
        //     break;
        // }
#endif
    }
#if NEOPIXEL_ALLOW_INTERRUPTS
    if (allowInterrupts) {
        ets_intr_unlock();
    }
#endif
    while ((_getCycleCount() - startTime) < period)
        ; // Wait for last bit

#if DEBUG_RECORD_SERIAL_OUTPUT
    if (record_serial_output) {
        record_serial_output = false;
        serial_output = output;
    }
#endif
    return result;
#if NEOPIXEL_USE_PRECACHING
    PRECACHE_END(NeoPixel_espShow);
#endif
}

#if NEOPIXEL_DEBUG_TRIGGER_PIN
static bool _init_toggle();
static bool _toogle = _init_toggle();
static bool _init_toggle() {
    digitalWrite(NEOPIXEL_DEBUG_TRIGGER_PIN, LOW);
    pinMode(NEOPIXEL_DEBUG_TRIGGER_PIN, OUTPUT);
    return false;
}
#endif

bool NeoPixel_espShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint16_t brightness)
{
#if NEOPIXEL_DEBUG_TRIGGER_PIN && 0
    if ((_toogle = !_toogle)) {
        GPOC = _BV(NEOPIXEL_DEBUG_TRIGGER_PIN);
    } else {
        GPOS = _BV(NEOPIXEL_DEBUG_TRIGGER_PIN);
    }
#endif
    const uint8_t *p, *end;
    uint32_t time0, time1, period, reset, pinMask;
#if NEOPIXEL_INTERRUPT_RETRY_COUNT > 0
    const bool doRetry = (brightness & NeoPixelEx::kNeoPixelNoRetries) == 0;
    brightness &= 0xff;
#endif

    #if defined(ESP8266)
        if (pin == 16) {
            pinMask = _BV(0);
        }
        else {
            pinMask = _BV(pin);
        }
    #else
        pinMask = _BV(pin);
    #endif
    p = pixels;
    end = p + numBytes;

    using Timings = NeoPixelEx::DefaultTimings;
    time0 = Timings::kCyclesT0H;
    time1 = Timings::kCyclesT1H;
    period = Timings::kCyclesPeriod;
    reset = Timings::kCyclesRES;

#if NEOPIXEL_DEBUG_TRIGGER_PIN && 1
    if ((_toogle = !_toogle)) {
        GPOC = _BV(NEOPIXEL_DEBUG_TRIGGER_PIN);
    } else {
        GPOS = _BV(NEOPIXEL_DEBUG_TRIGGER_PIN);
    }
#endif
    bool result;
    #if NEOPIXEL_INTERRUPT_RETRY_COUNT > 0
        uint8_t retries = NEOPIXEL_INTERRUPT_RETRY_COUNT;
        do {
    #endif
            ets_intr_lock();
            #if defined(ESP8266)
                result = espShow(pin, brightness, p, end, time0, time1, period, reset, pinMask);
            #else
                result = espShow(pin, brightness, p, end, time0, time1, reset, period);
            #endif
            ets_intr_unlock();
    #if NEOPIXEL_INTERRUPT_RETRY_COUNT > 0
            if (result == true || doRetry == false || retries == 0) {
                break;
            }
            retries--;
            delayMicroseconds(Timings::kCyclesToMicros(reset));
        }
        while(true);
    #endif

    return result;
}
