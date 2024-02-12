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

#ifndef PVZERO_CLASS_H
#define PVZERO_CLASS_H

#include <Arduino.h>
#include <ewcConfigServer.h>
#include <ewcLogger.h>
#include <extensions/ewcTime.h>
#include <extensions/ewcUpdater.h>
#include <extensions/ewcMail.h>
#include <extensions/ewcMqtt.h>
#include "config.h"
#include "device_state.h"
#include "shelly_em3_connector.h"
// #include "sleeper.h"

#include "taster.h"
#include "lcd.h"

namespace PVZERO {

class PVZeroClass {

public:
    PVZeroClass();
    ~PVZeroClass();
    void setup();
    void loop();

    Config& config() { return _config; }

protected:
    unsigned long _tsStarted = 0;
    unsigned long _tsStartWaitForConnection = 0;
    unsigned long _tsMeasLoopStart = 0;
    unsigned long _maxPumpTime = 0;
    bool _timePrinted;
    String _sleepInfoStr;
    Config _config;
    EWC::ConfigServer _ewcServer;
    EWC::Time _ewcTime;
    EWC::Updater _ewcUpdater;
    EWC::Mqtt _ewcMqtt;
    EWC::Mail _ewcMail;
    DeviceState _deviceState;
    ShellyEm3Connector _shellyEm3Connector;
    Taster _taster;
    LCD _lcd;
    void _onPVZeroConfig(WebServer* webserver);
    void _onPVZeroSave(WebServer* webserver);
    void _onPVZeroState(WebServer* webserver);
    void _onPVZeroCheck(WebServer* webserver);
    // void _onBbsPump1(WebServer* webserver);
    // void _onBbsPump2(WebServer* webserver);
    void _onPotState(bool state, int duration);
};
}; // namespace

#endif
