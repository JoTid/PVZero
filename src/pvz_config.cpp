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
#include "generated/pvzeroSetupHTML.h"
#include "pvz_config.h"
#include "pvzero_class.h"

using namespace EWC;
using namespace PVZ;

PVZConfig::PVZConfig() : EWC::ConfigInterface("pvzero")
{
}

PVZConfig::~PVZConfig()
{
}

void PVZConfig::setup(JsonDocument &config, bool resetConfig)
{
  _initParameter();
  fromJson(config);
  EWC::I::get().server().insertMenuG("Device", "/pvzero/setup", "menu_device", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_PVZERO_SETUP_GZIP, sizeof(HTML_PVZERO_SETUP_GZIP), true, 0);
  WebServer *ws = &EWC::I::get().server().webServer();
  EWC::I::get().server().webServer().on("/pvzero/config.json", std::bind(&PVZConfig::_onConfigGet, this, ws));
  EWC::I::get().server().webServer().on("/pvzero/config/save", std::bind(&PVZConfig::_onConfigSave, this, ws));
  EWC::I::get().server().webServer().on("/pvzero/calibration/low", std::bind(&PVZConfig::_onCalibrationLow, this, ws));
  EWC::I::get().server().webServer().on("/pvzero/calibration/high", std::bind(&PVZConfig::_onCalibrationHigh, this, ws));
}

void PVZConfig::fillJson(JsonDocument &config)
{
  config["pvzero"]["name"] = EWC::I::get().config().paramDeviceName;
  config["pvzero"]["version"] = EWC::I::get().server().version();
  config["pvzero"]["check_interval"] = _checkInterval;
  config["pvzero"]["shelly3emAddr"] = _shelly3emAddr;
  config["pvzero"]["filter_order"] = _filterOrder;
  config["pvzero"]["max_voltage"] = String(_maxVoltage, 2);
  config["pvzero"]["max_amperage"] = String(_maxAmperage, 2);
  config["pvzero"]["enable_lcd"] = _enabledLcd;
  config["pvzero"]["enable_second_psu"] = _enableSecondPsu;
  config["pvzero"]["calibration_low"] = String(_calibrationLow, 2);
  config["pvzero"]["calibration_low_mes"] = String(_calibrationLowMes, 0);
  config["pvzero"]["calibration_high"] = String(_calibrationHigh, 2);
  config["pvzero"]["calibration_high_mes"] = String(_calibrationHighMes, 0);
}

void PVZConfig::fromJson(JsonDocument &config)
{
  JsonVariant jv = config["pvzero"]["check_interval"];
  if (!jv.isNull())
  {
    _checkInterval = jv.as<int>();
  }
  jv = config["pvzero"]["shelly3emAddr"];
  if (!jv.isNull())
  {
    _shelly3emAddr = jv.as<String>();
  }
  jv = config["pvzero"]["filter_order"];
  if (!jv.isNull())
  {
    _filterOrder = jv.as<int>();
    PZI::get().pvz().clCaP.setFilterOrder(_filterOrder);
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
    if (_enableSecondPsu)
    {
      EWC::I::get().logger().setLogging(false);
    }
    // if psu activated, we should disable the settings to disallow enable the logger again.
    EWC::I::get().config().disableLogSetting = _enableSecondPsu;
  }
  jv = config["pvzero"]["calibration_low"];
  if (!jv.isNull())
  {
    _calibrationLow = jv.as<float>();
  }
  jv = config["pvzero"]["calibration_low_mes"];
  if (!jv.isNull())
  {
    _calibrationLowMes = jv.as<float>();
  }
  jv = config["pvzero"]["calibration_high"];
  if (!jv.isNull())
  {
    _calibrationHigh = jv.as<float>();
  }
  jv = config["pvzero"]["calibration_high_mes"];
  if (!jv.isNull())
  {
    _calibrationHighMes = jv.as<float>();
  }
}

void PVZConfig::_initParameter()
{
  _checkInterval = 1;
  // deep_sleep("deep-sleep", "Deep sleep"),
  _shelly3emAddr = "";
  _filterOrder = 1;
  _maxVoltage = 36;
  _maxAmperage = 7;
  _enableSecondPsu = false;
  _enabledLcd = true;
  _calibrationLow = 24;
  _calibrationLowMes = -1;
  _calibrationHigh = 48;
  _calibrationHighMes = -1;
}

void PVZConfig::_onConfigGet(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  this->fillJson(jsonDoc);
  String output;
  serializeJson(jsonDoc, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void PVZConfig::_onConfigSave(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] save config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument config;
  if (webServer->hasArg("check_interval"))
  {
    long val = webServer->arg("check_interval").toInt();
    if (val > 0)
    {
      config["pvzero"]["check_interval"] = val;
    }
  }
  bool enableSecondPsu = false;
  if (webServer->hasArg("enable_second_psu"))
  {
    enableSecondPsu = webServer->arg("enable_second_psu").equals("true");
  }
  config["pvzero"]["enable_second_psu"] = enableSecondPsu;
  I::get().config().disableLogSetting = enableSecondPsu;
  bool enableLcd = false;
  if (webServer->hasArg("enable_lcd"))
  {
    enableLcd = webServer->arg("enable_lcd").equals("true");
  }
  config["pvzero"]["enable_lcd"] = enableLcd;
  if (webServer->hasArg("shelly3emAddr"))
  {
    String val = webServer->arg("shelly3emAddr");
    if (val.length() >= 0)
    {
      config["pvzero"]["shelly3emAddr"] = val;
    }
  }
  if (webServer->hasArg("filter_order"))
  {
    long val = webServer->arg("filter_order").toInt();
    if (val >= 0)
    {
      config["pvzero"]["filter_order"] = val;
    }
  }
  if (webServer->hasArg("max_voltage"))
  {
    float val = webServer->arg("max_voltage").toFloat();
    if (val >= 0)
    {
      config["pvzero"]["max_voltage"] = val;
    }
  }
  if (webServer->hasArg("max_voltage"))
  {
    I::get().logger() << F("Ampere: ") << webServer->arg("max_amperage") << endl;
    float val = webServer->arg("max_amperage").toFloat();
    if (val >= 0)
    {
      I::get().logger() << F("  Ampere value: ") << val << endl;
      config["pvzero"]["max_amperage"] = val;
    }
  }

  this->fromJson(config);
  I::get().configFS().save();
  String details;
  serializeJsonPretty(config["pvzero"], details);
  I::get().server().sendPageSuccess(webServer, "PVZ Config save", "Save successful!", "/pvzero/setup", "<pre id=\"json\">" + details + "</pre>");
}

void PVZConfig::_onCalibrationLow(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] calibration low request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument config;
  if (webServer->hasArg("calibration_low"))
  {
    float val = webServer->arg("calibration_low").toFloat();
    if (val > 0)
    {
      config["pvzero"]["calibration_low"] = val;
      // set low value and get result for calibration_low_mes
      if (_cbLow)
      {
        _calibrationLowMes = _cbLow(val);
      }
    }
  }
  this->fromJson(config);
  I::get().configFS().save();
  webServer->sendHeader("Location", "/pvzero/setup");
  webServer->send(302, "text/plain", "");
}

void PVZConfig::_onCalibrationHigh(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] calibration high request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument config;
  if (webServer->hasArg("calibration_high"))
  {
    float val = webServer->arg("calibration_high").toFloat();
    if (val > 0)
    {
      config["pvzero"]["calibration_high"] = val;
      // set high value and get result for calibration_high_mes
      if (_cbHigh)
      {
        _calibrationHighMes = _cbHigh(val);
      }
    }
  }
  this->fromJson(config);
  I::get().configFS().save();
  webServer->sendHeader("Location", "/pvzero/setup");
  webServer->send(302, "text/plain", "");
}
