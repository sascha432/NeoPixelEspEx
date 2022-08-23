/**
 * Author: sascha_lammers@gmx.de
 */

#include <Arduino.h>
// #include <core_version.h>
#include <Schedule.h>
#define FASTLED_INTERNAL
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include "NeoPixelEspEx.h"
#include "wiring_private.h"

#define NEOPIXEL_NUM_PIXELS_CLEAR NEOPIXEL_NUM_PIXELS + 10

using namespace NeoPixelEx;

Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, NeoPixelEx::GRB, NEOPIXEL_CHIPSET> pixels;

// example of using the DataWrapper to use any raw pointer with the Strip class
//
// uint8_t pixelData[NEOPIXEL_NUM_PIXELS * sizeof(GRB)];
// Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, GRB, NEOPIXEL_CHIPSET, DataWrapper<NEOPIXEL_NUM_PIXELS, GRB>> pixels(&pixelData);

extern bool record_serial_output;
extern String serial_output;

#if 0
#include <user_interface.h>
uart_t *_uart;
void preinit()
{
    _uart = uart_init(UART0, 115200, (int) SERIAL_8N1, (int) SERIAL_FULL, 1, 64, false);
    ::printf("preinit\n");
}
#endif

void help()
{
    Serial.printf_P(PSTR("---\npin=%u pixels=%u cpu=%u\n"), pixels.kOutputPin, pixels.getNumPixels(), (int)(F_CPU / 1000000UL));
    Serial.print(F(
        "---\n" \
        "H  turn all pixels off and halt\n" \
        "R  reset pixels and values\n" \
        "C  clear pixels\n" \
        "t  test all pixels\n" \
        "+  increase selected value by step size\n" \
        "-  decrease selected value by step size\n" \
        "*  increase step size\n" \
        "/  decrease step size\n" \
        "r  select red value\n" \
        "b  select blue value\n" \
        "g  select green value\n" \
        "B  select brightness\n" \
        "d  select delay\n" \
        "S  clear stats\n" \
        "p  next pattern (param1-param3 can be used to modify it)\n" \
        "P  prev. pattern (param1-param3 can be used to modify it)\n" \
        "1  select param 1\n" \
        "2  select param 2\n" \
        "3  select param 3\n" \
        "?  display help\n" \
        "\n" \
    ));
}

void setup()
{
#if 0
    ::printf("setup\n");
    uart_uninit(_uart);
#endif
    forceClear<pixels.kOutputPin, decltype(pixels)::chipset_type>(NEOPIXEL_NUM_PIXELS_CLEAR);

    Serial.begin(115200);
    Serial.println(F("starting..."));

    // pinMode(2,OUTPUT);
    // gdbstub_init();


    pixels.begin();

    schedule_recurrent_function_us([]() {
        help();
        return false;
    }, 5000000);

}

NeoPixelEx::GRB color(0, 0, 0xff);
uint8_t brightness = 10;
uint8_t delayMillis = 4;
uint8_t pattern = 1;
uint8_t stepSize = 1;
uint8_t param1 = 10;
uint8_t param2 = 10;
uint8_t param3 = 1;
uint8_t *valuePtr = &brightness;

static void default_params() {
    switch(pattern) {
        case 1:
            param1 = 30;
            param2 = 10;
            break;
        case 2:
            param1 = 10;
            param2 = 4;
            break;
    }
}

static PGM_P get_type_str()
{
    if (valuePtr == &brightness) {
        return PSTR("brightness");
    }
    if (valuePtr == &color.r) {
        return PSTR("red");
    }
    if (valuePtr == &color.g) {
        return PSTR("green");
    }
    if (valuePtr == &color.b) {
        return PSTR("red");
    }
    if (valuePtr == &delayMillis) {
        return PSTR("delay");
    }
    if (valuePtr == &stepSize) {
        return PSTR("step size");
    }
    if (valuePtr == &pattern) {
        return PSTR("pattern");
    }
    if (valuePtr == &param1) {
        return PSTR("param1");
    }
    if (valuePtr == &param2) {
        return PSTR("param2");
    }
    if (valuePtr == &param3) {
        return PSTR("param3");
    }
    return PSTR("?");
}

static void clear_keys() {
    while(Serial.available()) {
        Serial.read();
    }
}

static int testAll = -1;
static int delayMillisOld = -1;

void end_test(const __FlashStringHelper *msg)
{
    forceClear<pixels.kOutputPin, decltype(pixels)::chipset_type>(NEOPIXEL_NUM_PIXELS_CLEAR);
    testAll = -1;
    if (delayMillisOld != -1) {
        delayMillis = delayMillisOld;
        delayMillisOld = -1;
    }
    Serial.println(msg);
}

void loop()
{
    static int counter = 0;

    EVERY_N_MILLIS(250) {
        static bool toggle;
        toggle = !toggle;
        digitalWrite(2, toggle);
    }

    if (testAll != -1 && counter > 100) {
        counter = 0;
        using ColorType = NeoPixelEx::GRB;
        // test pixels
        static ColorType onColor;

        auto numRun = testAll / pixels.getNumPixels();
        auto currentPixelNum = testAll % pixels.getNumPixels();
        if (currentPixelNum == 0) {
            if (numRun >= 4) {
                end_test(F("finished testing"));
                return;
            }
            // change color per run
            if (testAll % pixels.getNumPixels() == 0) {
                switch(numRun) {
                    case 0:
                        onColor = ColorType(0xff, 0, 0);
                        break;
                    case 1:
                        onColor = ColorType(0, 0xff, 0);
                        break;
                    case 2:
                        onColor = ColorType(0, 0, 0xff);
                        break;
                    case 3:
                        onColor = ColorType(0xff, 0xff, 0xff);
                        break;
                }
            }
        }
        ColorType offColor = onColor.inverted();
        offColor.setBrightness(128);
        #if NEOPIXEL_HAVE_STATS
            auto &stats = pixels.getStats();
            Serial.printf_P(PSTR("testing color on=%s off=%s pixel=%u/%u run=%u dropped=%u (%.1f%%) frames=%u fps=%u delay=10\n"),
                onColor.toString().c_str(), offColor.toString().c_str(), currentPixelNum, pixels.getNumPixels(), numRun,
                stats.getAbortedFrames(),  stats.getAbortedFrames() * 100.0 / stats.getFrames(), stats.getFrames(),
                stats.getFps(), delayMillis
            );
        #else

        #endif
        pixels.fill(offColor);
        pixels[currentPixelNum] = onColor;
        pixels.show(10);
        testAll++;
    }
    else if (testAll == -1) {
        EVERY_N_MILLISECONDS(10) {
            switch(pattern % 4) {
                case 0:
                    // display pixels
                    pixels.fill(color);
                    break;
                case 1: {
                        fill_rainbow(pixels.cast<::CRGB *>(), pixels.getNumPixels(), beat8(param1, 255), param2);
                    }
                    break;
                case 2: {
                        uint32_t from = 0x00ff00;
                        uint32_t to = 0xff00ff;
                        for(auto i = 0; i < pixels.getNumPixels() && i < pixels.getNumPixels(); i += param1) {
                            fill_gradient_RGB(&pixels.cast<::CRGB *>()[i], pixels.getNumPixels() - i, from, to);
                            std::swap(from, to);
                            // auto tmp = NeoPixelEx::GRB(from);
                            // tmp <<= 4;
                            // from = tmp.toRGB();
                            auto tmp = NeoPixelEx::GRB(to);
                            tmp <<= param2;
                            to = tmp.toRGB();
                        }
                    } break;
                default:
                    pixels.fill(0);
                    break;
            }
        }
        pixels.show(brightness);
    }

    // serial input handler
    if (Serial.available()) {
        if (testAll != -1) {
            end_test(F("testing pixels aborted..."));
        }
        while(Serial.available()) {
            char ch  = Serial.read();
            switch(ch) {
                case 'M': {
                        pixels.fill(0);
                        auto &data = pixels.data();
                        auto endPtr = data.end();
                        for(auto ptr = data.begin(); ptr < endPtr; ptr += 10) {
                            *ptr = 0x000011;
                        }
                        for(auto ptr = data.begin() + 4; ptr < endPtr; ptr += 10) {
                            *ptr = 0x000044;
                        }
                        for(auto ptr = data.begin() + 9; ptr < endPtr; ptr += 10) {
                            *ptr = 0x0f0088;
                        }
                        for(auto ptr = data.begin() + 49; ptr < endPtr; ptr += 50) {
                            *ptr = 0x004400;
                        }
                        for(auto ptr = data.begin() + 99; ptr < endPtr; ptr += 100) {
                            *ptr = 0x008800;
                        }
                        pixels[pixels.size() - 1] = 0xff0000;
                        pixels.show(255);
                        delay(30000);
                        pixels.clear();
                    }
                    break;
                case 't':
                    if (delayMillisOld == -1) {
                        delayMillisOld = delayMillis;
                    }
                    delayMillis = 10;
                    Serial.println(F("testing all pixels\n"));
                    clear_keys();
                    testAll = 0;
                    counter = -1;
                    #if NEOPIXEL_HAVE_STATS
                        pixels.getStats().clear();
                    #endif
                    pixels.clear();
                    break;
                case 'p':
                    pattern++;
                    default_params();
                    break;
                case 'P':
                    pattern--;
                    default_params();
                    break;
                case '*':
                    stepSize = std::min(32, stepSize + 1);
                    break;
                case '/':
                    stepSize = std::max<int>(1, stepSize - 1);
                    break;
                case '+':
                    if (valuePtr == &pattern || valuePtr == &param1 || valuePtr == &param2 || valuePtr == &param3) {
                        *valuePtr = std::min<int>(255, *valuePtr + 1);
                    }
                    else {
                        *valuePtr = std::min<int>(255, *valuePtr + stepSize);
                    }
                    break;
                case '-':
                    if (valuePtr == &pattern || valuePtr == &param1 || valuePtr == &param2 || valuePtr == &param3) {
                        *valuePtr = std::min<int>(255, *valuePtr - 1);
                    }
                    else {
                        *valuePtr = std::max<int>(0, *valuePtr - stepSize);
                    }
                    break;
                case 'r':
                    valuePtr = &color.r;
                    break;
                case 'b':
                    valuePtr = &color.b;
                    break;
                case 'g':
                    valuePtr = &color.g;
                    break;
                case 'B':
                    valuePtr = &brightness;
                    break;
                case 'd':
                    valuePtr = &delayMillis;
                    break;
                case '1':
                    valuePtr = &param1;
                    break;
                case '2':
                    valuePtr = &param2;
                    break;
                case '3':
                    valuePtr = &param3;
                    break;
                case 'S':
                    Serial.println(F("clearing stats..."));
                    #if NEOPIXEL_HAVE_STATS
                        pixels.getStats().clear();
                    #endif
                    break;
                case 'R':
                    Serial.println(F("reset"));
                    color = NeoPixelEx::GRB(0, 0, 0xff);
                    brightness = 16;
                    delayMillis = 1;
                    valuePtr = &brightness;
                    // falltrough
                case 'C':
                    Serial.println(F("clearing all pixels..."));
                    forceClear<pixels.kOutputPin, decltype(pixels)::chipset_type>(NEOPIXEL_NUM_PIXELS_CLEAR);
                    Serial.println(F("waiting 3 seconds..."));
                    delay(3000);
                    clear_keys();
                    #if NEOPIXEL_HAVE_STATS
                        pixels.getStats().clear();
                    #endif
                    break;
                case 'H':
                    forceClear<pixels.kOutputPin, decltype(pixels)::chipset_type>(NEOPIXEL_NUM_PIXELS_CLEAR);
                    Serial.println(F("halted..."));
                    for(;;) {
                        delay(1000);
                    }
                    break;
                case '?':
                    help();
                    break;
                default:
                    break;
            }
            #if NEOPIXEL_HAVE_STATS
                const auto &stats = pixels.getStats();
                Serial.printf_P(PSTR("selected=%s value=%u brightness=%u %s dropped=%u (%.1f%%) frames=%u fps=%u delay=%u\n"),
                    get_type_str(), *valuePtr, brightness, color.toString().c_str(),
                    stats.getAbortedFrames(),  stats.getAbortedFrames() * 100.0 / stats.getFrames(), stats.getFrames(),
                    stats.getFps(), delayMillis
                );
            #else
                Serial.printf_P(PSTR("selected=%s value=%u brightness=%u %s delay=%u\n"), get_type_str(), *valuePtr, brightness, color.toString().c_str(), delayMillis);
            #endif
        }
    }

    delay(delayMillis);
    counter++;
}
