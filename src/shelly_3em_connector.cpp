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
#include <extensions/ewcMail.h>
#include <ewcTickerLED.h>
#include <ewcInterface.h>
#include "shelly_3em_connector.h"
#include "ewcLogger.h"
#include "config.h"
#include "pvzero_interface.h"

using namespace EWC;
using namespace PVZ;

void Shelly3emConnector::startTaskImpl(void *_this)
{
  static_cast<Shelly3emConnector *>(_this)->httpTask();
}

Shelly3emConnector::Shelly3emConnector(int potPin)
{
  mailStateChanged = false;
  _potPin = potPin;
  _callbackState = NULL;
  _currentState = Shelly3emConnector::State::UNKNOWN;
  _consumptionPower = -1;
  _feedInPower = 0;
  _isRequesting = false;
  _countRequestsFailed = 0;
  btIsValidP = false;
#ifdef ESP8266
  // initialize utc addresses in sutup method
  _utcAddress = 0;
#endif
}

Shelly3emConnector::~Shelly3emConnector()
{
  // delete _sleeper;
}

void Shelly3emConnector::setup(bool resetConfig)
{
#ifdef ESP8266
  _utcAddress = EWC::I::get().rtc().get();
  _reachedUpperLimit = EWC::I::get().rtc().read(_utcAddress);
#endif
  if (_reachedUpperLimit != 1)
  {
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
      << F("Shelly3emConnector: reads from UTC (addr: ") << _utcAddress << F(") stored last state: ") << _reachedUpperLimit << endl;
#endif
}

// long Shelly3emConnector::_analog2percent(long analogValue)
// {
//     float result = float(750 - analogValue) / 750.0 * 100.0;
//     if (result > 100.0) {
//         result = 100.0;
//     } else if (result < 0.0) {
//         result = 0.0;
//     }
//     return long(result);
// }

void Shelly3emConnector::loop()
{
  // if (!sleeper().finished()) {
  //     return;  // we are sleeping now
  // }
  _infoState = String("");
  if (I::get().time().isDisturb())
  {
    _currentState = DO_NOT_DISTURB;
    _infoState += "bitte nicht stoeren phase";
    return;
  }
  if (_sleeper.finished() && !_isRequesting && PZI::get().config().shelly3emAddr.length() > 0)
  {
    _isRequesting = true;
    I::get().led().start(1000, 250);
    xTaskCreate(
        this->startTaskImpl,           // Function that should be called
        "GET current by http request", // Name of the task (for debugging)
        2048,                          // Stack size (bytes)
        this,                          // Parameter to pass
        5,                             // Task priority
        NULL                           // Task handle
    );
  }
}

String Shelly3emConnector::state2string(Shelly3emConnector::State state)
{
  switch (state)
  {
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

void Shelly3emConnector::httpTask()
{
  _infoState = String("Lese status vom Shelly Em3 Modul");
  // Send request
  String uri = PZI::get().config().shelly3emAddr;
  if (!uri.startsWith("http"))
  {
    uri = String("http://") + uri;
  }
  String request_uri = uri + "/status";
  EWC::I::get().logger() << F("Shelly3emConnector: request_uri: ") << request_uri << endl;
  _httpClient.begin(_wifiClient, request_uri.c_str());
  int httpCode = _httpClient.GET();
  if (httpCode != 200)
  {
    EWC::I::get().logger() << F("Shelly3emConnector: fehler beim holen der aktuellen Verbrauchswerte vom Shelly") << endl;
    _infoState = "Fehler beim holen der aktuellen Verbrauchswerte vom Shelly " + String(PZI::get().config().shelly3emAddr) + "/status";
    _feedInPower = -1;
    _countRequestsFailed += 1;
    btIsValidP = false;
    I::get().led().start(3000, 3000);
    if (_callbackState != NULL)
    {
      _callbackState(false, _feedInPower);
    }
    if (_countRequestsFailed >= 3)
    {
      PZI::get().mail().sendWarning("Shelly 3em nicht erreichbar", _infoState.c_str());
    }
  }
  else
  {
    btIsValidP = true;
    JsonDocument doc;
    // deserializeJson(doc, http.getStream());
    String jsonStr = _httpClient.getString();
    deserializeJson(doc, jsonStr);
    _consumptionPower = (int)doc["total_power"];
    EWC::I::get().logger() << F("Shelly3emConnector: aktueller Verbrauch: ") << _consumptionPower << " W" << endl;
    // _feedInPower += _consumptionPower / PZI::get().config().maxVoltage;
    // if (_feedInPower < 0) {
    //     _feedInPower = 0;
    //     _infoState = "Akku wird nicht entladen!";
    // } else if (_feedInPower > PZI::get().config().maxAmperage) {
    //     _feedInPower = PZI::get().config().maxAmperage;
    //     _infoState = "Maximale erlaubte Einspasung!";
    // } else {
    //     _infoState = "";
    // }
    if (_callbackState != NULL)
    {
      _callbackState(true, _consumptionPower);
    }
    // String topic = "/" + I::get().config().paramDeviceName + "/consumption/power";
    // String payload = String(_consumptionPower);
    // PZI::get().mqtt().publishState("pvz", "consumption-power", payload);
    if (_countRequestsFailed >= 3)
    {
      String body = String("Nach ") + _countRequestsFailed + " Versuchen wurde der Wert " + _consumptionPower + " W geholt.";
      PZI::get().mail().sendWarning("Shelly 3em wieder erreichbar", body.c_str());
      _countRequestsFailed = 0;
    }
    I::get().led().stop();
  }
  EWC::I::get().logger() << F("Shelly3emConnector: request finished... sleep ") << endl;
  _sleepUntil = I::get().time().str(PZI::get().config().checkInterval);
  // sleeper().sleep(PZI::get().config().checkInterval * 1000);
  // EWC::I::get().logger() << F("sleep for ") << PZI::get().config().checkInterval << "sec" << endl;
  _sleeper.sleep(PZI::get().config().checkInterval * 1000);
  _currentState = SLEEP;

  _isRequesting = false;
  vTaskDelete(NULL);
}
