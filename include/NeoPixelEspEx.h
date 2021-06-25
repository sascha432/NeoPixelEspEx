/**
 * Author: sascha_lammers@gmx.de
 */

// Based on:
// This is a mash-up of the Due show() code + insights from Michael Miller's
// ESP8266 work for the NeoPixelBus library: github.com/Makuna/NeoPixelBus
// and
// https://github.com/adafruit/Adafruit_NeoPixel/blob/master/esp8266.c


/*

- support for dynamic CPU speed (ESP8266 80/160MHHz) with runtime switching
- support for ESP8266/GPIO16
- support for ESP32
- option for ESP8266 to use precaching instead of IRAM
- support for brightness scaling
- support for uint8_t, NeoPixelGRB and custom types
- support for interrupts and retrying if interrupted (ESP8266)
- function to clear pixels without using any memory, for example during boot, restart, crash...
- safety functions to protect LEDs and controller during shutdown

examples:

bright red, green, blue + 7 low intesity red pixels

#define WS2812_OUTPUT_PIN 12

NeoPixelGRB pixels[10] = { NeoPixelGRB(0xff0000), NeoPixelGRB(0x00ff00), NeoPixelGRB(0, 0, 255) };
template<uint16_t _First>
constexpr auto _startPtr = reinterpret_cast<uint8_t *>(&pixels[_First]);
constexpr auto _endPtr = reinterpret_cast<uint8_t *>(&pixels[sizeof(pixels) / sizeof(NeoPixelGRB)]);
constexpr auto kLengthInBytes = _endPtr - _startPtr;

NeoPixel_fillColor(&pixels[3], kLengthInBytes, 0x100000);
NeoPixel_espShow(WS2812_OUTPUT_PIN, pixels, sizeof(pixels));

NeoPixel_espClear(WS2812_OUTPUT_PIN)


// clear all pixel strips that have been defined
// and reset PINs to input
NeoPixel_clearStrips();

*/

#pragma once

#include <Arduino.h>
#include <array>

// enable the brightness scaling
#ifndef NEOPIXEL_HAVE_BRIGHTHNESS
#   define NEOPIXEL_HAVE_BRIGHTHNESS 1
#endif

#if defined(ESP8266)
// allow interrupts during the output. recommended for more than a couple pixels
// interrupts that take too long will abort the current frame and increment NeoPixel_getAbortedFrames
#   ifndef NEOPIXEL_ALLOW_INTERRUPTS
#   define NEOPIXEL_ALLOW_INTERRUPTS 0
#   endif
// use preching instead of IRAM
#   ifndef NEOPIXEL_USE_PRECACHING
#       define NEOPIXEL_USE_PRECACHING 1
#   endif
#   if NEOPIXEL_USE_PRECACHING
#       define NEOPIXEL_ESPSHOW_FUNC_ATTR PRECACHE_ATTR
#   else
#       define NEOPIXEL_ESPSHOW_FUNC_ATTR IRAM_ATTR
#   endif
#else
#   define NEOPIXEL_USE_PRECACHING 0
#endif

#if NEOPIXEL_ALLOW_INTERRUPTS
// retry if a frame got aborted
#   ifndef NEOPIXEL_INTERRUPT_RETRY_COUNT
#       define NEOPIXEL_INTERRUPT_RETRY_COUNT 2
#   endif
#elif defined(NEOPIXEL_INTERRUPT_RETRY_COUNT) && (NEOPIXEL_INTERRUPT_RETRY_COUNT > 0)
#   error NEOPIXEL_INTERRUPT_RETRY_COUNT must be 0 if NEOPIXEL_ALLOW_INTERRUPTS is disabled
#else
#   define NEOPIXEL_INTERRUPT_RETRY_COUNT 0
#endif

#ifndef NEOPIXEL_SUPPORT_SET_CPU_FREQ
#   if defined(ESP8266)
#       warning NEOPIXEL_SUPPORT_SET_CPU_FREQ not set, using default value
#       define NEOPIXEL_SUPPORT_SET_CPU_FREQ 1
#   else
#       define NEOPIXEL_SUPPORT_SET_CPU_FREQ 0
#   endif
#endif

// default timings for NeoPixel_espShow()
#ifndef NEOPIXEL_CHIPSET
#   define NEOPIXEL_CHIPSET NeoPixelEx::TimingsWS2812<F_CPU>
#endif

#define DEBUG_RECORD_SERIAL_OUTPUT 0

#if DEBUG_RECORD_SERIAL_OUTPUT
extern bool record_serial_output;
extern String serial_output;
#endif

// toggle this pin when calling show()
#ifndef NEOPIXEL_DEBUG_TRIGGER_PIN
#   define NEOPIXEL_DEBUG_TRIGGER_PIN 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

// exported legacy C functions to be compatible with old code

// #if NEOPIXEL_ALLOW_INTERRUPTS
extern uint32_t NeoPixel_getAbortedFrames;
extern uint32_t NeoPixel_getFrames;
// #endif

// deprecated legacy function
// fill pixel data
// numBytes is 3 per pixel
// color is 24 bit RGB
void NeoPixel_fillColor(uint8_t *pixels, uint16_t numBytes, uint32_t color);

// show pixels
// numBytes is 3 per pixel
// brightness 0-255
// can be used with (brightness|kNeoPixelNoRetries) if NEOPIXEL_ALLOW_INTERRUPTS == 1 and NEOPIXEL_INTERRUPT_RETRY_COUNT > 0 to disable retries
bool NeoPixel_espShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint16_t brightness = 255);

#ifdef __cplusplus
}

namespace NeoPixelEx {

    // no retries bit for brightness
    static constexpr uint16_t kNeoPixelNoRetries = 0x8000;

    // timits in nano seconds
    template<uint32_t _T0H, uint32_t _T1H, uint32_t _TPeriod, uint32_t _TReset, uint32_t _FCpu/*Hz or MHz*/>
    class Timings {
    public:
        using type = Timings<_T0H, _T1H, _TPeriod, _TReset, _FCpu>;

    public:
        static constexpr uint16_t kFCpu = _FCpu < 32768 ? _FCpu : _FCpu / 1000000UL;

        static constexpr uint32_t kMicrosToCycles(uint16_t time) {
            return kFCpu * time;
        }

        static constexpr uint32_t kCyclesToMicros(uint16_t cycles) {
            return cycles * 1000UL / kFCpu;

        }
        static constexpr uint32_t kNanosToCycles(uint16_t time) {
            return kFCpu * time / 1000UL;
        }

        static constexpr uint32_t kCyclesT0H = kNanosToCycles(_T0H);
        static constexpr uint32_t kCyclesT1H = kNanosToCycles(_T1H);
        static constexpr uint32_t kCyclesPeriod = kNanosToCycles(_TPeriod);
        static constexpr uint32_t kCyclesRES = kNanosToCycles(_TReset);
        static constexpr uint32_t kCyclesMaxWait = kNanosToCycles((_TPeriod + _TReset) * 1250 / 100);

        inline static uint32_t _panic() {
            // dynamic CPU frequency must be 1/2 or 2 times of kFCpu
            panic();
            return 0;
        }

        // FCpu in MHz
        inline static uint32_t shiftCycles(uint32_t cycles, uint16_t FCpu) {
            return FCpu == _FCpu ? cycles : (FCpu == (kFCpu << 1)) ? (cycles << 1) : ((FCpu << 1) == kFCpu) ? (cycles >> 1) : _panic();
        }

        inline static uint32_t getCyclesT0H(uint16_t FCpu) {
            return shiftCycles(kNanosToCycles(_T0H), FCpu);
        }

        inline static uint32_t getCyclesT1H(uint16_t FCpu) {
            return shiftCycles(kNanosToCycles(_T1H), FCpu);
        }

        inline static uint32_t getCyclesPeriod(uint16_t FCpu) {
            return shiftCycles(kNanosToCycles(_TPeriod), FCpu);
        }

        inline static uint32_t getCyclesRES(uint16_t FCpu) {
            return shiftCycles(kNanosToCycles(_TReset), FCpu);
        }
    };


    template<uint32_t _FCpu>
    using TimingsWS2811 = Timings<500, 1200, 2500, 250, _FCpu>;

    template<uint32_t _FCpu>
    using TimingsWS2812 = Timings<400, 800, 1250, 250, _FCpu>;

    template<uint32_t _FCpu>
    using TimingsWS2813 = Timings<350, 750, 1250, 250, _FCpu>;

    using DefaultTimings = NEOPIXEL_CHIPSET;

    class GRBType
    {
    public:
        GRBType() :
            g(0),
            r(0),
            b(0)
        {
        }

        GRBType(uint32_t rgb) :
            g(static_cast<uint8_t>(rgb >> 8)),
            r(static_cast<uint8_t>(rgb >> 16)),
            b(static_cast<uint8_t>(rgb))
        {
        }

        GRBType(uint8_t red, uint8_t green, uint8_t blue) :
            g(green),
            r(red),
            b(blue)
        {
        }

    public:
        uint8_t g;
        uint8_t r;
        uint8_t b;
    };

    class RGBType
    {
    public:
        RGBType() :
            r(0),
            g(0),
            b(0)
        {
        }

        RGBType(uint32_t rgb) :
            r(static_cast<uint8_t>(rgb >> 16)),
            g(static_cast<uint8_t>(rgb >> 8)),
            b(static_cast<uint8_t>(rgb))
        {
        }

        RGBType(uint8_t red, uint8_t green, uint8_t blue) :
            r(red),
            g(green),
            b(blue)
        {
        }

    public:
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    template<typename _Type>
    class Color : public _Type{
    public:
        using type = _Type;

    public:
        using type::type;
        using type::r;
        using type::g;
        using type::b;

        type operator==(uint32_t color) {
            r = static_cast<uint8_t>(color >> 16);
            g = static_cast<uint8_t>(color >> 8);
            b = static_cast<uint8_t>(color);
        }

        operator int() const {
            return toRGB();
        }

        uint32_t toRGB() const {
            return (r << 16) | (g << 8) | b;
        }

        String toString() const {
            char buf[10];
            snprintf_P(buf, sizeof(buf), PSTR("#%06x"), toRGB());
            return buf;
        }

        // rotate left
        type &operator <<=(int num) {
            auto color = toRGB();
            *this = (color >> (24 - num)) | (color << num);
            return *this;
        }

        // rotate right
        type &operator >>=(int num) {
            auto color = toRGB();
            *this = ((color & ((1 << num) - 1)) << (24 - num)) | (color >> num);
            return *this;
        }
    };

    using GRB = Color<GRBType>;
    using RGB = Color<RGBType>;

    template<uint16_t _NumPixels, typename _PixelType = GRB>
    class Array : public std::array<_PixelType, _NumPixels>
    {
    public:
        using type = std::array<_PixelType, _NumPixels>;
        using pixel_type = _PixelType;
        using type::data;
        using type::size;
        using type::begin;
        using type::end;

        static constexpr uint16_t kNumPixels = _NumPixels;
        static constexpr uint16_t kNumBytes = kNumPixels * sizeof(pixel_type);

        static constexpr uint16_t kGetNumBytes(const uint16_t offset = 0, const uint16_t num = kNumPixels) {
            return (num - offset) * sizeof(pixel_type);
        }

        constexpr uint16_t getNumBytes() const {
            return kNumBytes;
        }

        constexpr uint16_t getNumPixels() const {
            return kNumPixels;
        }

        explicit operator const pixel_type *() const {
            return reinterpret_cast<const pixel_type *>(data());
        }

        explicit operator pixel_type *() {
            return reinterpret_cast<pixel_type *>(data());
        }

        operator const uint8_t *() const {
            return reinterpret_cast<const uint8_t *>(data());
        }

        operator uint8_t *() {
            return reinterpret_cast<uint8_t *>(data());
        }

        inline void fill(uint32_t color) {
            std::fill(begin(), end(), pixel_type(color));
        }

        inline void fill(const pixel_type &color) {
            std::fill(begin(), end(), color);
        }

        inline void fill(uint16_t numPixels, uint32_t color) {
            std::fill_n(data(), numPixels, pixel_type(color));
        }

        inline void fill(uint16_t numPixels, const pixel_type &color) {
            std::fill_n(data(), numPixels, color);
        }

        inline void fill(uint16_t offset, uint16_t numPixels, uint32_t color) {
            std::fill(data() + offset, data() + numPixels, pixel_type(color));
        }

        inline void fill(uint16_t offset, uint16_t numPixels, const pixel_type &color) {
            std::fill(data() + offset, data() + numPixels, color);
        }
    };

    // force to clear all pixels without interruptions
    template<typename _Chipset = NEOPIXEL_CHIPSET>
    inline static void forceClear(uint8_t pin, uint16_t numBytes)
    {
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);
        delayMicroseconds(_Chipset::kCyclesToMicros(_Chipset::kCyclesMaxWait));
        #if defined(ESP8266)
            ets_intr_lock();
            ets_intr_lock();
        #endif
        uint8_t buf[1];
        NeoPixel_espShow(pin, buf, numBytes, kNeoPixelNoRetries);
        #if defined(ESP8266)
            ets_intr_unlock();
            ets_intr_unlock();
        #endif
    }

    template<uint8_t _OutputPin, uint16_t _NumPixels, typename _PixelType = GRB, typename _Chipset = TimingsWS2812<F_CPU>>
    class Strip : public Array<_NumPixels, _PixelType>
    {
    public:
        static constexpr auto kOutputPin = _OutputPin;
        using chipset_type = _Chipset;

    public:
        using type = Array<_NumPixels, _PixelType>;
        using type::data;
        using type::fill;
        using type::getNumBytes;
        using type::getNumPixels;

        __attribute__((always_inline)) inline void begin() {
            digitalWrite(kOutputPin, LOW);
            pinMode(kOutputPin, OUTPUT);
        }

        __attribute__((always_inline)) inline void end() {
            clear();
            digitalWrite(kOutputPin, LOW);
            pinMode(kOutputPin, INPUT);
        }

        __attribute__((always_inline)) inline void clear() {
            fill(0);
            _clear(getNumBytes());
        }

        __attribute__((always_inline)) inline void show(uint8_t brightness) {
            NeoPixel_espShow(kOutputPin, reinterpret_cast<uint8_t *>(data()), getNumBytes(), brightness);
        }

        __attribute__((always_inline)) inline void show() {
            show(255);
        }

        // this method allows to clear any number of pixels
        __attribute__((always_inline)) bool _clear(uint16_t numBytes)
        {
            uint8_t buf[1];
            return NeoPixel_espShow(kOutputPin, buf, numBytes, 0);
        }
    };

}

#endif
