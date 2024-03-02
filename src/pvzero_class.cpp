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
#include "generated/webIndexHTML.h"
#include "generated/webLanguagesJSON.h"
#include "generated/webShelly_3em_connectorJS.h"
#include "pvzero_class.h"
#include "pvzero_interface.h"
#include "sw_ovs.h"

using namespace EWC;
using namespace PVZ;

#if defined(ESP8266)
#define MY_SHELLY_PIN D6
#else
#define MY_SHELLY_PIN 6
#endif

PvzLcd::Screen_ts atsLcdScreenG[5]; // make screen of LCD available for whole application
McOvs_ts atsOvsInputsG[4];          // prepare software oversampling for up to 4 values
float ftPsuSupplyGainG;
float ftPsuSupplyOffsetG;
int32_t aftPsuSupplyCalRawG[2];     // index 0 contains Low value and 1 high value 
float   aftPsuSupplyCalNominalG[2]; // index 0 contains Low value and 1 high value 

PVZeroClass::PVZeroClass()
    : _shelly3emConnector(MY_SHELLY_PIN) // pinPot=D6
{
  PZI::get()._pvz = this;
  PZI::get()._config = &_config;
  PZI::get()._ewcServer = &_ewcServer;
  PZI::get()._ewcMail = &_ewcMail;
  PZI::get()._ewcMqttHA = &_ewcMqttHA;
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
  EWC::I::get().configFS().addConfig(_shelly3emConnector);
  // EWC::I::get().led().enable(true, LED_BUILTIN, LOW);
  EWC::I::get().server().enableConfigUri();
  EWC::I::get().server().setup();
  EWC::I::get().config().paramLanguage = "de";
  WebServer *ws = &EWC::I::get().server().webServer();
  EWC::I::get().logger() << F("Setup WebServer") << endl;
  EWC::I::get().server().webServer().on(HOME_URI, std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_WEB_INDEX_GZIP, sizeof(HTML_WEB_INDEX_GZIP)));
  EWC::I::get().server().webServer().on("/languages.json", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), JSON_WEB_LANGUAGES_GZIP, sizeof(JSON_WEB_LANGUAGES_GZIP)));
  EWC::I::get().server().webServer().on("/pvzero/state.json", std::bind(&PVZeroClass::_onPVZeroState, this, ws));
  EWC::I::get().server().webServer().on("/check", std::bind(&PVZeroClass::_onPVZeroCheck, this, ws));
  EWC::I::get().server().webServer().on("/js/shelly_3em_connector.js", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_APPLICATION_JS), JS_WEB_SHELLY_3EM_CONNECTOR_GZIP, sizeof(JS_WEB_SHELLY_3EM_CONNECTOR_GZIP)));
  _ewcMqttHA.setup(_ewcMqtt, "pvz." + I::get().config().getChipId(), I::get().config().paramDeviceName, "pvz");
  _ewcMqttHA.addProperty("sensor", "consumption" + I::get().config().getChipId(), "Consumption", "power", "consumption", "W", false);
  _ewcMqttHA.addProperty("sensor", "feedIn" + I::get().config().getChipId(), "Feed-In", "power", "feedIn", "W", false);
  _tsMeasLoopStart = millis();
  _shelly3emConnector.setCallbackState(std::bind(&PVZeroClass::_onTotalWatt, this, std::placeholders::_1, std::placeholders::_2));

  _config.setCalibrationLowCallback(std::bind(&PVZeroClass::handleCalibrationLow, this, std::placeholders::_1));
  _config.setCalibrationHighCallback(std::bind(&PVZeroClass::handleCalibrationHigh, this, std::placeholders::_1));

  //---------------------------------------------------------------------------------------------------
  // setup the control algorithm
  //
  clCaP.init();
  EWC::I::get().logger() << F("set maxVoltage: ") << PZI::get().config().getMaxVoltage() << ", maxCurrent: " << PZI::get().config().getMaxAmperage() << endl;
  clCaP.setFeedInTargetDcVoltage(PZI::get().config().getMaxVoltage());
  clCaP.setFeedInTargetDcCurrentLimits(0.0, PZI::get().config().getMaxAmperage());
  clCaP.setFilterOrder(_config.getFilterOrder());

  //---------------------------------------------------------------------------------------------------
  // update the PSU
  //
  if (_config.isEnabledSecondPsu())
  {
    aclPsuP[0].init(Serial1);
  }
  aclPsuP[1].init(Serial2);

  int32_t slPsuNrT = 2;
  while (slPsuNrT > 0)
  {
    slPsuNrT--;
    if (aclPsuP[slPsuNrT].isAvailable())
    {
      aclPsuP[slPsuNrT].set(clCaP.feedInTargetDcVoltage(), clCaP.feedInTargetDcCurrent());
      aclPsuP[slPsuNrT].enable(true);
    } else
    {
      EWC::I::get().logger() << F("No PSU has been found via Serial") << slPsuNrT+1 << endl;
    }
  }
  EWC::I::get().logger() << F("Setup ok") << endl;

  //---------------------------------------------------------------------------------------------------
  // prepare software oversampling
  //
  McOvsInit(&atsOvsInputsG[0], 4); // increase ADC value to 14 bit

  //---------------------------------------------------------------------------------------------------
  // calculate the gain and offset based on on imperially based values
  // 10368 <=> 12.1
  // 23535 <=> 24.3
  // y=m*x+b
  //
  aftPsuSupplyCalRawG[0] = 2895;     // index 0 contains Low value and 1 high value 
  aftPsuSupplyCalRawG[1] = 23546;     
  aftPsuSupplyCalNominalG[0] = 5.0; // index 0 contains Low value and 1 high value 
  aftPsuSupplyCalNominalG[1] = 24.3; 
  
  // 
  updatePsuVccScaling();

  I::get().logger() << F("init gain: ") << String(ftPsuSupplyGainG,5) << " offset" << String(ftPsuSupplyOffsetG, 2) << endl;
  clBatGuardP.init();
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
  if (isConsumptionPowerValid)
  {
    clCaP.updateConsumptionPower(consumptionPower);
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
  int32_t slLcdScreenNrT;

  unsigned long ts_now = millis();
  _ewcMail.loop();
  _ewcUpdater.loop();

  //---------------------------------------------------------------------------------------------------
  // Calculate PSU Vcc
  //
  if (McOvsSample(&atsOvsInputsG[0], analogRead(34)) == true)
  {
    ftPsuVccT = (float)McOvsGetResult(&atsOvsInputsG[0]);
    ftPsuVccT *= ftPsuSupplyGainG;
    ftPsuVccT += ftPsuSupplyOffsetG;

    I::get().logger() << F("PSU Vcc: : ") << ftPsuVccT << F(" V") << endl;
  }

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
    aclPsuP[0].set(clCaP.feedInTargetDcVoltage(), clCaP.feedInTargetDcCurrent());
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
  if ((aclPsuP[0].isAvailable() == true) || (aclPsuP[1].isAvailable() == true))
  {
    processControlAlgorithm();
  }

  //---------------------------------------------------------------------------------------------------
  // Prepare informations for the LCD screens, when WiFi is connected
  //
  if (WiFi.isConnected() == true)
  {
    tsWifiT.clIp = WiFi.localIP();
    tsWifiT.clSsid = WiFi.SSID();

    _lcd.updateWifiInfo(&tsWifiT);
    _lcd.updateWifiRssi(WiFi.RSSI());

    if (isConsumptionPowerValid)
    {
      atsLcdScreenG[0].aclLine[0] = String("Cons. power: " + String(consumptionPower) + "Wh");

      //-----------------------------------------------------------------------------------
      // Print Infos from PSUs only if they are available
      //
      slLcdScreenNrT = 0;

      //-----------------------------------------------------------------------------------
      // display info that depends on PSU A
      //
      if (aclPsuP[0].isAvailable())
      {
        atsLcdScreenG[0].aclLine[1] = String("A[Wh]: " + String(clCaP.feedInTargetPower(), 0) + " | " + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0));
        slLcdScreenNrT++;

        atsLcdScreenG[slLcdScreenNrT].aclLine[0] = String("Feed-in via PSU A");
        atsLcdScreenG[slLcdScreenNrT].aclLine[1] = String("(" + String(clCaP.feedInTargetPower(), 0) + ")" + String(clCaP.feedInTargetPowerApprox(), 0) + "Wh=" +
                                                          String(clCaP.feedInTargetDcVoltage(), 0) + "V+" +
                                                          String(clCaP.feedInTargetDcCurrent(), 1) + "A");

        atsLcdScreenG[slLcdScreenNrT].aclLine[2] = String("" + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0) + " Wh = " +
                                                          String(aclPsuP[1].actualVoltage(), 0) + "V + " +
                                                          String(aclPsuP[1].actualCurrent(), 1) + "A");
      }
      else
      {
        atsLcdScreenG[0].aclLine[1] = String(" - No PSU A connected");
      }

      //-----------------------------------------------------------------------------------
      // display info that depends on PSU A
      //
      if (aclPsuP[1].isAvailable())
      {
        atsLcdScreenG[0].aclLine[2] = String("B[Wh]: " + String(clCaP.feedInTargetPower(), 0) + " | " + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0));

        slLcdScreenNrT++;

        atsLcdScreenG[slLcdScreenNrT].aclLine[0] = String("Feed-in via PSU B");
        atsLcdScreenG[slLcdScreenNrT].aclLine[1] = String("(" + String(clCaP.feedInTargetPower(), 0) + ")" + String(clCaP.feedInTargetPowerApprox(), 0) + "Wh=" +
                                                          String(clCaP.feedInTargetDcVoltage(), 0) + "V+" +
                                                          String(clCaP.feedInTargetDcCurrent(), 1) + "A");

        atsLcdScreenG[slLcdScreenNrT].aclLine[2] = String("" + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0) + " Wh = " +
                                                          String(aclPsuP[1].actualVoltage(), 0) + "V + " +
                                                          String(aclPsuP[1].actualCurrent(), 1) + "A");
      }
      else
      {
        atsLcdScreenG[0].aclLine[2] = String(" - No PSU B connected");
      }

      _lcd.setScreen(&atsLcdScreenG[0], slLcdScreenNrT + 1);
      _lcd.ok();
    }
    //---------------------------------------------------------------------------
    // print warning with connection failure to the power meter
    //
    else
    {
      _lcd.warning("Request ERROR at 3EM", "Check URL:", PZI::get().config().getShelly3emAddr());
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
  json["name"] = I::get().config().paramDeviceName;
  json["version"] = I::get().server().version();
  json["consumption_power"] = consumptionPower;
  json["feed_in_power"] = String(clCaP.feedInTargetPower(), 0);
  json["psu_vcc"] = String(ftPsuVccT, 2);
  json["enable_second_psu"] = consumptionPower;
  json["check_info"] = _shelly3emConnector.info();
  json["check_interval"] = PZI::get().config().getCheckInterval();
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

void PVZeroClass::_onTotalWatt(bool state, int32_t totalWatt)
{
  I::get().logger() << F("[PVZ] callback with state: ") << state << F(", Verbrauch: ") << totalWatt << endl;
  if (isConsumptionPowerValid)
  {
    _ewcMqttHA.publishState("consumption" + I::get().config().getChipId(), String(consumptionPower));
    _ewcMqttHA.publishState("feedIn" + I::get().config().getChipId(), String(clCaP.feedInTargetPower(), 0));
  }
  consumptionPower = totalWatt;
  isConsumptionPowerValid = state;
}

float PVZeroClass::handleCalibrationLow(float value)
{
  I::get().logger() << F("[PVZ] callback calibration low for ") << String(value, 2) << " <=> " << McOvsGetResult(&atsOvsInputsG[0]) << endl;

  //--------------------------------------------------------------------------------------------------- 
  // update calibration parameter and calculate new gain and offset
  //  
  aftPsuSupplyCalRawG[0] = McOvsGetResult(&atsOvsInputsG[0]);
  aftPsuSupplyCalNominalG[0] = value; 

  I::get().logger() << F("[PVZ] callback calibration gain: ") << String(ftPsuSupplyGainG,5) << " offset: " << String(ftPsuSupplyOffsetG, 2) << endl;

  // \todo store content of aftPsuSupplyCalRawG[0] and aftPsuSupplyCalNominalG [0]

  return 0.5;
}


float PVZeroClass::handleCalibrationHigh(float value)
{
  I::get().logger() << F("[PVZ] callback calibration high for ") << String(value, 2) << " <=> " << McOvsGetResult(&atsOvsInputsG[0]) << endl;


  //--------------------------------------------------------------------------------------------------- 
  // update calibration parameter and calculate new gain and offset
  //  
  aftPsuSupplyCalRawG[1] = McOvsGetResult(&atsOvsInputsG[0]);
  aftPsuSupplyCalNominalG[1] = value; 
  updatePsuVccScaling();

  I::get().logger() << F("[PVZ] callback calibration gain: ") << String(ftPsuSupplyGainG,5) << " offset" << String(ftPsuSupplyOffsetG, 2) << endl;

  // \todo store content of aftPsuSupplyCalRawG[1] and aftPsuSupplyCalNominalG [1]

  return 1.5;
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void PVZeroClass::updatePsuVccScaling()
{
  ftPsuSupplyGainG = ((aftPsuSupplyCalNominalG[1] - aftPsuSupplyCalNominalG[0]) / (aftPsuSupplyCalRawG[1] - aftPsuSupplyCalRawG[0]));
  ftPsuSupplyOffsetG = (aftPsuSupplyCalNominalG[0] - (aftPsuSupplyCalRawG[0] * ftPsuSupplyGainG));
}