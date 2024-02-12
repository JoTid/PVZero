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
#include "taster.h"
#ifdef ESP8266
    #include <ewcRTC.h>
#endif
#include <ewcLogger.h>
#include "shelly_em3_connector.h"
#include "config.h"


using namespace EWC;
using namespace PVZERO;

Taster::Taster(int pin) 
{
    _pin = pin;
    _tsPressed = 0;
    _pressDetected = false;
    _function = TASTER_NONE;
}

Taster::~Taster()
{
}

void Taster::setup(bool resetConfig)
{
    // pinMode(_pin, INPUT);
}

void Taster::loop()
{
    if (PZI::get().config().tasterFunc != TASTER_NONE) {
        int value = digitalRead(_pin);
        if (value > 0) {
            // button pressed
            if (_tsPressed == 0) {
                // set timestamp for debounce
                _tsPressed = millis();
            } else if (millis() -_tsPressed > 50) {
                _pressDetected = true;
            }
        } else {
            // button released, reset timestamp
            _tsPressed = 0;
            if (_pressDetected) {
                // execute action
                switch (PZI::get().config().tasterFunc) {
                case TASTER_CHECK_NOW:
                    I::get().logger() << "[Taster] trigger check now" << endl;
                    // PZI::get().shellyEm3Connector().sleeper().wakeup();
                    break;
                default:
                    I::get().logger() << "[Taster] undefined action: " << PZI::get().config().tasterFunc << endl;
                    break;
                }
                _pressDetected = false;
            }
        }
    }
}
