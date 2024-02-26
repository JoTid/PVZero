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
#ifndef PVZ_CONFIG_H
#define PVZ_CONFIG_H

#include <map>
#include <Arduino.h>
#include "ewcConfigInterface.h"
#include "pvzero_interface.h"
#include "defaults.h"

namespace PVZ
{

  class Config : public EWC::ConfigInterface
  {
  public:
    Config();
    ~Config();
    void setup(JsonDocument &config, bool resetConfig = false);
    void fillJson(JsonDocument &config);
    void fromJson(JsonDocument &config);
    /** === parameter  === **/
    int checkInterval;
    String shelly3emAddr;
    float maxVoltage;
    float maxAmperage;
    bool enableSecondPsu;

    // LCD enabled
    bool enabledLcd;
    // taster configuration parameter
    uint8_t tasterFunc;

  protected:
    void _initParameter();
  };
}; // namespace

#endif
