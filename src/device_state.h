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
#ifndef PVZERO_deviceState_H
#define PVZERO_deviceState_H

#include "ewcConfigInterface.h"

namespace PVZERO {
class DeviceState {
public:
    enum State {
        SETUP,
        STANDALONE,
        INIT,
        RUNNING,
        SOFTSLEEP
    };

    DeviceState();
    ~DeviceState();
    /** === ConfigInterface Methods === **/
    void setup(bool resetConfig=false);
    void loop();

    /** === OWN Methods === **/

    bool ready();
    DeviceState::State currentState() { return _currentState; }
    String currentStateStr() { return state2string(_currentState); }
    bool setState(DeviceState::State newState, unsigned long msDelay=0);
    String state2string(DeviceState::State state);

protected:
    State _currentState;
    unsigned long _msDelay;
    unsigned long _msDelayStart;
    DeviceState::State _delayState;
};
}; // namespace

#endif
