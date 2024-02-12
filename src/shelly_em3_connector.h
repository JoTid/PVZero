/**************************************************************

This file is a part of Solar EinspeseRegelungsSystem mit Shelly Em3
https://github.com/JoTid/PVZero

Copyright [2020] Alexander Tiderko

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

**************************************************************/
#ifndef PVZERO_SHELLY_EM3_CONNECTOR_H
#define PVZERO_SHELLY_EM3_CONNECTOR_H

#include <Arduino.h>
#include "sleeper.h"

namespace PVZERO {
    typedef std::function<void(bool, int)> SellyStateCallback;

class ShellyEm3Connector {
public:
    enum State {
        UNKNOWN,
        SLEEP,
        ON_CHECK,
        DO_NOT_DISTURB
    };
    // Sleeper& sleeper() { return *_sleeper; }
    bool mailStateChanged;  // use for state detection for e-mail send
    ShellyEm3Connector(int potPin=A0);
    ~ShellyEm3Connector();
    void setup(bool resetConfig=false);
    void loop();
    void setCallbackState(SellyStateCallback callback) { _callbackState = callback; }
    String state2string(ShellyEm3Connector::State state);
    String info() { return _infoState; }
    String infoSleepUntil() { return _sleepUntil; }
    int currentExcess() { return _currentExcess; }
    int currentCurrent() { return _currentCurrent; }

protected:
    int _potPin;
#ifdef ESP8266
    uint8_t _utcAddress;
#endif
    unsigned int _reachedUpperLimit;
    SellyStateCallback _callbackState;
    State _currentState;
    Sleeper _sleeper;
    String _infoState;
    String _sleepUntil;
    long _currentExcess;
    long _currentCurrent;
};
}; // namespace

#endif
