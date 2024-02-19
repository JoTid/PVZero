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
#ifndef PVZ_SHELLY_3EM_CONNECTOR_H
#define PVZ_SHELLY_3EM_CONNECTOR_H

#include <WiFiClient.h>
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
#include <HttpClient.h>
#endif
#include <Arduino.h>
#include "sleeper.h"

namespace PVZ {
    typedef std::function<void(bool, int)> SellyStateCallback;

class Shelly3emConnector {
public:
    enum State {
        UNKNOWN,
        SLEEP,
        ON_CHECK,
        DO_NOT_DISTURB
    };
    // Sleeper& sleeper() { return *_sleeper; }
    bool mailStateChanged;  // use for state detection for e-mail send
    Shelly3emConnector(int potPin=A0);
    ~Shelly3emConnector();
    void setup(bool resetConfig=false);
    void loop();
    void setCallbackState(SellyStateCallback callback) { _callbackState = callback; }
    String state2string(Shelly3emConnector::State state);
    String info() { return _infoState; }
    String infoSleepUntil() { return _sleepUntil; }
    int consumptionPower() { return _consumptionPower; }
    int feedInPower() { return _feedInPower; }
    bool isValid() {return btIsValidP; }

  protected:
    int _potPin;
#ifdef ESP8266
    uint8_t _utcAddress;
#endif
    WiFiClient _wifiClient;
    HTTPClient _httpClient;
    unsigned int _reachedUpperLimit;
    SellyStateCallback _callbackState;
    State _currentState;
    Sleeper _sleeper;
    String _infoState;
    String _sleepUntil;
    bool _isRequesting;
    long _consumptionPower;
    long _feedInPower;
    long _countRequestsFailed;
    long btIsValidP;

    void httpTask();
    static void startTaskImpl(void *);
};
}; // namespace

#endif
