/**
 * Author: sascha_lammers@gmx.de
 */

#define FASTLED_INTERNAL
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include "NeoPixelEspEx.h"
#include <Schedule.h>
#include <ESP8266WiFi.h>

// to test with an active WiFi connection, add SSID and password or put credentials into ../../wifi.h
// #define WIFI_SSID "SSID"
// #define WIFI_PASS "123"
#ifndef WIFI_SSID
    #include "../../wifi.h"
#endif

#undef NEOPIXEL_NUM_PIXELS
#define NEOPIXEL_NUM_PIXELS 64


CRGB pixelData[NEOPIXEL_NUM_PIXELS];
// NeoPixelEx::CRGB requires more CPU cycles than NeoPixelEx::GRB
NeoPixelEx::Strip<NEOPIXEL_OUTPUT_PIN, NEOPIXEL_NUM_PIXELS, NeoPixelEx::CRGB, NeoPixelEx::TimingsWS2812, NeoPixelEx::DataWrapper<NEOPIXEL_NUM_PIXELS, NeoPixelEx::CRGB>> pixels(&pixelData);
bool useFastLEDShow = true;
int stepSize = 4;
int bpm = 10;
int mainLoopDelay = 5;
int powerLimit = 2500; // mW
bool dithering = false;

NeoPixelEx::Context context;

static void show()
{
    if (useFastLEDShow) {
        FastLED.show();
    }
    else {
        pixels.show(FastLED.getBrightness());
    }
}

// static void _delay(int millis)
// {
//     if (useFastLEDShow) {
//         FastLED.delay(millis);
//     }
//     else {
//         delay(millis);
//     }
// }

static void help()
{
    Serial.printf_P(PSTR(
        "---\n"
        "+      increase brightness\n"
        "-      decrease brightness\n"
        "*      increase step size\n"
        "/      decrease step size\n"
        "0-9    set bpm from 10-100\n"
        "p      change pattern\n"
        "d      toggle display method\n"
        "D      enable/disable dithering\n"
        "w      connect to WiFi\n"
        "W      disconnect WiFi\n"
        "\n"
        "power limit set to %umW\n"
        "\n"), powerLimit
    );
}

extern uint32_t test_value;
uint32_t test_value;
int pattern = 0;

void update_pattern() {
    switch(pattern % 2) {
        case 0: {
                pixels.fill(0, 10, 0x202020);
                pixels.fill(50, 10, 0x000001);
                pixels[0] = 0xff7f00;
                pixels[1] = 0xff0000;
                pixels[10] = 0xff0000;
                pixels[15] = 0x0000ff;
                pixels[25] = 0x00ff00;

                NeoPixelEx::GRB color(0x0000ff);
                for(uint16_t i = 0; i < pixels.getNumPixels(); i += 10) {
                    pixels.fill(i, 10, color);
                    color <<= 8;
                    if (i % 30 == 20) {
                        color.setBrightness(200);
                    }
                }
            }
            break;
        case 1: {
                fill_rainbow(pixelData, pixels.getNumPixels(), beat8(10, 255), 10);
            } break;
    }
}

void setup()
{
    Serial.begin(115200);
    pixels.begin();
    NeoPixelEx::forceClear(NEOPIXEL_OUTPUT_PIN, 256);

    FastLED.addLeds<WS2812, NEOPIXEL_OUTPUT_PIN, GRB>(pixelData, NEOPIXEL_NUM_PIXELS);
// #if NEOPIXEL_NUM_PIXELS < 100
    FastLED.setMaxPowerInVoltsAndMilliamps(5, powerLimit / 5);
// #endif

    FastLED.setBrightness(10);
    FastLED.setDither(dithering);

    FastLED.clear();
    show();

    schedule_recurrent_function_us([]() {
        help();
        return false;
    }, 5 * 1000 * 1000);

    schedule_recurrent_function_us([]() {
        Serial.printf("FPS: %u\n", FastLED.getFPS());
        return true;
    }, 5 * 1000 * 1000);

    update_pattern();

    WiFi.persistent(false);
    WiFi.onEvent([](WiFiEvent_t event) {
        Serial.printf_P(PSTR("WiFi event: %u\n"), event);
        if (event == WIFI_EVENT_STAMODE_GOT_IP) {
            Serial.printf_P(PSTR("WiFi: connected to %s / %s\n"), WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
        }
        else if (event == WIFI_EVENT_STAMODE_DISCONNECTED) {
            Serial.printf_P(PSTR("WiFi: disconnect\n"));
        }
    }, WIFI_EVENT_ANY);
}

NeoPixelEx::CRGB color(0xff0000);

void loop()
{
    EVERY_N_MILLISECONDS(10) {
        update_pattern();
    }
    show();

    if (Serial.available()) {
        int ch;
        switch(ch = Serial.read()) {
            case '*':
                stepSize = std::min(32, stepSize + 1);
                Serial.printf_P(PSTR("step size %u\n"), stepSize);
                break;
            case '/':
                stepSize = std::max(1, stepSize - 1);
                Serial.printf_P(PSTR("step size: %u\n"), stepSize);
                break;
            case '+':
                FastLED.setBrightness(std::min<int>(255, FastLED.getBrightness() + stepSize));
                Serial.printf_P(PSTR("brightness: %u\n"), FastLED.getBrightness());
                break;
            case '-':
                FastLED.setBrightness(std::max<int>(0, FastLED.getBrightness() - stepSize));
                Serial.printf_P(PSTR("brightness: %u\n"), FastLED.getBrightness());
                break;
            case 'p':
                pattern++;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                ch -= '0';
                bpm = ch * 10 + 10;
                Serial.printf_P(PSTR("bpm: %u\n"), bpm);
                break;
            case 'D':
                FastLED.setDither(dithering = !dithering);
                Serial.printf_P(PSTR("dithering: %u\n"), dithering);
                break;
            case 'd':
                useFastLEDShow = !useFastLEDShow;
                Serial.printf_P(PSTR("display method: %s\n"), useFastLEDShow ? PSTR("FastLED") : PSTR("NeoPixel"));
                break;
            case 'w':
                Serial.printf_P(PSTR("WiFi status: %u\n(re)connecting WiFi...\n"), WiFi.status());
                WiFi.disconnect();
                WiFi.setAutoConnect(true);
                WiFi.setAutoReconnect(true);
                WiFi.begin(WIFI_SSID, WIFI_PASS);
                break;
            case 'W':
                WiFi.setAutoConnect(false);
                WiFi.setAutoReconnect(false);
                WiFi.disconnect();
                Serial.println(F("WiFi disconnected..."));
                break;
            default:
                help();
                break;
        }
    }
}
