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
    // LCD enabled
    bool isEnabledLcd() { return _enabledLcd; }
    // LED enabled
    bool isEnabledLed() { return _enabledLed; }
    // Analog enabled
    bool isEnabledAnalog() { return _enabledAnalog; }

    float getCalibrationRawLow() { return _calibrationLowMes; }
    float getCalibrationRawHigh() { return _calibrationHighMes; }
    float getCalibrationNominalLow() { return _calibrationLow; }
    float getCalibrationNominalHigh() { return _calibrationHigh; }

  protected:
    void _initParameter();
    int _checkInterval;
    String _shelly3emAddr;
    int _filterOrder;
    float _maxVoltage;
    float _maxAmperage;
    int _maxPowerIfUnknown;
    bool _enabledLcd;
    bool _enabledLed;
    bool _enabledAnalog;
    float _calibrationLow;
    float _calibrationLowMes;
    float _calibrationHigh;
    float _calibrationHighMes;

    CalibrationCallback _cbLow;
    CalibrationCallback _cbHigh;

    void _onCfgCaGet(WebServer *webServer);
    void _onCfgCaSave(WebServer *webServer);
    void _onCfgShellyGet(WebServer *webServer);
    void _onCfgShellySave(WebServer *webServer);
    void _onCfgPeripheralsGet(WebServer *webServer);
    void _onCfgPeripheralsSave(WebServer *webServer);

    void _onLcdEnable(WebServer *webServer);
    void _onLedEnable(WebServer *webServer);
    void _onAnalogEnable(WebServer *webServer);
    void _onCalibrationLow(WebServer *webServer);
    void _onCalibrationHigh(WebServer *webServer);

    void _fillCaJson(JsonDocument &config);
    void _fillShellyJson(JsonDocument &config);
    void _fillPeripheralsJson(JsonDocument &config);

    void _setLcd(bool state);
    void _setLed(bool state);
    void _setAnalog(bool state);
  };
}; // namespace

#endif
