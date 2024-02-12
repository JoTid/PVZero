/**************************************************************

This file is a part of
https://github.com/JoTid/PVZero

Copyright [2020] Johann Tiderko

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
#ifndef PVZERO_LED_H
#define PVZERO_LED_H

#include <Arduino.h>

namespace PVZERO {

class LCD {
public:
    LCD();
    ~LCD();
    void setup(bool resetConfig=false);
    void loop();
    
protected:
};
}; // namespace

#endif
