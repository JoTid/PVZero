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
  config["pvzero"]["check_interval"] = checkInterval;
  config["pvzero"]["taster_func"] = tasterFunc;
  config["pvzero"]["shelly3emAddr"] = shelly3emAddr;
  config["pvzero"]["max_voltage"] = String(maxVoltage, 2);
  config["pvzero"]["max_amperage"] = String(maxAmperage, 2);
  config["pvzero"]["enable_lcd"] = enabledLcd;
  config["pvzero"]["enable_second_psu"] = enableSecondPsu;
}

void Config::fromJson(JsonDocument &config)
{
  JsonVariant jv = config["pvzero"]["check_interval"];
  if (!jv.isNull())
  {
    checkInterval = jv.as<int>();
  }
  jv = config["pvzero"]["taster_func"];
  if (!jv.isNull())
  {
    tasterFunc = jv.as<unsigned char>();
  }
  jv = config["pvzero"]["shelly3emAddr"];
  if (!jv.isNull())
  {
    shelly3emAddr = jv.as<String>();
  }
  jv = config["pvzero"]["max_voltage"];
  if (!jv.isNull())
  {
    maxVoltage = jv.as<float>();
  }
  jv = config["pvzero"]["max_amperage"];
  if (!jv.isNull())
  {
    maxAmperage = jv.as<float>();
  }
  jv = config["pvzero"]["enable_lcd"];
  if (!jv.isNull())
  {
    enabledLcd = jv.as<bool>();
  }
  jv = config["pvzero"]["enable_second_psu"];
  if (!jv.isNull())
  {
    enableSecondPsu = jv.as<bool>();
    if (enableSecondPsu) {
      EWC::I::get().logger().setLogging(false);
    }
  }
}

void Config::_initParameter()
{
  checkInterval = 1;
  // deep_sleep("deep-sleep", "Deep sleep"),
  shelly3emAddr = "";
  // taster
  tasterFunc = TASTER_CHECK_NOW;
  maxVoltage = 36;
  maxAmperage = 7;
  enableSecondPsu = false;
  enabledLcd = true;
}
