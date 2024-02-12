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
#ifndef PVZERO_INTERFACE_H
#define PVZERO_INTERFACE_H

#include <Arduino.h>

namespace EWC {
  class ConfigServer;
  class Time;
};

namespace PVZERO {
class PVZeroClass;
class Config;
class DeviceState;
class ShellyEm3Connector;
class LCD;

class InterfaceData {
  friend PVZeroClass;

  public:
    InterfaceData();

    PVZeroClass& pvzero() { return *_pvzero; }
    ShellyEm3Connector& shellyEm3Connector() { return *_shellyEm3Connector; }
    EWC::Time& time() { return *_time; }
    Config& config() { return *_config; }
    DeviceState& deviceState() { return *_deviceState; }
    EWC::ConfigServer& ewcServer() { return *_ewcServer; }
    LCD& lcd() { return *_lcd; }

  private:
    PVZeroClass* _pvzero;
    EWC::Time* _time;
    Config* _config;
    DeviceState* _deviceState;
    ShellyEm3Connector *_shellyEm3Connector;
    EWC::ConfigServer* _ewcServer;
    LCD* _lcd;
};

class PZI {
  public:
    static PVZERO::InterfaceData& get();

  private:
    static PVZERO::InterfaceData _interface;
};

}

#endif
