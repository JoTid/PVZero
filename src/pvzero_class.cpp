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
#include <ewcConfigServer.h>
#include "generated/pvzeroSetupHTML.h"
#include "generated/webIndexHTML.h"
#include "generated/webLanguagesJSON.h"
#include "generated/webShelly_3em_connectorJS.h"
#include "pvzero_class.h"
#include "pvzero_interface.h"

using namespace EWC;
using namespace PVZ;

#if defined(ESP8266)
#define MY_SHELLY_PIN D6
#else
#define MY_SHELLY_PIN 6
#endif

PvzLcd::Screen_ts atsLcdScreenG[5]; // make screen of LCD available for whole application

PVZeroClass::PVZeroClass()
    : _shelly3emConnector(MY_SHELLY_PIN) // pinPot=D6
{
  PZI::get()._pvz = this;
  PZI::get()._config = &_config;
  PZI::get()._ewcServer = &_ewcServer;
  PZI::get()._lcd = &_lcd;
  PZI::get()._shelly3emConnector = &_shelly3emConnector;
  PZI::get()._lcd = &_lcd;
}

PVZeroClass::~PVZeroClass()
{
}

void PVZeroClass::setup()
{
  //---------------------------------------------------------------------------------------------------
  // initialise the LCD and trigger first display
  //
  _lcd.init(FIRMWARE_VERSION);
  _lcd.process();

  EWC::I::get().configFS().addConfig(_ewcUpdater);
  EWC::I::get().configFS().addConfig(_ewcMail);
  EWC::I::get().configFS().addConfig(_ewcMqtt);
  EWC::I::get().configFS().addConfig(_config);
  // EWC::I::get().led().enable(true, LED_BUILTIN, LOW);
  EWC::I::get().server().enableConfigUri();
  EWC::I::get().server().setup();
  EWC::I::get().config().paramLanguage = "de";
  EWC::I::get().server().insertMenuG("Device", "/pvzero/setup", "menu_device", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_PVZERO_SETUP_GZIP, sizeof(HTML_PVZERO_SETUP_GZIP), true, 0);
  WebServer *ws = &EWC::I::get().server().webServer();
  EWC::I::get().logger() << F("Setup WebServer") << endl;
  EWC::I::get().server().webServer().on(HOME_URI, std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_WEB_INDEX_GZIP, sizeof(HTML_WEB_INDEX_GZIP)));
  EWC::I::get().server().webServer().on("/languages.json", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), JSON_WEB_LANGUAGES_GZIP, sizeof(JSON_WEB_LANGUAGES_GZIP)));
  EWC::I::get().server().webServer().on("/pvzero/config.json", std::bind(&PVZeroClass::_onPVZeroConfig, this, ws));
  // EWC::I::get().server().webServer().on("/bbs/cycle.svg", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, "image/svg+xml", SVG_BBS_CYCLE_GZIP, sizeof(SVG_BBS_CYCLE_GZIP)));
  EWC::I::get().server().webServer().on("/pvzero/config/save", std::bind(&PVZeroClass::_onPVZeroSave, this, ws));
  EWC::I::get().server().webServer().on("/pvzero/state.json", std::bind(&PVZeroClass::_onPVZeroState, this, ws));
  EWC::I::get().server().webServer().on("/check", std::bind(&PVZeroClass::_onPVZeroCheck, this, ws));
  // EWC::I::get().server().webServer().on("/cycle1/pump", std::bind(&PVZeroClass::_onBbsPump1, this, ws));
  // EWC::I::get().server().webServer().on("/cycle2/pump", std::bind(&PVZeroClass::_onBbsPump2, this, ws));
  EWC::I::get().server().webServer().on("/js/shelly_3em_connector.js", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_APPLICATION_JS), JS_WEB_SHELLY_3EM_CONNECTOR_GZIP, sizeof(JS_WEB_SHELLY_3EM_CONNECTOR_GZIP)));
  // _ewcMqttHomie.setup(_ewcMqtt, I::get().config().paramDeviceName, "pvz-" + I::get().config().getChipId());
  // _ewcMqttHomie.addNode("pvz", "pvz");
  // _ewcMqttHomie.addProperty("pvz", "consumption-power", "consumption power", "integer", "", "W", false);
  _shelly3emConnector.setup(EWC::I::get().configFS().resetDetected());
  _taster.setup(EWC::I::get().configFS().resetDetected());
  // _mqttHelper.setup(_ewcMqtt);
  _tsMeasLoopStart = millis();
  _shelly3emConnector.setCallbackState(std::bind(&PVZeroClass::_onTotalWatt, this, std::placeholders::_1, std::placeholders::_2));
  EWC::I::get().logger() << F("Setup ok") << endl;

  //---------------------------------------------------------------------------------------------------
  // setup the control algorithm 
  //
  clCaP.init();
  clCaP.setFeedInTargetDcVoltage(24.0);
  clCaP.setFeedInTargetDcCurrentLimits(0.0, 1.6);

  //---------------------------------------------------------------------------------------------------
  // update the PSU
  //
  // aclPsuP[0].init(Serial1);
  aclPsuP[1].init(Serial2);
  aclPsuP[1].set(clCaP.feedInTargetDcVoltage(), clCaP.feedInTargetDcCurrent());
  aclPsuP[1].enable(true);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PVZeroClass::processControlAlgorithm(void)
{
  //---------------------------------------------------------------------------------------------------
  // collect and provide data to the control algorithm
  //
  if (_shelly3emConnector.isValid())
  {
    clCaP.updateConsumptionPower(_shelly3emConnector.consumptionPower());
  }
  else
  {
    //-------------------------------------------------------------------------------------------
    // \todo decide what to do, if the no consumption power is not available
    //
    // Set the power to 0 so that the current is also limited to 0.
    clCaP.updateConsumptionPower(0.0);
  }
  clCaP.updateFeedInActualDcValues(aclPsuP[1].actualVoltage(), aclPsuP[1].actualCurrent()); 

  //---------------------------------------------------------------------------------------------------
  // finally trigger the processing of data
  //
  clCaP.process();

}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void PVZeroClass::loop()
{
  String clStringT;
  PvzLcd::WifiConfig_ts tsWifiT;

  unsigned long ts_now = millis();
  _taster.loop();
  _ewcMail.loop();
  _ewcUpdater.loop();

  //---------------------------------------------------------------------------------------------------
  // Trigger 3EM loop and NTP time each second
  //
  if ((ts_now - _tsMeasLoopStart) > 1000)
  {
    _tsMeasLoopStart = ts_now;

    //-------------------------------------------------------------------------------------------
    // proceed only if WiFi connection is established
    //
    if (PZI::get().ewcServer().isConnected())
    {
      //-----------------------------------------------------------------------------------
      // update time if available
      //
      if (I::get().time().timeAvailable())
      {
        //---------------------------------------------------------------------------
        // update current time on LCD
        //
        _lcd.updateTime(I::get().time().str());
      }
      //---------------------------------------------------------------------------
      // time is not available, clear it on LCD
      //
      else
      {
        _lcd.updateTime("");
      }

      //-----------------------------------------------------------------------------------
      // trigger Shell 3EM loop
      //
      _shelly3emConnector.loop();
    }

    //------------------------------------------------------------------------------------------- 
    // Update target data of the PSU only one time in second
    //
    aclPsuP[1].set(clCaP.feedInTargetDcVoltage(), clCaP.feedInTargetDcCurrent());
  }

  //---------------------------------------------------------------------------------------------------
  // process PSUs, read of actual data is performed each 500 ms
  //
  aclPsuP[0].process();
  aclPsuP[1].process();

  //---------------------------------------------------------------------------------------------------
  // process algorithm
  //
  processControlAlgorithm();

  //---------------------------------------------------------------------------------------------------
  // Prepare informations for the LCD screens, when WiFi is connected
  //
  if (WiFi.isConnected() == true)
  {
    tsWifiT.clIp = WiFi.localIP();
    tsWifiT.clSsid = WiFi.SSID();

    _lcd.updateWifiInfo(&tsWifiT);
    _lcd.updateWifiRssi(WiFi.RSSI());

    if ((PZI::get().shelly3emConnector().isValid()))
    {
      atsLcdScreenG[0].aclLine[0] = String("Cons. power: " + String(PZI::get().shelly3emConnector().consumptionPower()) + "Wh");

      //----------------------------------------------------------------------------------- 
      // Print Infos from PSUs only if they are available
      //
      if (aclPsuP[0].isAvailable())
      {
        atsLcdScreenG[0].aclLine[1] = String("A[Wh]: " + String(clCaP.feedInTargetPower(), 0) + " | " + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(),0));
      }
      if (aclPsuP[1].isAvailable())
      {
        atsLcdScreenG[0].aclLine[2] = String("B[Wh]: " + String(clCaP.feedInTargetPower(), 0) + " | " + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(),0));
      }


      atsLcdScreenG[1].aclLine[0] = String("Feed-in target/actual");
      atsLcdScreenG[1].aclLine[1] = String("("+String(clCaP.feedInTargetPower(), 0) + ")" + String(clCaP.feedInTargetPowerApprox(), 0) + "Wh=" +
                                           String(clCaP.feedInTargetDcVoltage(), 0) + "V+" +
                                           String(clCaP.feedInTargetDcCurrent(), 1) + "A");

      atsLcdScreenG[1].aclLine[2] = String("" + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0) + " Wh = " +
                                           String(aclPsuP[1].actualVoltage(), 0) + "V + " +
                                           String(aclPsuP[1].actualCurrent(), 1) + "A");

      _lcd.setScreen(&atsLcdScreenG[0], 2);
      _lcd.ok();
    }
    //---------------------------------------------------------------------------
    // print warning with connection failure to the power meter
    //
    else
    {
      _lcd.warning("Request ERROR at 3EM", "Check URL:", PZI::get().config().shelly3emAddr);
    }
  }

  //---------------------------------------------------------------------------------------------------
  // Show failure info, when WiFi is not connected
  //
  else
  {
    _lcd.updateTime("");
    _lcd.updateWifiRssi(0);

    if (I::get().config().paramWifiDisabled)
    {
      _lcd.busy("Connect to AP:", "Name of AP");
    }
    else
    {
      switch (WiFi.status())
      {
      case WL_NO_SHIELD:
        clStringT = "NO_SHIELD";
        break;
      case WL_IDLE_STATUS:
        clStringT = "IDLE_STATUS";
        break;
      case WL_NO_SSID_AVAIL:
        clStringT = "NO_SSID_AVAIL";
        break;
      case WL_SCAN_COMPLETED:
        clStringT = "SCAN_COMPLETED";
        break;
      case WL_CONNECTED:
        clStringT = "CONNECTED";
        break;
      case WL_CONNECT_FAILED:
        clStringT = "CONNECT_FAILED";
        break;
      case WL_CONNECTION_LOST:
        clStringT = "CONNECTION_LOST";
        break;
      case WL_DISCONNECTED:
        clStringT = "DISCONNECTED";
        break;
      default:
        clStringT = "WL: " + String(WiFi.status());
        break;
      }

      _lcd.warning("WiFi connection lost:", String("WiFi status: " + String(WiFi.status())), clStringT);
    }
  }

  //---------------------------------------------------------------------------------------------------
  // Trigger the LCD application
  //
  _lcd.process();
}

void PVZeroClass::_onPVZeroConfig(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] config request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  _config.fillJson(jsonDoc);
  String output;
  serializeJson(jsonDoc, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void PVZeroClass::_onPVZeroSave(WebServer *webServer)
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
  if (webServer->hasArg("taster_func"))
  {
    long val = webServer->arg("taster_func").toInt();
    if (val >= 0)
    {
      config["pvzero"]["taster_func"] = val;
    }
  }
  if (webServer->hasArg("shelly3emAddr"))
  {
    String val = webServer->arg("shelly3emAddr");
    if (val.length() >= 0)
    {
      config["pvzero"]["shelly3emAddr"] = val;
    }
  }
  if (webServer->hasArg("voltage"))
  {
    long val = webServer->arg("voltage").toInt();
    if (val >= 0)
    {
      config["pvzero"]["voltage"] = val;
    }
  }
  if (webServer->hasArg("voltage"))
  {
    long val = webServer->arg("max_amperage").toInt();
    if (val >= 0)
    {
      config["pvzero"]["max_amperage"] = val;
    }
  }

  _config.fromJson(config);
  I::get().configFS().save();
  String details;
  serializeJsonPretty(config["pvzero"], details);
  I::get().server().sendPageSuccess(webServer, "PVZ Config save", "Save successful!", "/pvzero/setup", "<pre id=\"json\">" + details + "</pre>");
}

void PVZeroClass::_onPVZeroState(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] state request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  JsonObject json = jsonDoc.to<JsonObject>();
  json["name"] = I::get().server().brand();
  json["version"] = I::get().server().version();
  json["consumption_power"] = _shelly3emConnector.consumptionPower();
  json["feed_in_power"] = _shelly3emConnector.feedInPower();
  json["check_info"] = _shelly3emConnector.info();
  json["check_interval"] = PZI::get().config().checkInterval;
  json["next_check"] = _shelly3emConnector.infoSleepUntil();
  String output;
  serializeJson(json, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void PVZeroClass::_onPVZeroCheck(WebServer *webServer)
{
  I::get().logger() << F("[PVZ] check request") << endl;
  if (!I::get().server().isAuthenticated(webServer))
  {
    I::get().logger() << F("[PVZ] not sufficient authentication") << endl;
    return webServer->requestAuthentication();
  }
  // _shelly3emConnector.sleeper().wakeup();
  // I::get().server().sendRedirect(webServer, HOME_URI);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), "{\"success\": true}");
}

void PVZeroClass::_onTotalWatt(bool state, int totalWatt)
{
  // _mqttHelper.publishC1Pump();
  I::get().logger() << F("[PVZ] callback with state: ") << state << F(", Verbrauch: ") << totalWatt << endl;
}
