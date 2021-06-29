/**
 * Author: sascha_lammers@gmx.de
 */

#pragma once

#include <Arduino.h>
#include <array>
#if defined(ESP8266)
#include <user_interface.h>
#endif

// enable debug mode
#ifndef NEOPIXEL_DEBUG
#define NEOPIXEL_DEBUG 1
#endif

// enable the brightness scaling
#ifndef NEOPIXEL_HAVE_BRIGHTHNESS
#   define NEOPIXEL_HAVE_BRIGHTHNESS 1
#endif

// enable simple stats about frames, dropped frames and fps
#ifndef NEOPIXEL_HAVE_STATS
#   define NEOPIXEL_HAVE_STATS 1
#endif

#if defined(ESP8266)
// allow interrupts during the output. recommended for more than a couple pixels
// interrupts that take too long will abort the current frame and increment NeoPixel_getAbortedFrames
#   ifndef NEOPIXEL_ALLOW_INTERRUPTS
#   define NEOPIXEL_ALLOW_INTERRUPTS 1
#   endif
// use preching instead of IRAM
//
// precaching adds extra overhead to each show() call
//
// IRAM usage
// NEOPIXEL_ALLOW_INTERRUPTS=0 212 byte
// NEOPIXEL_ALLOW_INTERRUPTS=1 316 byte
//
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

#define NEOPIXEL_CHIPSET_WS2811 NeoPixelEx::TimingsWS2811<F_CPU>
#define NEOPIXEL_CHIPSET_WS2812 NeoPixelEx::TimingsWS2812<F_CPU>
#define NEOPIXEL_CHIPSET_WS2813 NeoPixelEx::TimingsWS2813<F_CPU>

// default timings for NeoPixel_espShow()
#ifndef NEOPIXEL_CHIPSET
#   define NEOPIXEL_CHIPSET NEOPIXEL_CHIPSET_WS2812
#endif

// toggle this pin when calling show()
#ifndef NEOPIXEL_DEBUG_TRIGGER_PIN
#   define NEOPIXEL_DEBUG_TRIGGER_PIN -1
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool espShow(uint8_t pin, uint16_t brightness, const uint8_t *p, const uint8_t *end, uint32_t time0, uint32_t time1, uint32_t period, uint32_t wait, uint32_t minWaitPeriod, uint32_t pinMask, uint32_t *lastDisplayTime);

// deprecated legacy function
// fill pixel data
// numBytes is 3 per pixel
// color is 24 bit RGB
void NeoPixel_fillColor(uint8_t *pixels, uint16_t numBytes, uint32_t color);

// show pixels
// numBytes is 3 per pixel
// brightness 0-255
bool NeoPixel_espShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint16_t brightness = 255);

#ifdef __cplusplus
}

namespace NeoPixelEx {

    // no retries bit for brightness
    #if NEOPIXEL_DEBUG_TRIGGER_PIN==16
        #error GPIO16 not supported
    #elif NEOPIXEL_DEBUG_TRIGGER_PIN>=0
    extern bool _toogleDebugFlag;

    __attribute__((always_inline)) inline bool NeoPixel_initToggleDebugPin()
    {
        digitalWrite(NEOPIXEL_DEBUG_TRIGGER_PIN, LOW);
        pinMode(NEOPIXEL_DEBUG_TRIGGER_PIN, OUTPUT);
        NeoPixelEx::_toogleDebugFlag = false;
        return false;
    }

    __attribute__((always_inline)) inline void NeoPixel_toggleDebugPin()
    {
        if ((_toogleDebugFlag = !_toogleDebugFlag)) {
            GPOC = _BV(NEOPIXEL_DEBUG_TRIGGER_PIN);
        } else {
            GPOS = _BV(NEOPIXEL_DEBUG_TRIGGER_PIN);
        }
    }
    #else
    __attribute__((always_inline)) inline bool NeoPixel_initToggleDebugPin() {
        return false;
    }

    __attribute__((always_inline)) inline void NeoPixel_toggleDebugPin() {
    }
    #endif


    // timits in nano seconds
    template<uint32_t _T0H, uint32_t _T1H, uint32_t _TPeriod, uint32_t _TReset, uint32_t _FCpu/*Hz or MHz*/>
    class Timings {
    public:
        using type = Timings<_T0H, _T1H, _TPeriod, _TReset, _FCpu>;
        using cpu_frequency_t = uint8_t;

    public:
        static constexpr cpu_frequency_t kFCpu = _FCpu < 32768 ? _FCpu : _FCpu / 1000000UL;

        constexpr Timings() {}

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
        static constexpr uint32_t kMinDisplayPeriod = _TPeriod + _TReset;

        static constexpr uint32_t getCyclesT0H() {
            return kNanosToCycles(_T0H);
        }

        static constexpr uint32_t getCyclesT1H() {
            return kNanosToCycles(_T1H);
        }

        static constexpr uint32_t getCyclesPeriod() {
            return kNanosToCycles(_TPeriod);
        }

        static constexpr uint32_t getCyclesRES() {
            return kNanosToCycles(_TReset);
        }

        static constexpr uint32_t getMinDisplayPeriod() {
            return kMinDisplayPeriod;
        }
    };


    template<uint32_t _FCpu>
    using TimingsWS2811 = Timings<500, 1200, 2500, 50, _FCpu>;

    template<uint32_t _FCpu>
    using TimingsWS2812 = Timings<400, 800, 1250, 50, _FCpu>;

    template<uint32_t _FCpu>
    using TimingsWS2813 = Timings<350, 750, 1250, 250, _FCpu>;

    using DefaultTimings = NEOPIXEL_CHIPSET;

    class Stats {
    public:
        Stats() : _start(millis64()), _frames(0)
        {
        }

        void clear() {
            *this = Stats();
        }

        uint32_t getFrames() const {
            return _frames;
        }

        float getFps() const {
            return (_frames * 1000UL) / static_cast<float>(getTime());
        }

        // time since last reset in milliseconds
        uint32_t getTime() const {
            return millis64() - _start;
        }

    public:
        uint32_t &__frames() {
            return _frames;
        }

    protected:
        uint64_t millis64() const {
            return micros64() / 1000;
        }

    private:
        uint64_t _start;
        uint32_t _frames;

    public:
    #if NEOPIXEL_ALLOW_INTERRUPTS

        __attribute__((always_inline)) uint32_t getAbortedFrames() const {
            return _aborted;
        }

        static constexpr bool allowInterrupts() {
            return true;
        }

    public:
        __attribute__((always_inline)) uint32_t &__aborted() {
            return _aborted;
        }

    private:
        uint32_t _aborted{0};
    #else

        static constexpr uint32_t getAbortedFrames() {
            return 0;
        }

        static constexpr bool allowInterrupts() {
            return false;
        }

    #endif
    };

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

        uint8_t red() const {
            return r;
        }

        uint8_t green() const {
            return g;
        }

        uint8_t blue() const {
            return b;
        }

        uint8_t &red() {
            return r;
        }

        uint8_t &green() {
            return g;
        }

        uint8_t &blue() {
            return b;
        }

        Color inverted() const {
            return ~toRGB() & 0xffffffUL;
        }

        void invert() {
            *this = inverted();
        }

        void setBrightness(uint8_t brightness) {
            if (brightness == 0) {
                *this = 0;
            }
            else {
                brightness++;
                red() = (red() * brightness) >> 8;
                green() = (green() * brightness) >> 8;
                blue() = (blue() * brightness) >> 8;
            }
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

        inline void set(int index, pixel_type color) {
            data()[index] = color;
        }

        inline pixel_type get(int index) const {
            return data()[index];
        }

        inline pixel_type &operator[](int index) {
            return data()[index];
        }

        inline pixel_type operator[](int index) const {
            return data()[index];
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


    template<typename _Chipset = NEOPIXEL_CHIPSET>
    inline bool internalShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint8_t brightness, uint32_t *lastDisplayTime)
    {
    #if 0
        NeoPixel_toggleDebugPin();
    #endif
        bool result = false;
        auto p = pixels;
        auto end = p + numBytes;

        #if NEOPIXEL_INTERRUPT_RETRY_COUNT > 0
            uint8_t retries = NEOPIXEL_INTERRUPT_RETRY_COUNT;
            do {
        #endif
                #if defined(ESP8266) && !NEOPIXEL_ALLOW_INTERRUPTS
                    ets_intr_lock();
                #endif
                result = ::espShow(pin,
                    brightness,
                    p,
                    end,
                    _Chipset::getCyclesT0H(),
                    _Chipset::getCyclesT1H(),
                    _Chipset::getCyclesPeriod(),
                    _Chipset::getCyclesRES(),
                    _Chipset::getMinDisplayPeriod(),
                    pin == 16 ? _BV(0) : _BV(pin),
                    lastDisplayTime
                );

                #if defined(ESP8266) && !NEOPIXEL_ALLOW_INTERRUPTS
                    ets_intr_unlock();
                #endif
        #if NEOPIXEL_INTERRUPT_RETRY_COUNT > 0
                if (result == true || lastDisplayTime == nullptr || retries == 0) {
                    break;
                }
                retries--;
                uint32_t diff = micros() - *lastDisplayTime;
                if (diff < _Chipset::kMinDisplayPeriod) {
                    delayMicroseconds(_Chipset::kMinDisplayPeriod - diff);
                }
            }
            while(true);
        #endif

        return result;
    }

    // force to clear all pixels without interruptions
    template<typename _Chipset = NEOPIXEL_CHIPSET>
    inline void forceClear(uint8_t pin, uint16_t numBytes)
    {
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);
        uint8_t buf[1];
        uint32_t start = micros();
        while(micros() - start < DefaultTimings::kMinDisplayPeriod) {
        }
        #if defined(ESP8266) && NEOPIXEL_ALLOW_INTERRUPTS
            ets_intr_lock();
        #endif
        internalShow<_Chipset>(pin, buf, numBytes, 0, nullptr);
        #if defined(ESP8266) && NEOPIXEL_ALLOW_INTERRUPTS
            ets_intr_unlock();
        #endif
    }

    #if NEOPIXEL_HAVE_STATS
        NeoPixelEx::Stats &getStats();
    #endif

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
        using type::get;
        using type::set;
        using type::operator[];

        Strip() : _lastDisplayTime(micros()) {
        }

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

        __attribute__((always_inline)) inline void show(uint8_t brightness = 255) {
            _checkTime();
            internalShow(kOutputPin, reinterpret_cast<uint8_t *>(data()), getNumBytes(), brightness, &_lastDisplayTime);
        }

        // this method allows to clear any number of pixels
        __attribute__((always_inline)) void _clear(uint16_t numBytes)
        {
            _checkTime();
            uint8_t buf[1];
            internalShow(kOutputPin, buf, numBytes, 0, &_lastDisplayTime);
        }

        __attribute__((always_inline)) bool canShow() const {
            return (micros() > _lastDisplayTime + _Chipset::kMinDisplayPeriod);
        }

    private:
        __attribute__((always_inline)) void _checkTime() const {
            uint32_t diff = micros() - _lastDisplayTime;
            if (diff < _Chipset::kMinDisplayPeriod) {
                delayMicroseconds(_Chipset::kMinDisplayPeriod - diff);
            }
        }

    private:
        uint32_t _lastDisplayTime;
    };

    #if NEOPIXEL_HAVE_STATS

        extern Stats stats;

        __attribute__((always_inline)) inline NeoPixelEx::Stats &getStats()
        {
            return stats;
        }

    #endif

}

extern "C" {

    inline bool NeoPixel_espShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint16_t brightness)
    {
        uint32_t lastDisplayTime = micros() - 0xffff;
        return NeoPixelEx::internalShow<NeoPixelEx::DefaultTimings>(pin, pixels, numBytes, brightness, &lastDisplayTime);
    }

}

#endif
