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
#ifndef PVZ_TASTER_H
#define PVZ_TASTER_H

#include <Arduino.h>

namespace PVZ {

#if defined(ESP8266)
#define MY_TASTER_PIN D3
#else
#define MY_TASTER_PIN 1
#endif

enum TasterFunctions {
    TASTER_NONE = 0,
    TASTER_CHECK_NOW = 1,
};

class Taster {
public:
    Taster(int pin=MY_TASTER_PIN);
    ~Taster();
    void setup(bool resetConfig=false);
    void loop();
    
protected:
    int _pin;
    unsigned long _tsPressed;
    bool _pressDetected;
    uint8_t _function;
};
}; // namespace

#endif
