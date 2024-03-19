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

using namespace EWC;
using namespace PVZ;

#if defined(ESP8266)
#define MY_SHELLY_PIN D6
#else
#define MY_SHELLY_PIN 6
#endif

// void mpptCallback(uint16_t id, int32_t value);
// VEDirect mppt(Serial2, mpptCallback);

// uint16_t panelVoltage = 0;
// uint16_t chargeCurrent = 0;

// void setup() {
//   Serial.begin(19200);
//   mppt.begin();
// }

// void loop() {
//   static unsigned long secondsTimer = 0;
//   mppt.update();
//   unsigned long m = millis();
//   if(m - secondsTimer > 1000L){
//     secondsTimer = m;
//     mppt.ping();  // send oing every second
//   }
// }
// void mpptCallback(uint16_t id, int32_t value)
// {
//   if (id == VEDirect_kPanelVoltage)
//   {
//     panelVoltage = value;
//     Serial.print(F("Vpv : "));
//     Serial.println(value * 0.01);
//   }
//   if (id == VEDirect_kChargeCurrent)
//   {
//     chargeCurrent = value;
//     Serial.print(F("Ich : "));
//     Serial.println(value * 0.1);
//   }
// }

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

void BatteryGuard_TimeStorageCallback(uint64_t uqTimeV)
{
}

void BatteryGuard_EventCallback(BatteryGuard::State_te teSStateV)
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
  // setup multiplexer
  //
  // pinMode(5, OUTPUT);     // INH
  // pinMode(18, OUTPUT);    // B
  // pinMode(19, OUTPUT);    // A
  // digitalWrite(5, LOW);   // INH
  // digitalWrite(18, LOW);  // B
  // digitalWrite(19, HIGH); // A

  // pinMode(16, OUTPUT); // RX
  // pinMode(17, OUTPUT); // TX

  // digitalWrite(16, LOW);
  // digitalWrite(17, LOW);

  // Serial2.begin(19200);
  // Serial2.println("Starting setup...");
  // Serial2.flush();

  //---------------------------------------------------------------------------------------------------
  // initialize the PSU
  //
  uint8_t ubStringCountT = 1;

  // digitalWrite(5, LOW);
  // digitalWrite(18, LOW);
  // digitalWrite(19, LOW);
  // aclPsuP[1].init(Serial2);
  // if (_config.isEnabledSecondPsu())
  // {
  //   aclPsuP[0].init(Serial);
  //   ubStringCountT = 2;
  // }

  // mppt.begin();

  //---------------------------------------------------------------------------------------------------
  // setup the control algorithm
  //
  clCaP.init(ubStringCountT);
  EWC::I::get().logger() << F("set maxVoltage: ") << PZI::get().config().getMaxVoltage() << ", maxCurrent: " << PZI::get().config().getMaxAmperage() << endl;
  clCaP.setFeedInTargetDcVoltage(PZI::get().config().getMaxVoltage());
  clCaP.setFeedInTargetDcCurrentLimits(0.0, PZI::get().config().getMaxAmperage());
  clCaP.setFilterOrder(_config.getFilterOrder());

  int32_t slPsuCountT = 2;
  while (slPsuCountT > 0)
  {
    slPsuCountT--;
    if (aclPsuP[slPsuCountT].isAvailable())
    {
      aclPsuP[slPsuCountT].set(clCaP.feedInTargetDcVoltage(), clCaP.feedInTargetDcCurrent());
      aclPsuP[slPsuCountT].enable(true);
    }
    else
    {
      EWC::I::get().logger() << F("No PSU has been found via Serial") << slPsuCountT + 1 << endl;
    }
  }
  EWC::I::get().logger() << F("Setup ok") << endl;

  //---------------------------------------------------------------------------------------------------
  // prepare software oversampling
  //
  McOvsInit(&atsOvsInputsP[0], 4); // increase ADC value to 14 bit

  //---------------------------------------------------------------------------------------------------
  // calculate the gain and offset based on on imperially based values
  //
  updatePsuVccScaling(0);

  //---------------------------------------------------------------------------------------------------
  // initialise the battery guard to avoid discharging of the battery
  //
  // \todo provide the recover voltage and minimal recover time from web gui
  //
  // add here demo value so, the application runs
  //
  // clBatGuardP.init(44.0, BatteryGuard_TimeStorageCallback);
  // clBatGuardP.installEventHandler(BatteryGuard_EventCallback);

  // \todo disable the guarding only while debug
  // clBatGuardP.enable(false);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PVZeroClass::processControlAlgorithm(void)
{
  float ftActualVoltageT;
  float ftActualCurrentT;
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

  //---------------------------------------------------------------------------------------------------
  // \todo consider feed in of both PSUs
  //
  ftActualVoltageT = aclPsuP[1].actualVoltage();
  ftActualCurrentT = (aclPsuP[0].actualCurrent() + aclPsuP[1].actualCurrent());

  clCaP.updateFeedInActualDcValues(ftActualVoltageT, ftActualCurrentT);

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
  if (McOvsSample(&atsOvsInputsP[0], analogRead(33)) == true)
  {
    ftPsuVccT = (float)McOvsGetResult(&atsOvsInputsP[0]);
    ftPsuVccT *= ftPsuSupplyGainP;
    ftPsuVccT += ftPsuSupplyOffsetP;

    I::get().logger() << F("PSU Vcc: : ") << McOvsGetResult(&atsOvsInputsP[0]) << F(" V") << endl;

    // digitalWrite(5, LOW);
    // digitalWrite(18, LOW);
    // digitalWrite(19, HIGH);
    // Serial2.println("Hello... IF 2");
    // Serial2.flush();

    // digitalWrite(5, LOW);
    // digitalWrite(18, HIGH);
    // digitalWrite(19, LOW);
    // Serial2.println("Hello... IF 3");
    // Serial2.flush();
    //-------------------------------------------------------------------------------------------
    // update value for battery guard
    //
    // clBatGuardP.updateVoltage(ftPsuVccT);
  }

  //---------------------------------------------------------------------------------------------------
  // process battery guard
  //
  // clBatGuardP.process();

  //---------------------------------------------------------------------------------------------------
  // Trigger 3EM loop and NTP time each second
  //
  if ((ts_now - _tsMeasLoopStart) > 1000)
  {
    _tsMeasLoopStart = ts_now;

    I::get().logger() << F("Time: ") << I::get().time().str() << endl;
    I::get().logger() << F("Current Time: ") << I::get().time().currentTime() << endl;

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

    // digitalWrite(5, LOW);
    // digitalWrite(18, LOW);
    // digitalWrite(19, LOW);
    //-------------------------------------------------------------------------------------------
    // Update target data of the PSU only one time in second
    //
    if (clBatGuardP.alarm() == false)
    {
      // aclPsuP[0].set(clCaP.feedInTargetDcVoltage(), clBatGuardP.limitedCurrent(clCaP.feedInTargetDcCurrent()));
      aclPsuP[1].set(clCaP.feedInTargetDcVoltage(), clBatGuardP.limitedCurrent(clCaP.feedInTargetDcCurrent()));
    }
    else
    {
      // aclPsuP[0].enable(false);
      aclPsuP[1].enable(false);
    }
  }

  //---------------------------------------------------------------------------------------------------
  // process PSUs, read of actual data is performed each 500 ms
  //
  aclPsuP[0].process();
  // digitalWrite(5, LOW);
  // digitalWrite(18, LOW);
  // digitalWrite(19, LOW);
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
  if ((WiFi.isConnected() == true)) // && (clBatGuardP.alarm() == false))
  {
    tsWifiT.clIp = WiFi.localIP();
    tsWifiT.clSsid = WiFi.SSID();

    _lcd.updateWifiInfo(&tsWifiT);
    _lcd.updateWifiRssi(WiFi.RSSI());

    if (isConsumptionPowerValid)
    {
      atsLcdScreenP[0].aclLine[0] = String("Cons. power: " + String(consumptionPower) + "Wh");

      //-----------------------------------------------------------------------------------
      // Print Infos from PSUs only if they are available
      //
      slLcdScreenNrT = 0;

      //-----------------------------------------------------------------------------------
      // display info that depends on PSU A
      //
      if (aclPsuP[0].isAvailable())
      {
        atsLcdScreenP[0].aclLine[1] = String("A[Wh]: " + String(clCaP.feedInTargetPower(), 0) + " | " + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0));
        slLcdScreenNrT++;

        atsLcdScreenP[slLcdScreenNrT].aclLine[0] = String("Feed-in via PSU A");
        atsLcdScreenP[slLcdScreenNrT].aclLine[1] = String("(" + String(clCaP.feedInTargetPower(), 0) + ")" + String(clCaP.feedInTargetPowerApprox(), 0) + "Wh=" +
                                                          String(clCaP.feedInTargetDcVoltage(), 0) + "V+" +
                                                          String(clCaP.feedInTargetDcCurrent(), 1) + "A");

        atsLcdScreenP[slLcdScreenNrT].aclLine[2] = String("" + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0) + " Wh = " +
                                                          String(aclPsuP[1].actualVoltage(), 0) + "V + " +
                                                          String(aclPsuP[1].actualCurrent(), 1) + "A");
      }
      else
      {
        atsLcdScreenP[0].aclLine[1] = String(" - No PSU A connected");
      }

      //-----------------------------------------------------------------------------------
      // display info that depends on PSU A
      //
      if (aclPsuP[1].isAvailable())
      {
        atsLcdScreenP[0].aclLine[2] = String("B[Wh]: " + String(clCaP.feedInTargetPower(), 0) + " | " + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0));

        slLcdScreenNrT++;

        atsLcdScreenP[slLcdScreenNrT].aclLine[0] = String("Feed-in via PSU B");
        atsLcdScreenP[slLcdScreenNrT].aclLine[1] = String("(" + String(clCaP.feedInTargetPower(), 0) + ")" + String(clCaP.feedInTargetPowerApprox(), 0) + "Wh=" +
                                                          String(clCaP.feedInTargetDcVoltage(), 0) + "V+" +
                                                          String(clCaP.feedInTargetDcCurrent(), 1) + "A");

        atsLcdScreenP[slLcdScreenNrT].aclLine[2] = String("" + String(aclPsuP[1].actualVoltage() * aclPsuP[1].actualCurrent(), 0) + " Wh = " +
                                                          String(aclPsuP[1].actualVoltage(), 0) + "V + " +
                                                          String(aclPsuP[1].actualCurrent(), 1) + "A");
      }
      else
      {
        atsLcdScreenP[0].aclLine[2] = String(" - No PSU B connected");
      }

      _lcd.setScreen(&atsLcdScreenP[0], slLcdScreenNrT + 1);
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
  else if ((clBatGuardP.alarm() == false))
  {
    _lcd.updateTime("");
    _lcd.updateWifiRssi(0);

    if (I::get().server().isAP())
    {
      _lcd.busy(String("Connect to AP:").c_str(), I::get().server().config().paramAPName.c_str());
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
  // handle the case the battery has been discharged
  //
  // else if (clBatGuardP.alarm() == true)
  // {
  //   tsWifiT.clIp = WiFi.localIP();
  //   tsWifiT.clSsid = WiFi.SSID();

  //   _lcd.updateWifiInfo(&tsWifiT);
  //   _lcd.updateWifiRssi(WiFi.RSSI());

  //   _lcd.warning("Bat. discharge alarm!",
  //                String("Vbat: " + String(ftPsuVccT, 1) + " / " + String(clBatGuardP.alarmRecoverVoltage(), 1)),
  //                String("Time: " + String(clBatGuardP.alarmPendingTime()) + " / " + String(clBatGuardP.alarmRecoverTime())));
  // }

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
  //---------------------------------------------------------------------------------------------------
  // update calibration parameter (RAW low) and calculate new gain and offset
  //
  updatePsuVccScaling(1);

  //---------------------------------------------------------------------------------------------------
  // Return RAW value, so it will be stored in config class
  //
  return (float)McOvsGetResult(&atsOvsInputsP[0]);
}

float PVZeroClass::handleCalibrationHigh(float value)
{
  //---------------------------------------------------------------------------------------------------
  // update calibration parameter (RAW high) and calculate new gain and offset
  //
  updatePsuVccScaling(2);

  //---------------------------------------------------------------------------------------------------
  // Return RAW value, so it will be stored in config class
  //
  return (float)McOvsGetResult(&atsOvsInputsP[0]);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PVZeroClass::updatePsuVccScaling(uint8_t ubSetupV)
{
  float ftRawLowT;
  float ftRawHighT;

  //---------------------------------------------------------------------------------------------------
  // use different values, for RAW depending on calibration or configuration
  //
  if (ubSetupV == 1) // take low RAW value from ADC and high from configuration
  {
    ftRawLowT = (float)McOvsGetResult(&atsOvsInputsP[0]);
    ftRawHighT = _config.getCalibrationRawHigh();
  }
  else if (ubSetupV == 2) // take high RAW value from ADC and low from configuration
  {
    ftRawLowT = _config.getCalibrationRawLow();
    ftRawHighT = (float)McOvsGetResult(&atsOvsInputsP[0]);
  }
  else // take booth values from configuration
  {
    ftRawLowT = _config.getCalibrationRawLow();
    ftRawHighT = _config.getCalibrationRawHigh();
  }

  //---------------------------------------------------------------------------------------------------
  // perform calculation of gain and offset based on y=m*x+b
  //
  ftPsuSupplyGainP = ((_config.getCalibrationNominalHigh() - _config.getCalibrationNominalLow()) / (ftRawHighT - ftRawLowT));
  ftPsuSupplyOffsetP = (_config.getCalibrationNominalLow() - (ftRawLowT * ftPsuSupplyGainP));
}