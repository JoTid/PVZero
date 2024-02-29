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
#include <ewcConfigServer.h>
#include "ewcConfigInterface.h"
#include "pvzero_interface.h"
#include "defaults.h"

namespace PVZ
{

  // takes user defined calibration value and returns measured value
  typedef std::function<float(float)> CalibrationCallback;

  class PVZConfig : public EWC::ConfigInterface
  {
  public:
    PVZConfig();
    ~PVZConfig();
    void setup(JsonDocument &config, bool resetConfig = false);
    void fillJson(JsonDocument &config);
    void fromJson(JsonDocument &config);
    /** === callbacks  === **/
    void setCalibrationLowCallback(CalibrationCallback cb) { _cbLow = cb; }
    void setCalibrationHighCallback(CalibrationCallback cb) { _cbHigh = cb; }
    /** === parameter  === **/
    int getCheckInterval() { return _checkInterval; }
    String getShelly3emAddr() { return _shelly3emAddr; }
    uint8_t getFilterOrder() { return _filterOrder; }
    float getMaxVoltage() { return _maxVoltage; }
    float getMaxAmperage() { return _maxAmperage; }
    bool isEnabledSecondPsu() { return _enableSecondPsu; }
    // LCD enabled
    bool isEnabledLcd() { return _enabledLcd; }

  protected:
    void _initParameter();
    int _checkInterval;
    String _shelly3emAddr;
    int _filterOrder;
    float _maxVoltage;
    float _maxAmperage;
    bool _enableSecondPsu;
    bool _enabledLcd;
    float _calibrationLow;
    float _calibrationLowMes;
    float _calibrationHigh;
    float _calibrationHighMes;

    CalibrationCallback _cbLow;
    CalibrationCallback _cbHigh;

    void _onConfigGet(WebServer *webServer);
    void _onConfigSave(WebServer *webServer);
    void _onCalibrationLow(WebServer *webServer);
    void _onCalibrationHigh(WebServer *webServer);
  };
}; // namespace

#endif
