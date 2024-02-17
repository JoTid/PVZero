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
#ifndef PVZ_INTERFACE_H
#define PVZ_INTERFACE_H

#include <Arduino.h>

namespace EWC {
  class ConfigServer;
  class Time;
};

namespace PVZ {
class PVZeroClass;
class Config;
class DeviceState;
class Shelly3emConnector;
class LCD;

class InterfaceData {
  friend PVZeroClass;

  public:
    InterfaceData();

    PVZeroClass& pvz() { return *_pvz; }
    Shelly3emConnector& shelly3emConnector() { return *_shelly3emConnector; }
    EWC::Time& time() { return *_time; }
    Config& config() { return *_config; }
    DeviceState& deviceState() { return *_deviceState; }
    EWC::ConfigServer& ewcServer() { return *_ewcServer; }
    LCD& lcd() { return *_lcd; }

  private:
    PVZeroClass* _pvz;
    EWC::Time* _time;
    Config* _config;
    DeviceState* _deviceState;
    Shelly3emConnector *_shelly3emConnector;
    EWC::ConfigServer* _ewcServer;
    LCD* _lcd;
};

class PZI {
  public:
    static PVZ::InterfaceData& get();

  private:
    static PVZ::InterfaceData _interface;
};

}

#endif
