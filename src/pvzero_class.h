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

#ifndef PVZ_CLASS_H
#define PVZ_CLASS_H

#include <Arduino.h>
#include <ewcConfigServer.h>
#include <ewcLogger.h>
#include <extensions/ewcUpdater.h>
#include <extensions/ewcMail.h>
#include <extensions/ewcMqtt.h>
#include <extensions/ewcMqttHA.h>
#include "pvz_config.h"
#include "shelly_3em_connector.h"
#include "pvz_ca.hpp"
#include "pvz_lcd.hpp"
#include "pvz_psu.hpp"
#include "sw_ovs.h"
#include "battery_guard.hpp"

namespace PVZ
{

#define FIRMWARE_VERSION "0.81.01"

  class PVZeroClass
  {

  public:
    PVZeroClass();
    ~PVZeroClass();
    void setup();
    void loop();

    PVZConfig &config() { return _config; }
    PvzCa clCaP;

  protected:
    unsigned long _tsStarted = 0;
    unsigned long _tsStartWaitForConnection = 0;
    unsigned long _tsMeasLoopStart = 0;
    unsigned long _maxPumpTime = 0;
    String _sleepInfoStr;
    PVZConfig _config;
    EWC::ConfigServer _ewcServer;
    EWC::Updater _ewcUpdater;
    EWC::Mqtt _ewcMqtt;
    EWC::MqttHA _ewcMqttHA;
    EWC::Mail _ewcMail;
    Shelly3emConnector _shelly3emConnector;
    PvzLcd _lcd;
    PvzPsu aclPsuP[2]; // support up to 2 PSUs
    BatteryGuard clBatGuardP;
    float ftPsuVccT = 0.0;
    int32_t consumptionPower = -1;
    bool isConsumptionPowerValid = false;
    McOvs_ts atsOvsInputsP[4]; // prepare software oversampling for up to 4 values
    float ftPsuSupplyGainP;
    float ftPsuSupplyOffsetP;
    PvzLcd::Screen_ts atsLcdScreenP[5];
    void _onPVZeroState(WebServer *webServer);
    void _onPVZeroCheck(WebServer *webServer);
    void _onTotalWatt(bool state, int32_t totalWatt);
    void processControlAlgorithm(void);
    float handleCalibrationLow(float value);
    float handleCalibrationHigh(float value);
    void updatePsuVccScaling(uint8_t ubSetupV);
  };
}; // namespace

#endif
