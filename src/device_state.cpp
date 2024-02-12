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
#include "device_state.h"
#include "ewcLogger.h"

using namespace EWC;
using namespace PVZERO;

DeviceState::DeviceState()
{
    _msDelay = 0;
    _msDelayStart = 0;
    _delayState = State::SETUP;
    _currentState = State::SETUP;
}

DeviceState::~DeviceState()
{
}

void DeviceState::setup(bool resetConfig)
{
}

void DeviceState::loop()
{
    if (_msDelay && millis() - _msDelayStart > _msDelay)  {
        setState(_delayState);
    }
}

bool DeviceState::ready()
{
    return _currentState == State::RUNNING || _currentState == State::SOFTSLEEP;
}

bool DeviceState::setState(DeviceState::State newState, unsigned long msDelay)
{
    if (_currentState == newState) {
        return false;
    }
    _msDelay = msDelay;
    if (msDelay > 0) {
        _msDelayStart = millis();
        _delayState = newState;
        return true;
    }
    bool state_changed = false;
    switch(newState) {
        case SETUP:
            state_changed = true;
            break;
        case STANDALONE:
            state_changed = true;
            break;
        case INIT:
            state_changed = true;
            break;
        case RUNNING:
            if (_currentState == State::SOFTSLEEP || _currentState == State::INIT || _currentState == State::STANDALONE) {
                state_changed = true;
            }
            break;
        case SOFTSLEEP:
            if (_currentState == State::RUNNING) {
                state_changed = true;
            }
            break;
    }
    if (state_changed) {
        _currentState = newState;
        EWC::I::get().logger() << F("DeviceState: new state ") << state2string(newState) << endl;
    } else {
        EWC::I::get().logger() << F("DeviceState: cannot switch from ") << state2string(_currentState) << F(" into ") << state2string(newState) << endl;
    }
    return state_changed;
}

String DeviceState::state2string(DeviceState::State state)
{
    switch(state) {
        case SETUP:
            return "SETUP";
        case STANDALONE:
            return "STANDALONE";
        case INIT:
            return "INIT";
        case RUNNING:
            return "RUNNING";
        case SOFTSLEEP:
            return "SOFTSLEEP";
    }
    return "Unknown";
}
