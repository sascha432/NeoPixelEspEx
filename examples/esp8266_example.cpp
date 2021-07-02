/**
 * Author: sascha_lammers@gmx.de
 */

#include <Arduino.h>
// #include <core_version.h>
#include <Schedule.h>>
#include "NeoPixelEspEx.h"

// #define NEOPIXEL_NUM_PIXELS_CLEAR NEOPIXEL_NUM_PIXELS * 2
#define NEOPIXEL_NUM_PIXELS_CLEAR NEOPIXEL_NUM_PIXELS + 10

using namespace NeoPixelEx;

Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, GRB, NEOPIXEL_CHIPSET> pixels;

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
    Serial.print(F(
        "---\n" \
        "H  turn all pixels off and halt\n" \
        "R  reset pixels and values\n" \
        "C  clear pixels\n" \
        "t  test all pixels\n" \
        "+  increase selected value\n" \
        "-  decrease selected value\n" \
        "*  increase selected value by 16\n" \
        "/  decrease selected value by 16\n" \
        "r  select red value\n" \
        "b  select blue value\n" \
        "g  select green value\n" \
        "B  select brightness\n" \
        "d  select delay\n" \
        "S  clear stats\n" \
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
    forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, NEOPIXEL_NUM_PIXELS_CLEAR);

    Serial.begin(115200);
    Serial.println(F("starting..."));

    pixels.begin();

    schedule_recurrent_function_us([]() {
        help();
        return false;
    }, 5000000);

}

GRB color(0, 0, 0xff);
uint8_t brightness = 10;
uint8_t delayMillis = 4;
uint8_t *valuePtr = &brightness;

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
    return PSTR("?");
}

static int get_type_incr()
{
    // if (valuePtr == &brightness) {
    //     return 1;
    // }
    if (
        (valuePtr == &color.r) ||
        (valuePtr == &color.g) ||
        (valuePtr == &color.r)
    ) {
        return 8;
    }
    // if (valuePtr == &delayMillis) {
    //     return 1;
    // }
    return 1;
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
    forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, NEOPIXEL_NUM_PIXELS_CLEAR);
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

    if (testAll != -1 && counter > 100) {
        counter = 0;
        using ColorType = GRB;
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
            Serial.printf_P(PSTR("testing color on=%s off=%s pixel=%u/%u run=%u dropped=%u frames=%u fps=%u delay=10\n"), onColor.toString().c_str(), offColor.toString().c_str(), currentPixelNum, pixels.getNumPixels(), numRun, stats.getAbortedFrames(), stats.getFrames(), stats.getFps(), delayMillis);
        #else

        #endif
        pixels.fill(offColor);
        pixels[currentPixelNum] = onColor;
        pixels.show(10);
        testAll++;
    }
    else if (testAll == -1) {
        // display pixels
        pixels.fill(color);

        // GRB pattern[5] = { color, color.scale(200), color.scale(100), color.scale(75), color.scale(25) };
        // auto begin = static_cast<uint8_t *>(pixels);
        // auto end = begin + pixels.getNumBytes();
        // while(begin + sizeof(pattern) < end) {
        //     memcpy(begin, pattern, sizeof(pattern));
        //     begin += sizeof(pattern);
        // }
        // std::fill(begin, end, 0);

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
                case '*':
                    *valuePtr = std::min<int>(255, *valuePtr + 16);
                    break;
                case '/':
                    *valuePtr = std::max<int>(0, *valuePtr - 16);
                    break;
                case '+':
                    *valuePtr = std::min<int>(255, *valuePtr + get_type_incr());
                    break;
                case '-':
                    *valuePtr = std::max<int>(0, *valuePtr - get_type_incr());
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
                    if (pixels[0] != 0) {
                        color = 0;
                    }
                    else {
                        color = 0xffffff;
                    }
                    break;
                case '2':
                    if (pixels[0] != 0) {
                        brightness = 0;
                    }
                    else {
                        brightness = 255;
                    }
                    break;
                case '3':
                    if (pixels[0] != 0) {
                        brightness = 0;
                        color = 0;
                    }
                    else {
                        brightness = 255;
                        color = 0xffffff;
                    }
                    break;
                case 'S':
                    Serial.println(F("clearing stats..."));
                    #if NEOPIXEL_HAVE_STATS
                        pixels.getStats().clear();
                    #endif
                    break;
                case 'R':
                    Serial.println(F("reset"));
                    color = GRB(0, 0, 0xff);
                    brightness = 16;
                    delayMillis = 1;
                    valuePtr = &brightness;
                    // falltrough
                case 'C':
                    Serial.println(F("clearing all pixels..."));
                    forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, NEOPIXEL_NUM_PIXELS_CLEAR);
                    Serial.println(F("waiting 3 seconds..."));
                    delay(3000);
                    clear_keys();
                    #if NEOPIXEL_HAVE_STATS
                        pixels.getStats().clear();
                    #endif
                    break;
                case 'H':
                    forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, NEOPIXEL_NUM_PIXELS_CLEAR);
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
                Serial.printf_P(PSTR("selected=%s step=%u brightness=%u %s dropped=%u frames=%u fps=%u delay=%u\n"), get_type_str(), get_type_incr(), brightness, color.toString().c_str(), stats.getAbortedFrames(), stats.getFrames(), stats.getFps(), delayMillis);
            #else
                Serial.printf_P(PSTR("selected=%s step=%u brightness=%u %s delay=%u\n"), get_type_str(), get_type_incr(), brightness, color.toString().c_str(), delayMillis);
            #endif
        }
    }

    delay(delayMillis);
    counter++;
}
