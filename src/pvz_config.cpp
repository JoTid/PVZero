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
#include "generated/caSetupHTML.h"
#include "generated/peripheralsSetupHTML.h"
#include "generated/shellySetupHTML.h"
#include "pvz_config.h"
#include "pvzero_class.h"

using namespace EWC;
using namespace PVZ;

PVZConfig::PVZConfig() : EWC::ConfigInterface("pvz")
{
}

PVZConfig::~PVZConfig()
{
}

void PVZConfig::setup(JsonDocument &config, bool resetConfig)
{
  _initParameter();
  fromJson(config);
  EWC::I::get().server().insertMenuG("Peripherals", "/pvz/peripherals/setup", "menu_peripherals", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_PERIPHERALS_SETUP_GZIP, sizeof(HTML_PERIPHERALS_SETUP_GZIP), true, 0);
  EWC::I::get().server().insertMenuG("Shelly", "/pvz/shelly/setup", "menu_shelly", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_SHELLY_SETUP_GZIP, sizeof(HTML_SHELLY_SETUP_GZIP), true, 0);
  EWC::I::get().server().insertMenuG("CA", "/pvz/ca/setup", "menu_ca", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_CA_SETUP_GZIP, sizeof(HTML_CA_SETUP_GZIP), true, 0);
  WebServer *ws = &EWC::I::get().server().webServer();
  EWC::I::get().server().webServer().on("/pvz/ca/config.json", std::bind(&PVZConfig::_onCfgCaGet, this, ws));
  EWC::I::get().server().webServer().on("/pvz/ca/config/save", std::bind(&PVZConfig::_onCfgCaSave, this, ws));
  EWC::I::get().server().webServer().on("/pvz/shelly/config.json", std::bind(&PVZConfig::_onCfgShellyGet, this, ws));
  EWC::I::get().server().webServer().on("/pvz/shelly/config/save", std::bind(&PVZConfig::_onCfgShellySave, this, ws));
  EWC::I::get().server().webServer().on("/pvz/peripherals/config.json", std::bind(&PVZConfig::_onCfgPeripheralsGet, this, ws));
  EWC::I::get().server().webServer().on("/pvz/lcd/enable", std::bind(&PVZConfig::_onLcdEnable, this, ws));
  EWC::I::get().server().webServer().on("/pvz/led/enable", std::bind(&PVZConfig::_onLedEnable, this, ws));
  EWC::I::get().server().webServer().on("/pvz/analog/enable", std::bind(&PVZConfig::_onAnalogEnable, this, ws));
  EWC::I::get().server().webServer().on("/pvz/analog/calibration/low", std::bind(&PVZConfig::_onCalibrationLow, this, ws));
  EWC::I::get().server().webServer().on("/pvz/analog/calibration/high", std::bind(&PVZConfig::_onCalibrationHigh, this, ws));
}

void PVZConfig::fillJson(JsonDocument &config)
{
  // config["pvz"]["name"] = EWC::I::get().config().paramDeviceName;
  // config["pvz"]["version"] = EWC::I::get().server().version();
  _fillCaJson(config);
  _fillShellyJson(config);
  _fillPeripheralsJson(config);
}

void PVZConfig::fromJson(JsonDocument &config)
{
  JsonVariant jv = config["shelly"]["check_interval"];
  if (!jv.isNull())
  {
    _checkInterval = jv.as<int>();
  }
  jv = config["shelly"]["shelly3emAddr"];
  if (!jv.isNull())
  {
    _shelly3emAddr = jv.as<String>();
  }
  jv = config["ca"]["filter_order"];
  if (!jv.isNull())
  {
    _filterOrder = jv.as<int>();
    PZI::get().pvz().clCaP.setFilterOrder(_filterOrder);
  }
  jv = config["ca"]["max_voltage"];
  if (!jv.isNull())
  {
    _maxVoltage = jv.as<float>();
  }
  jv = config["ca"]["max_amperage"];
  if (!jv.isNull())
  {
    _maxAmperage = jv.as<float>();
  }
  jv = config["ca"]["max_power_if_unknown"];
  if (!jv.isNull())
  {
    _maxPowerIfUnknown = jv.as<int>();
  }
  jv = config["peripherals"]["enable_lcd"];
  if (!jv.isNull())
  {
    _setLcd(jv.as<bool>());
  }
  jv = config["peripherals"]["enable_led"];
  if (!jv.isNull())
  {
    _setLed(jv.as<bool>());
  }
  jv = config["peripherals"]["enable_analog"];
  if (!jv.isNull())
  {
    _setAnalog(jv.as<bool>());
  }
  jv = config["peripherals"]["calibration_low"];
  if (!jv.isNull())
  {
    _calibrationLow = jv.as<float>();
  }
  jv = config["peripherals"]["calibration_low_mes"];
  if (!jv.isNull())
  {
    _calibrationLowMes = jv.as<float>();
  }
  jv = config["peripherals"]["calibration_high"];
  if (!jv.isNull())
  {
    _calibrationHigh = jv.as<float>();
  }
  jv = config["peripherals"]["calibration_high_mes"];
  if (!jv.isNull())
  {
    _calibrationHighMes = jv.as<float>();
  }
}

void PVZConfig::_initParameter()
{
  _checkInterval = 10;
  // deep_sleep("deep-sleep", "Deep sleep"),
  _shelly3emAddr = "";

  _filterOrder = 1;
  _maxVoltage = 36;
  _maxAmperage = 7;
  _maxPowerIfUnknown = 260;

  _enabledLcd = true;
  _enabledLed = true;
  _enabledAnalog = false;
  _calibrationLow = 24;
  _calibrationLowMes = -1;
  _calibrationHigh = 48;
  _calibrationHighMes = -1;
}

void PVZConfig::_onCfgCaGet(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] ca config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  this->_fillCaJson(jsonDoc);
  String output;
  serializeJson(jsonDoc, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}
void PVZConfig::_onCfgCaSave(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] save ca config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument config;
  if (webServer->hasArg("filter_order"))
  {
    long val = webServer->arg("filter_order").toInt();
    if (val >= 0)
    {
      config["ca"]["filter_order"] = val;
    }
  }
  if (webServer->hasArg("max_voltage"))
  {
    float val = webServer->arg("max_voltage").toFloat();
    if (val >= 0)
    {
      config["ca"]["max_voltage"] = val;
    }
  }
  if (webServer->hasArg("max_voltage"))
  {
    I::get().logger() << F("Ampere: ") << webServer->arg("max_amperage") << endl;
    float val = webServer->arg("max_amperage").toFloat();
    if (val >= 0)
    {
      I::get().logger() << F("  Ampere value: ") << val << endl;
      config["ca"]["max_amperage"] = val;
    }
  }
  if (webServer->hasArg("max_power_if_unknown"))
  {
    long val = webServer->arg("max_power_if_unknown").toInt();
    if (val >= 0)
    {
      config["ca"]["max_power_if_unknown"] = val;
    }
  }
  this->fromJson(config);
  I::get().configFS().save();
  String details;
  serializeJsonPretty(config["ca"], details);
  I::get().server().sendPageSuccess(webServer, "PVZ Config save", "Save successful!", "/pvz/ca/setup", "<pre id=\"json\">" + details + "</pre>");
}

void PVZConfig::_fillCaJson(JsonDocument &config)
{
  config["ca"]["filter_order"] = _filterOrder;
  config["ca"]["max_voltage"] = String(_maxVoltage, 2);
  config["ca"]["max_amperage"] = String(_maxAmperage, 2);
  config["ca"]["max_power_if_unknown"] = _maxPowerIfUnknown;
}

void PVZConfig::_onCfgShellyGet(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] shelly config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  this->_fillShellyJson(jsonDoc);
  String output;
  serializeJson(jsonDoc, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}
void PVZConfig::_onCfgShellySave(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] save shelly config request") << endl;
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
      config["shelly"]["check_interval"] = val;
    }
  }
  if (webServer->hasArg("shelly3emAddr"))
  {
    String val = webServer->arg("shelly3emAddr");
    if (val.length() >= 0)
    {
      config["shelly"]["shelly3emAddr"] = val;
    }
  }
  this->fromJson(config);
  I::get().configFS().save();
  String details;
  serializeJsonPretty(config["shelly"], details);
  I::get().server().sendPageSuccess(webServer, "PVZ Shelly config save", "Save successful!", "/pvz/shelly/setup", "<pre id=\"json\">" + details + "</pre>");
  PZI::get().shelly3emConnector().forceUpdate();
}

void PVZConfig::_fillShellyJson(JsonDocument &config)
{
  config["shelly"]["check_interval"] = _checkInterval;
  config["shelly"]["shelly3emAddr"] = _shelly3emAddr;
}

void PVZConfig::_onCfgPeripheralsGet(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] peripherals config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  this->_fillPeripheralsJson(jsonDoc);
  String output;
  serializeJson(jsonDoc, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}
void PVZConfig::_onCfgPeripheralsSave(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] save peripheral config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument config;
}

void PVZConfig::_fillPeripheralsJson(JsonDocument &config)
{
  config["peripherals"]["enable_lcd"] = _enabledLcd;
  config["peripherals"]["enable_led"] = _enabledLed;
  config["peripherals"]["enable_analog"] = _enabledAnalog;
  config["peripherals"]["calibration_low"] = String(_calibrationLow, 2);
  config["peripherals"]["calibration_low_mes"] = String(_calibrationLowMes, 0);
  config["peripherals"]["calibration_high"] = String(_calibrationHigh, 2);
  config["peripherals"]["calibration_high_mes"] = String(_calibrationHighMes, 0);
}

void PVZConfig::_onLcdEnable(WebServer *webServer)
{
  if (!I::get().server().isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  if (webServer->hasArg("enable_lcd"))
  {
    _setLcd(webServer->arg("enable_lcd").equals("on"));
  }
  else
  {
    _setLcd(false);
  }
  I::get().configFS().save();
  I::get().server().sendRedirect(webServer, "/pvz/peripherals/setup");
}

void PVZConfig::_onLedEnable(WebServer *webServer)
{
  if (!I::get().server().isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  if (webServer->hasArg("enable_led"))
  {
    _setLed(webServer->arg("enable_led").equals("on"));
  }
  else
  {
    _setLed(false);
  }
  I::get().configFS().save();
  I::get().server().sendRedirect(webServer, "/pvz/peripherals/setup");
}

void PVZConfig::_onAnalogEnable(WebServer *webServer)
{
  if (!I::get().server().isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  // I::get().logger() << "[EWC CS]: handle args: " << webServer->args() << endl;
  // for (int i = 0; i < webServer->args(); i++)
  // {
  //   I::get().logger() << "[EWC CS]:   " << webServer->argName(i) << ": " << webServer->arg(i) << endl;
  // }
  if (webServer->hasArg("enable_analog"))
  {
    _setAnalog(webServer->arg("enable_analog").equals("on"));
  }
  else
  {
    _setAnalog(false);
  }
  I::get().configFS().save();
  I::get().server().sendRedirect(webServer, "/pvz/peripherals/setup");
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
      config["peripherals"]["calibration_low"] = val;
      // set low value and get result for calibration_low_mes
      if (_cbLow)
      {
        _calibrationLowMes = _cbLow(val);
      }
    }
  }
  this->fromJson(config);
  I::get().configFS().save();
  I::get().server().sendRedirect(webServer, "/pvz/peripherals/setup");
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
      config["peripherals"]["calibration_high"] = val;
      // set high value and get result for calibration_high_mes
      if (_cbHigh)
      {
        _calibrationHighMes = _cbHigh(val);
      }
    }
  }
  this->fromJson(config);
  I::get().configFS().save();
  I::get().server().sendRedirect(webServer, "/pvz/peripherals/setup");
}

void PVZConfig::_setLcd(bool state)
{
  if (!state && _enabledLcd)
  {
    // clear LCD display
    PZI::get().pvz().clearLcd();
  }
  else
  {
    if (state && !_enabledLcd)
    {
      // enable LCD display
      PZI::get().pvz().enableLcd();
    }
  }
  _enabledLcd = state;
}

void PVZConfig::_setLed(bool state)
{
  I::get().led().enable(state);
  _enabledLed = state;
}

void PVZConfig::_setAnalog(bool state)
{
  _enabledAnalog = state;
}