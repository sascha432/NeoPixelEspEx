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

NeoPixelEx::Context NeoPixelEx::_globalContext;

#if ESP32
    NeoPixelEx::RTM_Adapter_Data_t NeoPixelEx::rmtChannelsInUse[NeoPixelEx::kMaxRmtChannels] = {};
#endif
