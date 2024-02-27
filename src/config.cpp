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
#include <Arduino.h>
#include <ewcInterface.h>
#include <ewcConfigServer.h>
#include <ewcLogger.h>
#include "config.h"
#include "taster.h"

using namespace PVZ;

Config::Config() : EWC::ConfigInterface("pvzero")
{
}

Config::~Config()
{
}

void Config::setup(JsonDocument &config, bool resetConfig)
{
  _initParameter();
  fromJson(config);
}

void Config::fillJson(JsonDocument &config)
{
  config["pvzero"]["name"] = EWC::I::get().config().paramDeviceName;
  config["pvzero"]["version"] = EWC::I::get().server().version();
  config["pvzero"]["check_interval"] = _checkInterval;
  config["pvzero"]["taster_func"] = _tasterFunc;
  config["pvzero"]["shelly3emAddr"] = _shelly3emAddr;
  config["pvzero"]["max_voltage"] = String(_maxVoltage, 2);
  config["pvzero"]["max_amperage"] = String(_maxAmperage, 2);
  config["pvzero"]["enable_lcd"] = _enabledLcd;
  config["pvzero"]["enable_second_psu"] = _enableSecondPsu;
}

void Config::fromJson(JsonDocument &config)
{
  JsonVariant jv = config["pvzero"]["check_interval"];
  if (!jv.isNull())
  {
    _checkInterval = jv.as<int>();
  }
  jv = config["pvzero"]["taster_func"];
  if (!jv.isNull())
  {
    _tasterFunc = jv.as<unsigned char>();
  }
  jv = config["pvzero"]["shelly3emAddr"];
  if (!jv.isNull())
  {
    _shelly3emAddr = jv.as<String>();
  }
  jv = config["pvzero"]["max_voltage"];
  if (!jv.isNull())
  {
    _maxVoltage = jv.as<float>();
  }
  jv = config["pvzero"]["max_amperage"];
  if (!jv.isNull())
  {
    _maxAmperage = jv.as<float>();
  }
  jv = config["pvzero"]["enable_lcd"];
  if (!jv.isNull())
  {
    _enabledLcd = jv.as<bool>();
  }
  jv = config["pvzero"]["enable_second_psu"];
  if (!jv.isNull())
  {
    _enableSecondPsu = jv.as<bool>();
    if (_enableSecondPsu) {
      EWC::I::get().logger().setLogging(false);
    }
    // if psu activated, we should disable the settings to disallow enable the logger again.
    EWC::I::get().config().disableLogSetting = _enableSecondPsu;
  }
}

void Config::_initParameter()
{
  _checkInterval = 1;
  // deep_sleep("deep-sleep", "Deep sleep"),
  _shelly3emAddr = "";
  // taster
  _tasterFunc = TASTER_CHECK_NOW;
  _maxVoltage = 36;
  _maxAmperage = 7;
  _enableSecondPsu = false;
  _enabledLcd = true;
}
