/**************************************************************

This file is a part of
https://github.com/JoTid/PVZero

Copyright [2020] Alexander Tiderko

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-3.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

**************************************************************/
#ifdef ESP8266
    #include "ewcRTC.h"
#elif defined(ESP32)
#endif
#include <extensions/ewcTime.h>
#include <ewcTickerLED.h>
#include <ewcInterface.h>
#include "shelly_em3_connector.h"
#include "ewcLogger.h"
#include "config.h"
#include "pvzero_interface.h"

using namespace EWC;
using namespace PVZERO;

void ShellyEm3Connector::startTaskImpl(void *_this)
{
    static_cast<ShellyEm3Connector *>(_this)->httpTask();
}

ShellyEm3Connector::ShellyEm3Connector(int potPin)
{
    mailStateChanged = false;
    _potPin = potPin;
    _callbackState = NULL;
    _currentState = ShellyEm3Connector::State::UNKNOWN;
    _currentExcess = -1;
    _currentCurrent = 0;
    _isRequesting = false;
    btIsValidP = false;
#ifdef ESP8266
        // initialize utc addresses in sutup method
        _utcAddress = 0;
#endif
}

ShellyEm3Connector::~ShellyEm3Connector()
{
    // delete _sleeper;
}

void ShellyEm3Connector::setup(bool resetConfig)
{
#ifdef ESP8266
    _utcAddress = EWC::I::get().rtc().get();
    _reachedUpperLimit = EWC::I::get().rtc().read(_utcAddress);
#endif
    if (_reachedUpperLimit != 1) {
      _reachedUpperLimit = 0;
    }
    _httpClient.useHTTP10(true);
    _httpClient.setReuse(true);
    // pinMode(_potPin, OUTPUT);
    // digitalWrite(_potPin, LOW);
    // _sleeper->setup(resetConfig);
#ifdef ESP8266
        EWC::I::get()
            .logger()
        << F("ShellyEm3Connector: reads from UTC (addr: ") << _utcAddress << F(") stored last state: ") << _reachedUpperLimit << endl;
#endif
}

// long ShellyEm3Connector::_analog2percent(long analogValue)
// {
//     float result = float(750 - analogValue) / 750.0 * 100.0;
//     if (result > 100.0) {
//         result = 100.0;
//     } else if (result < 0.0) {
//         result = 0.0;
//     }
//     return long(result);
// }

void ShellyEm3Connector::loop()
{
    // if (!sleeper().finished()) {
    //     return;  // we are sleeping now
    // }
    _infoState = String("");
    if (PZI::get().time().isDisturb()) {
        _currentState = DO_NOT_DISTURB;
        _infoState += "bitte nicht stoeren phase";
        return;
    }
    if (_sleeper.finished() && !_isRequesting && PZI::get().config().shellyEm3Uri.length() > 0)
    {
        _isRequesting = true;
        I::get().led().start(1000, 250);
        xTaskCreate(
            this->startTaskImpl,           // Function that should be called
            "GET current by http request", // Name of the task (for debugging)
            2048,                         // Stack size (bytes)
            this,                  // Parameter to pass
            5,                             // Task priority
            NULL                           // Task handle
        );
    }
}

String ShellyEm3Connector::state2string(ShellyEm3Connector::State state)
{
    switch(state) {
        case ON_CHECK:
            return "ON_CHECK";
        case DO_NOT_DISTURB:
            return "DO_NOT_DISTURB";
        case UNKNOWN:
            return "UNKNOWN";
        case SLEEP:
            return "SLEEP";
    }
    return "Unknown";
}

void ShellyEm3Connector::httpTask()
{
    _infoState = String("Lese status vom Shelly Em3 Modul");
    // Send request
    String request_uri = String(PZI::get().config().shellyEm3Uri) + "/status";
    EWC::I::get().logger() << F("ShellyEm3Connector: request_uri: ") << request_uri << endl;
    _httpClient.begin(_wifiClient, request_uri.c_str());
    int httpCode = _httpClient.GET();
    if (httpCode != 200)
    {
        EWC::I::get().logger() << F("ShellyEm3Connector: fehler beim holen der aktuellen Verbrauchswerte vom Shelly") << endl;
        _infoState = "Fehler beim holen der aktuellen Verbrauchswerte vom Shelly " + String(PZI::get().config().shellyEm3Uri) + "/status";
        _currentCurrent = -1;
        btIsValidP = false;
        I::get().led().start(3000, 3000);
        if (_callbackState != NULL)
        {
            _callbackState(false, _currentCurrent);
        }
    }
    else
    {
        btIsValidP = true;
        JsonDocument doc;
        // deserializeJson(doc, http.getStream());
        String jsonStr = _httpClient.getString();
        deserializeJson(doc, jsonStr);
        _currentExcess = (int)doc["total_power"];
        EWC::I::get().logger() << F("ShellyEm3Connector: aktueller Verbrauch: ") << _currentExcess << " W" << endl;
        // _currentCurrent += _currentExcess / PZI::get().config().voltage;
        // if (_currentCurrent < 0) {
        //     _currentCurrent = 0;
        //     _infoState = "Akku wird nicht entladen!";
        // } else if (_currentCurrent > PZI::get().config().maxAmperage) {
        //     _currentCurrent = PZI::get().config().maxAmperage;
        //     _infoState = "Maximale erlaubte Einspasung!";
        // } else {
        //     _infoState = "";
        // }
        if (_callbackState != NULL)
        {
            _callbackState(true, _currentExcess);
        }
        I::get().led().stop();
    }
    EWC::I::get().logger() << F("ShellyEm3Connector: request finished... sleep ") << endl;
    _sleepUntil = PZI::get().time().str(PZI::get().config().checkInterval);
    // sleeper().sleep(PZI::get().config().checkInterval * 1000);
    // EWC::I::get().logger() << F("sleep for ") << PZI::get().config().checkInterval << "sec" << endl;
    _sleeper.sleep(PZI::get().config().checkInterval * 1000);
    _currentState = SLEEP;

    _isRequesting = false;
    vTaskDelete(NULL);
}
