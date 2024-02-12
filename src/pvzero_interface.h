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
#ifndef PVZERO_INTERFACE_H
#define PVZERO_INTERFACE_H

#include <Arduino.h>

namespace EWC {
  class ConfigServer;
  class Time;
};

namespace PVZERO {
class PVZEROClass;
class Config;
class DeviceState;
class ShellyEm3Connector;

class InterfaceData {
  friend PVZEROClass;

  public:
    InterfaceData();

    PVZEROClass& pvzero() { return *_pvzero; }
    ShellyEm3Connector& shellyEm3Connector() { return *_shellyEm3Connector; }
    EWC::Time& time() { return *_time; }
    Config& config() { return *_config; }
    DeviceState& deviceState() { return *_deviceState; }
    EWC::ConfigServer& ewcServer() { return *_ewcServer; }

  private:
    PVZEROClass* _pvzero;
    EWC::Time* _time;
    Config* _config;
    DeviceState* _deviceState;
    ShellyEm3Connector *_shellyEm3Connector;
    EWC::ConfigServer* _ewcServer;    
};

class SI {
  public:
    static PVZERO::InterfaceData& get();

  private:
    static PVZERO::InterfaceData _interface;
};

}

#endif
