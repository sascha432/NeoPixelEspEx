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
#define NEOPIXEL_DEBUG 0
#endif

// enable the brightness scaling
#ifndef NEOPIXEL_HAVE_BRIGHTNESS
#   define NEOPIXEL_HAVE_BRIGHTNESS 1
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
#elif ESP32
#   define NEOPIXEL_ESPSHOW_FUNC_ATTR IRAM_ATTR
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

#define NEOPIXEL_CHIPSET_WS2811 NeoPixelEx::TimingsWS2811
#define NEOPIXEL_CHIPSET_WS2812 NeoPixelEx::TimingsWS2812
#define NEOPIXEL_CHIPSET_WS2813 NeoPixelEx::TimingsWS2813

// default timings for NeoPixel_espShow()
#ifndef NEOPIXEL_CHIPSET
#   define NEOPIXEL_CHIPSET NEOPIXEL_CHIPSET_WS2812
#endif

// toggle this pin when calling show()
#ifndef NEOPIXEL_DEBUG_TRIGGER_PIN
#   define NEOPIXEL_DEBUG_TRIGGER_PIN -1
#endif

// toggle this pin when the show() method was interrupted
#ifndef NEOPIXEL_DEBUG_TRIGGER_PIN2
#   define NEOPIXEL_DEBUG_TRIGGER_PIN2 -1
#endif

#ifndef __CONSTEXPR17
#   if __GNUC__ >= 10
#       define __CONSTEXPR17 constexpr
#   else
#       define __CONSTEXPR17
#   endif
#endif

#pragma GCC push_options
#pragma GCC optimize ("O3")

#ifdef __cplusplus
extern "C" {
#endif

// void NeoPixel_fillColorGRB(void *pixels, uint16_t numBytes, const NeoPixelEx::GRB &color);

// deprecated legacy function
// fill pixel data
// numBytes is 3 per pixel
// color is 24 bit GRB
// void NeoPixel_fillColor(uint8_t *pixels, uint16_t numBytes, uint32_t color);

// show pixels
// numBytes is 3 per pixel
// brightness 0-255
// context is a pointer to the context object of the LED strip. nullptr will use the shared global object
bool NeoPixel_espShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint16_t brightness = 255, void *context = nullptr);

#ifdef __cplusplus
}

namespace NeoPixelEx {

    // timits in nano seconds
    template<uint32_t _T0H, uint32_t _T1H, uint32_t _TPeriod, uint32_t _TReset, uint32_t _MinDisplayPeriod, uint32_t _FCpu/*Hz or MHz*/>
    class Timings {
    public:
        using type = Timings<_T0H, _T1H, _TPeriod, _TReset, _MinDisplayPeriod, _FCpu>;
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
        static constexpr uint32_t kMinDisplayPeriod = _MinDisplayPeriod;

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
    using _TimingsWS2811 = Timings<500, 1200, 2500, 50, 2750, _FCpu>;
    using TimingsWS2811 = _TimingsWS2811<F_CPU>;

    template<uint32_t _FCpu>
    using _TimingsWS2812 = Timings<400, 800, 1250, 50, 1275, _FCpu>;
    // using _TimingsWS2812 = Timings<400, 800, 1250, 50, _FCpu>;
    using TimingsWS2812 = _TimingsWS2812<F_CPU>;

    template<uint32_t _FCpu>
    using _TimingsWS2813 = Timings<350, 750, 1250, 250, 1500, _FCpu>;
    using TimingsWS2813 = _TimingsWS2813<F_CPU>;

    using DefaultTimings = NEOPIXEL_CHIPSET;

    class Stats {
    public:
        void clear() {
            *this = Stats();
        }

        uint32_t getFrames() const {
            return _frames;
        }

        uint16_t getFps() const {
            auto time = getTime();
            return time ? (_frames * 1000UL) / getTime() : 0;
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
            #if ESP32
                return esp_timer_get_time() / 1000;
            #elif ESP8266
                return micros64() / 1000;
            #endif
        }

    private:
        uint64_t _start;
        uint32_t _frames;

    public:
    #if NEOPIXEL_ALLOW_INTERRUPTS

        __attribute__((always_inline)) inline Stats() : _start(millis64()), _frames(0), _aborted(0)
        {
        }

        __attribute__((always_inline)) inline uint32_t getAbortedFrames() const {
            return _aborted;
        }

        static constexpr bool allowInterrupts() {
            return true;
        }

        __attribute__((always_inline)) inline void increment(bool success) {
            _frames++;
            if (success) {
                return;
            }
            _aborted++;
        }

    private:
        uint32_t _aborted;
    #else

    public:
        __attribute__((always_inline)) inline Stats() : _start(millis64()), _frames(0)
        {
        }

        __attribute__((always_inline)) inline void increment(bool success) {
            _frames++;
        }

        static constexpr uint32_t getAbortedFrames() {
            return 0;
        }

        static constexpr bool allowInterrupts() {
            return false;
        }

    #endif
    };

    class DebugContext {
    public:
    static constexpr uint8_t kInvalidPin = 0xff;

    public:
        DebugContext(uint8_t pin = kInvalidPin, uint8_t pin2 = kInvalidPin) :
            _pin(pin),
            _pin2(pin2),
            _states{},
            _enabled(false)
        {
            begin();
        }

        void begin(uint8_t pin, uint8_t pin2 = kInvalidPin) {
            end();
            _pin = pin;
            _pin2 = pin2;
            begin();
        }

        void begin() {
            end();
            auto *ptr = _states;
            uint8_t pins[] = { _pin, _pin2 };
            for(auto pin: pins) {
                if (pin == kInvalidPin) {
                    continue;
                }
                digitalWrite(pin, *ptr++);
                pinMode(pin, OUTPUT);
                _enabled = true;
            }
        }

        void end() {
            std::fill_n(_states, sizeof(_states), false);
            _enabled = false;
            auto *ptr = _states;
            uint8_t pins[] = { _pin, _pin2 };
            for(auto &pin: pins) {
                if (pin == kInvalidPin) {
                    continue;
                }
                digitalWrite(pin, *ptr++);
                pinMode(pin, INPUT);
                pin = kInvalidPin;
            }
        }

        void togglePin() {
            auto &state = _states[0];
            if (_enabled) {
                state = !state;
                #if defined(ESP8266)
                    if (state) {
                        GPOC = _BV(_pin);
                    }
                    else {
                        GPOS = _BV(_pin);
                    }
                #else
                    digitalWrite(_pin, state);
                #endif
            }
        }

        void togglePin2() {
            auto &state = _states[1];
            if (_enabled) {
                state = !state;
                #if defined(ESP8266)
                    if (state) {
                        GPOC = _BV(_pin2);
                    }
                    else {
                        GPOS = _BV(_pin2);
                    }
                #else
                    digitalWrite(_pin2, state);
                #endif
            }
        }

    private:
        bool &_getState(uint8_t n) {
            return _states[n];
        }

    private:
        uint8_t _pin;
        uint8_t _pin2;
        bool _states[4];
        bool _enabled;
    };

    class Context {
    public:
        Context() :
            #if NEOPIXEL_DEBUG
                #if (NEOPIXEL_DEBUG_TRIGGER_PIN >= 0)
                    _debug(NEOPIXEL_DEBUG_TRIGGER_PIN, NEOPIXEL_DEBUG_TRIGGER_PIN2),
                #endif
            #endif
            _lastDisplayTime(0)
        {
        }

        uint32_t &getLastDisplayTime() {
            return _lastDisplayTime;
        }

        uint32_t getLastDisplayTime() const {
            return _lastDisplayTime;
        }

        void setLastDisplayTime(uint32_t micros) {
            _lastDisplayTime = micros;
        }

        void waitRefreshTime(uint32_t minWaitPeriod) {
            uint32_t diff = micros() - getLastDisplayTime();
            if (diff < minWaitPeriod) {
                delayMicroseconds(minWaitPeriod - diff);
            }
        }

        #if NEOPIXEL_HAVE_STATS
            Stats &getStats() {
                return _stats;
            }
        #endif

        #if NEOPIXEL_DEBUG
            DebugContext &getDebugContext() {
                return _debug;
            }
        #endif

        // returns global context if contextPtr is nullptr
        static Context &validate(void *contextPtr);

    private:
        #if NEOPIXEL_HAVE_STATS
            Stats _stats;
        #endif
        #if NEOPIXEL_DEBUG
            DebugContext _debug;
        #endif
        uint32_t _lastDisplayTime;
    };

    struct GRBOrder {
        __attribute__((always_inline)) inline static uint8_t get(const uint8_t *ptr, const uint8_t ofs) {
            switch(ofs) {
                case 0:
                    return *ptr;
                case 1:
                    return ptr[1];
                case 2:
                    return ptr[-1];
                default:
                    break;
            }
            return 0;
        }
    };

    struct RGBOrder {
        __attribute__((always_inline)) inline static uint8_t get(const uint8_t *ptr, const uint8_t ofs) {
            return *ptr;
        }
    };

    class GRBType
    {
    public:
        using OrderType = RGBOrder;

        static constexpr bool kReOrder = false;

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
        using OrderType = RGBOrder;

        static constexpr bool kReOrder = false;

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

    class CRGBType
    {
    public:
        using OrderType = GRBOrder;

        static constexpr bool kReOrder = true;

    public:
        CRGBType() :
            r(0),
            g(0),
            b(0)
        {
        }

        CRGBType(uint32_t rgb) :
            r(static_cast<uint8_t>(rgb >> 16)),
            g(static_cast<uint8_t>(rgb >> 8)),
            b(static_cast<uint8_t>(rgb))
        {
        }

        CRGBType(uint8_t red, uint8_t green, uint8_t blue) :
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
    class Color : public _Type {
    public:
        using type = _Type;
        using OrderType = typename _Type::OrderType;

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

        uint32_t toGRB() const {
            return (r << 8) | (g << 16) | b;
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

        Color scale(uint8_t brightness) const {
            if (brightness == 0) {
                return Color(0);
            }
            brightness++;
            return Color((red() * brightness) >> 8, (green() * brightness) >> 8, (blue() * brightness) >> 8);
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
    using CRGB = Color<CRGBType>;

    // wrapper for any raw pointer
    template<size_t _NumElements, typename _PixelType = GRB>
    class DataWrapper {
    public:
        DataWrapper(void *data) : _data(reinterpret_cast<_PixelType *>(data)) {}

        static constexpr size_t size() {
            return _NumElements;
        }

        void fill(_PixelType color) {
            std::fill_n(reinterpret_cast<_PixelType *>(_data), size(), color);
        }

        _PixelType *data() {
            return _data;
        }

        const _PixelType *data() const {
            return _data;
        }

        _PixelType *begin() {
            return _data;
        }

        const _PixelType *begin() const {
            return _data;
        }

        _PixelType *end() {
            return &_data[size()];
        }

        const _PixelType *end() const {
            return &_data[size()];
        }

        _PixelType &operator[](int index) {
            return _data[index];
        }

        _PixelType operator[](int index) const {
            return _data[index];
        }

    private:
        _PixelType *_data;
    };

    template<uint16_t _NumPixels, typename _PixelType = GRB, typename _DataType = std::array<_PixelType, _NumPixels>>
    class PixelData
    {
    public:
        using data_type = std::array<_PixelType, _NumPixels>;
        using pixel_type = _PixelType;

        static constexpr uint16_t kNumPixels = _NumPixels;
        static constexpr uint16_t kNumBytes = kNumPixels * sizeof(pixel_type);

    public:
        PixelData() {}

        template<typename ..._Args>
        PixelData(_Args &&...args) : _data(std::forward<_Args &&>(args)...) {}

        static  constexpr uint16_t getNumBytes() {
            return kNumBytes;
        }

        static constexpr uint16_t getNumPixels() {
            return kNumPixels;
        }

        static constexpr uint16_t size() {
            return kNumPixels;
        }

        pixel_type *data() {
            return _data.data();
        }

        const pixel_type *data() const {
            return _data.data();
        }

        pixel_type *begin() {
            return data();
        }

        const pixel_type *begin() const {
            return data();
        }

        pixel_type *end() {
            return &data()[_data.size()];
        }

        const pixel_type *end() const {
            return data()[_data.size()];
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

        void set(int index, pixel_type color) {
            data()[index] = color;
        }

        pixel_type get(int index) const {
            return data()[index];
        }

        pixel_type &operator[](int index) {
            return data()[index];
        }

        pixel_type operator[](int index) const {
            return data()[index];
        }

        // color is RGB
        void fill(uint32_t color) {
            std::fill(begin(), end(), pixel_type(color));
        }

        void fill(const pixel_type &color) {
            std::fill(begin(), end(), color);
        }

        // color is RGB
        void fill(uint16_t numPixels, uint32_t color) {
            std::fill_n(data(), numPixels, pixel_type(color));
        }

        void fill(uint16_t numPixels, const pixel_type &color) {
            std::fill_n(data(), numPixels, color);
        }

        // color is RGB
        void fill(uint16_t offset, uint16_t numPixels, uint32_t color) {
            std::fill(data() + offset, data() + numPixels, pixel_type(color));
        }

        void fill(uint16_t offset, uint16_t numPixels, const pixel_type &color) {
            std::fill(data() + offset, data() + numPixels, color);
        }

    private:
        data_type _data;
    };

    // force to clear all pixels without interruptions
    template<typename _Chipset = NEOPIXEL_CHIPSET>
    inline void forceClear(uint8_t pin, uint16_t numPixels, Context *contextPtr = nullptr);

    #if NEOPIXEL_HAVE_STATS
        NeoPixelEx::Stats &getStats();
    #endif

    template<uint8_t _OutputPin, uint16_t _NumPixels, typename _PixelType = GRB, typename _Chipset = TimingsWS2812, typename _DataType = PixelData<_NumPixels, _PixelType>>
    class Strip;
    //<_OutputPin, _NumPixels, _PixelType, _Chipset, _DataType>;

    using StaticStrip = Strip<0, 0, RGB, DefaultTimings>;

    // _DataType must provide enough data for _NumPixels * sizeof(_PixelType)
    template<uint8_t _OutputPin, uint16_t _NumPixels, typename _PixelType, typename _Chipset, typename _DataType>
    class Strip : public PixelData<_NumPixels, _PixelType>
    {
    public:
        static constexpr auto kOutputPin = _OutputPin;
        using chipset_type = _Chipset;
        using data_type = _DataType;
        using pixel_type = _PixelType;

    public:
        Strip() {}

        template<typename ..._Args>
        Strip(_Args &&...args) : _data(std::forward<_Args &&>(args)...) {}

        constexpr uint16_t getNumBytes() const {
            return _NumPixels * sizeof(pixel_type);
        }

        constexpr uint16_t getNumPixels() const {
            return _NumPixels;
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

        // clear is equal to
        // fill(0)
        // show(0)
        __attribute__((always_inline)) inline void clear() {
            fill(0);
            _clear(_data.size());
        }

        // color is RGB
        __attribute__((always_inline)) inline void fill(uint32_t color) {
            _data.fill(pixel_type(color));
        }

        __attribute__((always_inline)) inline void fill(const pixel_type &color) {
            _data.fill(color);
        }

        // color is RGB
        __attribute__((always_inline)) inline void fill(uint16_t numPixels, uint32_t color) {
            std::fill_n(data(), numPixels, pixel_type(color));
        }

        __attribute__((always_inline)) inline void fill(uint16_t numPixels, const pixel_type &color) {
            std::fill_n(data(), numPixels, color);
        }

        // color is RGB
        __attribute__((always_inline)) inline void fill(uint16_t offset, uint16_t numPixels, uint32_t color) {
            std::fill_n(_data.begin() + offset, numPixels, pixel_type(color));
        }

        __attribute__((always_inline)) inline void fill(uint16_t offset, uint16_t numPixels, const pixel_type &color) {
            std::fill_n(_data.begin() + offset, numPixels, color);
        }

        __attribute__((always_inline)) inline void show(uint8_t brightness = 255) {
            internalShow(kOutputPin, reinterpret_cast<uint8_t *>(_data.data()), getNumBytes(), brightness, _context);
        }

        __attribute__((always_inline)) inline pixel_type &operator[](int index) {
            return data()[index];
        }

        __attribute__((always_inline)) inline pixel_type operator[](int index) const {
            return data()[index];
        }

        // this method allows to clear any number of pixels
        // it does not change the actual data
        __attribute__((always_inline)) void _clear(uint16_t numPixels)
        {
            uint8_t buf[1];
            internalShow(kOutputPin, buf, getNumBytes(), 0, _context);
        }

        __attribute__((always_inline)) inline bool canShow() const {
            return (micros() - _context.getLastDisplayTime() > _Chipset::kMinDisplayPeriod);
        }

        __attribute__((always_inline)) inline data_type &data() {
            return _data;
        }

        template<typename _Ta>
        __attribute__((always_inline)) inline _Ta cast() {
            return (_Ta)ptr();
        }

        template<typename _Ta>
        __attribute__((always_inline)) inline _Ta cast() const {
            return (_Ta)ptr();
        }

        __attribute__((always_inline)) inline void *ptr() {
            return (void *)_data.data();
        }

        __attribute__((always_inline)) inline void *ptr() const {
            return (const void *)_data.data();
        }

        __attribute__((always_inline)) inline const data_type &data() const {
            return _data;
        }

    #if NEOPIXEL_HAVE_STATS
        __attribute__((always_inline)) inline Stats &getStats()
        {
            return _context.getStats();
        }
    #endif

        __attribute__((always_inline)) inline Context &getContext()
        {
            return _context;
        }

    protected:

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
            gpio_set_level(static_cast<gpio_num_t>(pin), true);
        }

        __attribute__((always_inline)) inline static void gpio_set_level_low(uint8_t pin, uint32_t pinMask, uint32_t invertedPinMask)
        {
            gpio_set_level(static_cast<gpio_num_t>(pin), false);
        }

    #endif

        __attribute__((always_inline)) inline static uint32_t _getCycleCount(void)
        {
            uint32_t cycleCount;
            __asm__ __volatile__("rsr %0,ccount" : "=a"(cycleCount));
            return cycleCount;
        }

    #if NEOPIXEL_HAVE_BRIGHTNESS

        template<typename _OrderType>
        __attribute__((always_inline)) inline static uint8_t loadPixel(const uint8_t *&ptr, uint16_t brightness, uint8_t ofs)
        {
            if (brightness == 0) {
                ptr++;
                return 0;
            }
            return _OrderType::get(ptr++, ofs);
        }

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
            if (brightness == 0) {
                return 0;
            }
            return (pixel * brightness) >> 8;
        }

        #else

        template<typename _OrderType>
        __attribute__((always_inline)) inline static uint8_t loadPixel(const uint8_t *&ptr, uint16_t brightness, uint8_t ofs)
        {
            if (brightness == 0) {
                ptr++;
                return 0;
            }
            return _OrderType::get(ptr++, ofs);
        }

        __attribute__((always_inline)) inline static uint8_t applyBrightness(uint8_t pixel, uint16_t brightness)
        {
            if (brightness == 0) {
                return 0;
            }
            return (pixel * brightness) >> 8;
        }

        __attribute__((always_inline)) inline static uint8_t applyBrightness(uint8_t pixel, uint16_t brightness)
        {
            return pixel;
        }

        #endif

        // extra function to keep the IRAM usage low
        template<typename _TChipset, typename _TPixelType>
        static bool NEOPIXEL_ESPSHOW_FUNC_ATTR _espShow(uint8_t pin, uint16_t brightness, uint8_t pix, uint8_t mask, const uint8_t *p, const uint8_t *end, uint32_t time0, uint32_t time1, uint32_t &period, uint32_t wait, uint32_t minWaitPeriod, uint32_t pinMask, uint32_t invPinMask)
        {
            #if NEOPIXEL_USE_PRECACHING
                PRECACHE_START(NeoPixel_espShow);
            #endif
            uint32_t startTime = 0;
            uint32_t c, t;
            uint8_t ofs = 1;

            for (;;) {
                t = (pix & mask) ? time1 : time0;

                #if NEOPIXEL_ALLOW_INTERRUPTS
                    // check first if we have a timeout
                    if (((c = _getCycleCount()) - startTime) <= period + (uint8_t)microsecondsToClockCycles(0.6)) {
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
                        if __CONSTEXPR17 (_TPixelType::kReOrder) {
                            if (ofs == 2) {
                                ofs = 0;
                            }
                            else {
                                ofs++;
                            }
                        }
                        else {
                            pix = loadPixel(p, brightness);
                        }
                    }
                    else {
                        mask = 0; // end of frame indicator
                    }
                }

                while (((c = _getCycleCount()) - startTime) < t) {
                    // t0h/t1h wait
                }
                gpio_set_level_low(pin, pinMask, invPinMask);

                #if NEOPIXEL_ALLOW_INTERRUPTS
                    // check if we had a timeout during the TxH phase
                    if ((c - startTime) > t + (uint8_t)microsecondsToClockCycles(0.3)) {
                        #if NEOPIXEL_DEBUG
                            Context::validate(nullptr).getDebugContext().togglePin2();
                        #endif
                        period = 0;
                        break;
                    }
                #endif

                if (mask == 0) { // end of frame
                    break;
                }
                if (mask == 0x80) {
                    if __CONSTEXPR17 (_TPixelType::kReOrder) {
                        pix = loadPixel<typename _TPixelType::OrderType>(p, brightness, ofs);
                    }
                    pix = applyBrightness(pix, brightness);
                }

            }
            while ((_getCycleCount() - startTime) < period) {
                // t0l/t1l wait
            }

            return period != 0;
            #if NEOPIXEL_USE_PRECACHING
                PRECACHE_END(NeoPixel_espShow);
            #endif
        }

        template<typename _TChipset, typename _TPixelType>
        static bool espShow(uint8_t pin, uint16_t brightness, const uint8_t *p, const uint8_t *end, uint32_t pinMask, void *contextPtr)
        {
            auto &context = *reinterpret_cast<NeoPixelEx::Context *>(contextPtr);
            brightness &= 0xff;
            if (brightness) { // change range to 1-256 to avoid division by 255 in applyBrightness()
                brightness++;
            }

            uint8_t pix;
            if __CONSTEXPR17 (_TPixelType::kReOrder) {
                pix = loadPixel<typename _TPixelType::OrderType>(p, brightness, 0);
            }
            else {
                pix = loadPixel(p, brightness);
            }
            pix = applyBrightness(pix, brightness);
            uint8_t mask = 0x80;

            context.waitRefreshTime(_TChipset::getMinDisplayPeriod());

            #if defined(ESP8266) && !NEOPIXEL_ALLOW_INTERRUPTS
                ets_intr_lock();
            #endif

            #if NEOPIXEL_DEBUG
                context.getDebugContext().togglePin();
            #endif

            uint32_t period = _TChipset::getCyclesPeriod();

            // this part must be in IRAM/ICACHE
            auto result = _espShow<_TChipset, _TPixelType>(pin, brightness, pix, mask, p, end, _TChipset::getCyclesT0H(), _TChipset::getCyclesT1H(), period, _TChipset::getCyclesRES(), _TChipset::getMinDisplayPeriod(), pinMask, ~pinMask);

            #if NEOPIXEL_HAVE_STATS
                context.getStats().increment(result);
            #endif

            context.setLastDisplayTime(micros());

            #if defined(ESP8266) && !NEOPIXEL_ALLOW_INTERRUPTS
                ets_intr_unlock();
            #endif

            return result;
        }

        bool internalShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint8_t brightness, Context &context)
        {
            return externalShow<_Chipset, _PixelType>(pin, pixels, numBytes, brightness, context);
        }

    public:
        template<typename _Chipset2, typename _PixelType2>
        static inline bool externalShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint8_t brightness, Context &context)
        {
            bool result = false;
            auto p = pixels;
            auto end = p + numBytes;

            #if NEOPIXEL_INTERRUPT_RETRY_COUNT > 0
                uint8_t retries = NEOPIXEL_INTERRUPT_RETRY_COUNT;
                do {
            #endif
                    result = espShow<_Chipset2, _PixelType2>(pin, brightness, p, end, pin == 16 ? _BV(0) : _BV(pin), &context);
            #if NEOPIXEL_INTERRUPT_RETRY_COUNT > 0
                }
                while(result != true && retries-- > 0);
            #endif

            return result;
        }



    private:
        data_type _data;
        Context _context;
    };

    extern Context _globalContext;

#if NEOPIXEL_HAVE_STATS

    __attribute__((always_inline)) inline Stats &getStats()
    {
        return _globalContext.getStats();
    }

#endif

    inline Context &Context::validate(void *contextPtr)
    {
        if (!contextPtr) {
            return _globalContext;
        }
        return *reinterpret_cast<Context *>(contextPtr);
    }

    template<typename _Chipset>
    inline void forceClear(uint8_t pin, uint16_t numPixels, Context *contextPtr)
    {
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);
        delayMicroseconds(100);

        #if defined(ESP8266) && NEOPIXEL_ALLOW_INTERRUPTS
            ets_intr_lock();
        #endif

        for(uint8_t i = 0; i < 5; i++) {
            if (StaticStrip::externalShow<_Chipset, GRB>(pin, nullptr, numPixels * sizeof(GRB), 0, Context::validate(contextPtr))) {
                break;
            }
            #if defined(ESP8266) && NEOPIXEL_ALLOW_INTERRUPTS
                ets_intr_unlock();
            #endif
            delayMicroseconds(100);
            #if defined(ESP8266) && NEOPIXEL_ALLOW_INTERRUPTS
                ets_intr_lock();
            #endif
        }

        #if defined(ESP8266) && NEOPIXEL_ALLOW_INTERRUPTS
            ets_intr_unlock();
        #endif
    }

}

extern "C" {

    inline void NeoPixel_fillColorGRB(NeoPixelEx::RGB *pixels, uint16_t numBytes, const NeoPixelEx::RGB &color)
    {
        std::fill_n(pixels, numBytes, color);
    }

    inline void NeoPixel_fillColor(uint8_t *pixels, uint16_t numBytes, uint32_t RGBcolor)
    {
        NeoPixel_fillColorGRB(reinterpret_cast<NeoPixelEx::RGB *>(pixels), numBytes / 3, NeoPixelEx::RGB(RGBcolor));
    }

    inline bool NeoPixel_espShow(uint8_t pin, const uint8_t *pixels, uint16_t numBytes, uint16_t brightness, void *contextPtr)
    {
        return NeoPixelEx::StaticStrip::externalShow<NeoPixelEx::DefaultTimings, NeoPixelEx::GRB>(pin, pixels, numBytes, brightness, NeoPixelEx::Context::validate(contextPtr));
    }

}

#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC pop_options
#pragma GCC diagnostic pop
