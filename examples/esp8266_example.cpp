/**
 * Author: sascha_lammers@gmx.de
 */

#include <Arduino.h>
#include "NeoPixelEspEx.h"
#include <core_version.h>

NeoPixelEx::Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, NeoPixelEx::GRB, NEOPIXEL_CHIPSET> pixels;

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
    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, 256);
    delay(2);

    Serial.begin(115200);
    Serial.println(F("Starting..."));
    help();

    pixels.begin();
}

NeoPixelEx::GRB color(0, 0, 0xff);
uint8_t brightness = 10;
uint8_t delayMillis = 1;
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
    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, 256);
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
        auto &stats = NeoPixelEx::getStats();
        offColor.setBrightness(128);
        Serial.printf_P(PSTR("testing color on=%s off=%s pixel=%u/%u run=%u dropped=%u frames=%u fps=%.2f delay=10\n"), onColor.toString().c_str(), offColor.toString().c_str(), currentPixelNum, pixels.getNumPixels(), numRun, stats.getAbortedFrames(), stats.getFrames(), stats.getFps(), delayMillis);
        pixels.fill(offColor);
        pixels[currentPixelNum] = onColor;
        pixels.show(20);
        testAll++;
    }
    else if (testAll == -1) {
        // display pixels
        pixels.fill(color);
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
                case 't':
                    if (delayMillisOld == -1) {
                        delayMillisOld = delayMillis;
                    }
                    delayMillis = 10;
                    Serial.println(F("testing all pixels\n"));
                    clear_keys();
                    testAll = 0;
                    counter = -1;
                    NeoPixelEx::getStats().clear();
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
                case 'S':
                    Serial.println(F("clear stat..."));
                    NeoPixelEx::getStats().clear();
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
                    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, 256);
                    Serial.println(F("waiting 3 seconds..."));
                    delay(3000);
                    clear_keys();
                    NeoPixelEx::getStats().clear();
                    break;
                case 'H':
                    NeoPixelEx::forceClear<decltype(pixels)::chipset_type>(pixels.kOutputPin, 256);
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
            const auto &stats = NeoPixelEx::getStats();
            Serial.printf_P(PSTR("selected=%s step=%u brightness=%u %s dropped=%u frames=%u fps=%.2f delay=%u\n"), get_type_str(), get_type_incr(), brightness, color.toString().c_str(), stats.getAbortedFrames(), stats.getFrames(), stats.getFps(), delayMillis);
        }
    }

    delay(delayMillis);
    counter++;
}
