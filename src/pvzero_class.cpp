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
#include "generated/webPvzJS.h"
#include "pvzero_class.h"
#include "pvzero_interface.h"
#include <mutex>

using namespace EWC;
using namespace PVZ;

#if defined(ESP8266)
#define MY_SHELLY_PIN D6
#else
#define MY_SHELLY_PIN 6
#endif

PVZeroClass::PVZeroClass()
    : _shelly3emConnector(MY_SHELLY_PIN), // pinPot=D6
      uartP(clMpptP, aclPsuP[0], aclPsuP[1])
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

#ifndef BUILD_DEPLOY_RELEASE
#define BUILD_DEPLOY_RELEASE 1
#endif

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PVZeroClass::setup()
{
  EWC::I::get().configFS().addConfig(_ewcUpdater);
  EWC::I::get().configFS().addConfig(_ewcMail);
  EWC::I::get().configFS().addConfig(_ewcMqtt);
  EWC::I::get().configFS().addConfig(_config);
  EWC::I::get().configFS().addConfig(_shelly3emConnector);
  EWC::I::get().server().enableConfigUri();
  EWC::I::get().server().setup();
  EWC::I::get().config().paramLanguage = "de";
  WebServer *ws = &EWC::I::get().server().webServer();
  EWC::I::get().logger() << F("Setup WebServer") << endl;
  EWC::I::get().server().webServer().on(HOME_URI, std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_WEB_INDEX_GZIP, sizeof(HTML_WEB_INDEX_GZIP)));
  EWC::I::get().server().webServer().on("/languages.json", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), JSON_WEB_LANGUAGES_GZIP, sizeof(JSON_WEB_LANGUAGES_GZIP)));
  EWC::I::get().server().webServer().on("/pvz/state.json", std::bind(&PVZeroClass::_onPVZeroState, this, ws));
  EWC::I::get().server().webServer().on("/check", std::bind(&PVZeroClass::_onPVZeroCheck, this, ws));
  EWC::I::get().server().webServer().on("/js/pvz.js", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), ws, FPSTR(PROGMEM_CONFIG_APPLICATION_JS), JS_WEB_PVZ_GZIP, sizeof(JS_WEB_PVZ_GZIP)));

  _ewcMqttHA.setup(_ewcMqtt, "pvz." + I::get().config().getChipId(), I::get().config().paramDeviceName, "pvz");

  //---------------------------------------------------------------------------------------------------
  // Add properties for Home Assistant
  //
  _ewcMqttHA.addProperty("sensor", "consumption" + I::get().config().getChipId(), "3EM Consumption", "power", "measurement", "consumption", "W", false);
  _ewcMqttHA.addProperty("sensor", "feedIn" + I::get().config().getChipId(), "PVZ Feed-In", "power", "measurement", "feedIn", "W", false);
  _ewcMqttHA.addProperty("sensor", "totalConsumption" + I::get().config().getChipId(), "PVZ Consumption", "power", "measurement", "totalConsumption", "W", false);
  _ewcMqttHA.addProperty("sensor", "batteryCurrent" + I::get().config().getChipId(), "PVZ Battery Current", "current", "measurement", "batteryCurrent", "A", false);
  _ewcMqttHA.addProperty("sensor", "yieldEfficiencyToday" + I::get().config().getChipId(), "PVZ Yield Efficiency Today", "battery", "measurement", "yieldEfficiencyToday", "%", false);

  _ewcMqttHA.addProperty("sensor", "actualVoltagePsu1" + I::get().config().getChipId(), "PSU1 Voltage", "voltage", "measurement", "actualVoltagePsu1", "V", false);
  _ewcMqttHA.addProperty("sensor", "actualVoltagePsu2" + I::get().config().getChipId(), "PSU2 Voltage", "voltage", "measurement", "actualVoltagePsu2", "V", false);
  _ewcMqttHA.addProperty("sensor", "actualCurrentPsu1" + I::get().config().getChipId(), "PSU1 Current", "current", "measurement", "actualCurrentPsu1", "A", false);
  _ewcMqttHA.addProperty("sensor", "actualCurrentPsu2" + I::get().config().getChipId(), "PSU2 Current", "current", "measurement", "actualCurrentPsu2", "A", false);

  _ewcMqttHA.addProperty("sensor", "actualVoltageMppt" + I::get().config().getChipId(), "MPPT Battery Voltage", "voltage", "measurement", "actualVoltageMppt", "V", false);
  _ewcMqttHA.addProperty("sensor", "actualCurrentMppt" + I::get().config().getChipId(), "MPPT Battery Current", "current", "measurement", "actualCurrentMppt", "A", false);
  _ewcMqttHA.addProperty("sensor", "mpptPowerYieldToday" + I::get().config().getChipId(), "MPPT Power Yield Today", "energy", "total_increasing", "mpptPowerYieldToday", "kWh", false);
  _ewcMqttHA.addProperty("sensor", "mpptPanelVoltage" + I::get().config().getChipId(), "MPPT Panel Voltage", "voltage", "measurement", "mpptPanelVoltage", "V", false);
  _ewcMqttHA.addProperty("sensor", "mpptPanelPower" + I::get().config().getChipId(), "MPPT Panel Power", "power", "measurement", "mpptPanelPower", "W", false);

  _ewcMqttHA.addProperty("number", "batteryGuardState" + I::get().config().getChipId(), "PVZ Battery State", "", "", "batteryGuardState", "", false);
  _ewcMqttHA.addProperty("sensor", "pvzFeedInPowerToday" + I::get().config().getChipId(), "PVZ Feed-In Energy Today", "energy", "total_increasing", "PVZ_FeedIn_Today_Energy", "kWh", false);
  _ewcMqttHA.addProperty("sensor", "pvzExpectedYieldToday" + I::get().config().getChipId(), "PVZ Expected Power Yield Today", "energy", "total_increasing", "pvzExpectedYieldToday", "kWh", false);

  _ewcMqttHA.addProperty("number", "mpptOperatingState" + I::get().config().getChipId(), "MPPT Operating State", "", "", "mpptOperatingState", "", false);

  _tsMeasLoopStart = millis();
  _shelly3emConnector.setCallbackState(std::bind(&PVZeroClass::_onTotalWatt, this, std::placeholders::_1, std::placeholders::_2));

  _config.setCalibrationLowCallback(std::bind(&PVZeroClass::handleCalibrationLow, this, std::placeholders::_1));
  _config.setCalibrationHighCallback(std::bind(&PVZeroClass::handleCalibrationHigh, this, std::placeholders::_1));

#if (BUILD_DEPLOY_RELEASE > 0)
  //---------------------------------------------------------------------------------------------------
  // setup the control algorithm
  //
  clCaP.init();
  EWC::I::get().logger() << F("set maxVoltage: ") << PZI::get().config().getMaxVoltage() << ", maxCurrent: " << PZI::get().config().getMaxAmperage() << endl;
  clCaP.setFeedInTargetDcVoltage(PZI::get().config().getMaxVoltage());
  clCaP.setFeedInTargetDcCurrentLimits(0.0, PZI::get().config().getMaxAmperage());
  clCaP.setFilterOrder(_config.getFilterOrder());
  clCaP.setConsumptionPowerOffset(50.0); // Adjust the zero line

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
  // Read the last time from file system.
  //
  //
  uint64_t bgTime = I::get().configFS().readFrom(BATTERY_GUARD_FILE).toInt();
  EWC::I::get().logger() << F("Battery guard load time: ") << bgTime << endl;
  clBatGuardP.init(bgTime, std::bind(&PVZeroClass::batteryGuard_TimeStorageCallback, this, std::placeholders::_1));
  clBatGuardP.installEventHandler(std::bind(&PVZeroClass::batteryGuard_EventCallback, this, std::placeholders::_1));

  // \todo disable the guarding only while debug
  // clBatGuardP.enable(false);

  // prepare PSUs before uartP.setup() call
  aclPsuP[0].set(PZI::get().config().getMaxVoltage(), 0.0);
  aclPsuP[1].set(PZI::get().config().getMaxVoltage(), 0.0);

  EWC::I::get().logger() << F("setup() running on core ") << xPortGetCoreID() << endl;
  uartP.setup();
#endif

  ftFeedInPowerSumP = 0.0;
  ftFeedInPowerTodayP = 0.0;

  //---------------------------------------------------------------------------------------------------
  // initialisation of the LCD
  //
  enableLcd();
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PVZeroClass::processControlAlgorithm(void)
{
  float ftTargetVoltageT = 0.0; // is typically is equal to the MPPT voltage except the PSUs should be switched off
  int32_t slNumberOfStringsT;   // number of available strings, typically number of PSUs

  //---------------------------------------------------------------------------------------------------
  // update availability flags
  //
  abtPsuIsAvailableP[0] = aclPsuP[0].isAvailable();
  abtPsuIsAvailableP[1] = aclPsuP[1].isAvailable();

  //---------------------------------------------------------------------------------------------------
  // depending on availability of the PSUs update number of Strings and Current sum
  //
  slNumberOfStringsT = 0;
  if (abtPsuIsAvailableP[0] == true)
  {
    slNumberOfStringsT++;

    //-------------------------------------------------------------------------------------------
    // take values from PSU
    //
    aftActualVoltageOfPsuP[0] = aclPsuP[0].actualVoltage();
    aftActualCurrentOfPsuP[0] = aclPsuP[0].actualCurrent();
  }
  else
  {
    aftActualVoltageOfPsuP[0] = 0.0;
    aftActualCurrentOfPsuP[0] = 0.0;
  }

  if (abtPsuIsAvailableP[1] == true)
  {
    slNumberOfStringsT++;

    //-------------------------------------------------------------------------------------------
    // take values from PSU
    //
    aftActualVoltageOfPsuP[1] = aclPsuP[1].actualVoltage();
    aftActualCurrentOfPsuP[1] = aclPsuP[1].actualCurrent();
  }
  else
  {
    aftActualVoltageOfPsuP[1] = 0.0;
    aftActualCurrentOfPsuP[1] = 0.0;
  }

  //---------------------------------------------------------------------------------------------------
  // take values from MPPT
  //
  ftMpptBatteryCurrentP = clMpptP.batteryCurrent();
  ftMpptBatteryVoltageP = clMpptP.batteryVoltage();
  ftMpptPanelPowerP = clMpptP.panelPower();
  ftMpptPanelVoltageP = clMpptP.panelVoltage();
  ftMpptYieldTodayP = clMpptP.powerYieldToday();
  ubMpptStateOfOperationP = clMpptP.stateOfOperation();
  ftExpectedYieldTodayP = ftMpptYieldTodayP * (0.95 * 0.92 * 0.95); // real feed in [kWh] that is also measured by the inverter

  // update the MPPT state for the GUI
  switch (ubMpptStateOfOperationP)
  {
  case 0:
    strMpptState = "Off";
    break;
  case 2:
    strMpptState = "Fault";
    break;
  case 3:
    strMpptState = "Bulk - Ladephase mit maximalen Ladestrom";
    break;
  case 4:
    strMpptState = "Absorption - Nachladephase in der die Batterie­spannung konstant bleibt";
    break;
  case 5:
    strMpptState = "Float - Erhaltungsphase der Batterie und angeschlossene Endgeräte werden mit Strom versorgt";
    break;
  case 7:
    strMpptState = "Equalize (manual)";
    break;
  case 245:
    strMpptState = "Starting-up";
    break;
  case 247:
    strMpptState = "Auto equalize / Recondition";
    break;
  case 252:
    strMpptState = "External Control";
    break;
  default:
    strMpptState = "unknown";
    break;
  }

  //---------------------------------------------------------------------------------------------------
  // calculate the real feed in power and other values for display in GUI
  // The efficiency is not considered by that value.
  // There are three sources for which the efficiency must be taken into account
  // - inverter 95%
  // - PSU 92%
  // - Battery 95% (83% - 88%)
  // The efficiency will be tracked in ftEfficiency
  //
  ftFeedInPowerP = (((aftActualVoltageOfPsuP[0] * aftActualCurrentOfPsuP[0]) +
                     (aftActualVoltageOfPsuP[1] * aftActualCurrentOfPsuP[1])));

  ftTotalConsumptionP = ftFeedInPowerP + (float)consumptionPower;
  ftBatteryCurrentP = (ftMpptBatteryCurrentP - (aftActualCurrentOfPsuP[0] + aftActualCurrentOfPsuP[1]));

  //---------------------------------------------------------------------------------------------------
  // update values for curren in and current out in [Ah]
  //

  // update current values each second
  ftFeedInPowerSumP += ftFeedInPowerP;
  ftFeedInPowerTodayP = (ftFeedInPowerSumP / 3600); // value should be in [h]
  ftFeedInPowerTodayP /= 1000;                      // value should be in [kWh]

  //---------------------------------------------------------------------------------------------------
  // calculate the yield efficiency of the day
  //
  ftYieldEfficiencyTodayP = ftFeedInPowerTodayP;
  ftYieldEfficiencyTodayP *= 100.0;
  ftYieldEfficiencyTodayP /= ftMpptYieldTodayP;

  //---------------------------------------------------------------------------------------------------
  // just show all data we handle with
  //
  // I::get().logger() << F("Current Time: ") << I::get().time().currentTime() << endl;
  // I::get().logger() << F("loop() running on core ") << xPortGetCoreID() << "..." << endl;
  // I::get().logger() << F("MPPT Values: ") << String(ftMpptBatteryVoltageP, 3) << " V, " << String(ftMpptBatteryCurrentP, 3) << " A, state " << ubMpptStateOfOperationP << endl;
  // I::get().logger() << F("PSU0 actual values: ") << String(aftActualVoltageOfPsuP[0], 3) << " V, " << String(aftActualVoltageOfPsuP[0], 3) << " A, is available " << abtPsuIsAvailableP[0] << endl;
  // I::get().logger() << F("PSU0 target values: ") << String(aclPsuP[0].targetVoltage(), 3) << " V, " << String(aclPsuP[0].targetCurrent(), 3) << " A" << endl;
  // I::get().logger() << F("PSU1 actual values: ") << String(aftActualVoltageOfPsuP[1], 3) << " V, " << String(aftActualCurrentOfPsuP[1], 3) << " A, is available " << abtPsuIsAvailableP[1] << endl;
  // I::get().logger() << F("PSU1 target values: ") << String(aclPsuP[1].targetVoltage(), 3) << " V, " << String(aclPsuP[1].targetCurrent(), 3) << " A" << endl;

  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
  // collect and provide data to the control algorithm
  //

  //---------------------------------------------------------------------------------------------------
  // update actual consumption power given in [Wh]
  //
  if (isConsumptionPowerValid)
  {
    // take value from Shelly 3EM
    clCaP.updateConsumptionPower((float)consumptionPower);
  }
  else
  {
    //-------------------------------------------------------------------------------------------
    // \todo decide what to do, if the no consumption power is not available
    //       provide this value from GUI
    //
    clCaP.updateConsumptionPower(250.0);
  }

  //---------------------------------------------------------------------------------------------------
  // Set voltage to the voltage from MPPT that also corresponds to the voltage of battery
  // and update actual values, that correspond to the target values.
  // Set target actual values to the target values leads in to the better control results.
  //
  clCaP.setFeedInTargetDcVoltage(ftMpptBatteryVoltageP);
  clCaP.updateFeedInActualDcValues(ftMpptBatteryVoltageP, ftLimitedTargetCurrentP * slNumberOfStringsT);

  // I::get().logger() << F("Actual Values for CA: Consumption ") << String((float)consumptionPower, 3) << " W " << String((ftMpptBatteryVoltageP), 1) << " V, " << String(clBatGuardP.limitedCurrent(clCaP.feedInTargetDcCurrent()), 3) << " A" << endl;

  //---------------------------------------------------------------------------------------------------
  // finally trigger the processing of data
  //
  clCaP.process();

  // I::get().logger() << F("Target Current after CA: ") << String(clCaP.feedInTargetDcCurrent(), 3) << " A" << endl;

  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
  // update value for battery guard
  //
  clBatGuardP.updateVoltage(ftMpptBatteryVoltageP);
  clBatGuardP.updateCurrent(ftMpptBatteryCurrentP);
  clBatGuardP.updateMpptState(ubMpptStateOfOperationP);
  if (I::get().time().timeAvailable())
  {
    clBatGuardP.updateTime(I::get().time().currentTime());
  }
  clBatGuardP.process();

  //---------------------------------------------------------------------------------------------------
  // Adjust current value depending on available number of strings / PSUs
  //
  if (slNumberOfStringsT > 0)
  {
    //-------------------------------------------------------------------------------------------
    // Consider current limit and adjust it corresponding to the number of connected PSUs
    //
    ftLimitedTargetCurrentP = clBatGuardP.limitedCurrent(clCaP.feedInTargetDcCurrent());
    if (slNumberOfStringsT > 1)
    {
      ftLimitedTargetCurrentP /= 2.0;
    }

    //-------------------------------------------------------------------------------------------
    // make sure current do not exceeds the limit provided by user
    //
    if (ftLimitedTargetCurrentP > PZI::get().config().getMaxAmperage())
    {
      ftLimitedTargetCurrentP = PZI::get().config().getMaxAmperage();
    }
  }

  // I::get().logger() << F("Target Current after BG: ") << String(ftLimitedTargetCurrentP, 3) << " A, considering " << slNumberOfStringsT << " PSUs" << endl;

  //---------------------------------------------------------------------------------------------------
  // Adjust voltage value depending on feed in current.
  // If feed in current is near by 0.00 A, that means that PSUs can be switched off and the
  // target voltage can be set to 0.0 V.
  //
  if ((int32_t)(ftLimitedTargetCurrentP * 100) != 0)
  {
    ftTargetVoltageT = PZI::get().config().getMaxVoltage(); // clCaP.feedInTargetDcVoltage();
  }
  else
  {
    ftTargetVoltageT = 0.0;
  }

  //---------------------------------------------------------------------------------------------------
  // update PSU0 if available
  //
  if (abtPsuIsAvailableP[0])
  {
    aclPsuP[0].set(ftTargetVoltageT, ftLimitedTargetCurrentP);
  }
  else
  {
    aclPsuP[0].set(0.0, 0.0);
  }

  //---------------------------------------------------------------------------------------------------
  // update PSU1 if available
  //
  //
  if (abtPsuIsAvailableP[1])
  {
    aclPsuP[1].set(ftTargetVoltageT, ftLimitedTargetCurrentP);
  }
  else
  {
    aclPsuP[1].set(0.0, 0.0);
  }
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
  // Measure and calculate the charge voltage (input voltage PSUs)
  //
#if 0
  if (McOvsSample(&atsOvsInputsP[0], analogRead(33)) == true)
  {
    ftPsuVccT = (float)McOvsGetResult(&atsOvsInputsP[0]);
    ftPsuVccT *= ftPsuSupplyGainP;
    ftPsuVccT += ftPsuSupplyOffsetP;

    I::get().logger() << F("Measured Charge Voltage: ") << McOvsGetResult(&atsOvsInputsP[0]) << F(" V") << endl;
  }
#endif

  //-------------------------------------------------------------------------------------------
  // proceed only if WiFi connection is established
  //
  if (PZI::get().ewcServer().isConnected())
  {
    //-----------------------------------------------------------------------------------
    // trigger Shelly 3EM loop
    //
    _shelly3emConnector.loop();

    //-----------------------------------------------------------------------------------
    // trigger mqtt client loop to trigger reconnection
    //
    _ewcMqtt.loop();
    _ewcMqttHA.loop();
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
      if (_config.isEnabledLcd())
      {
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
      }
    }

#if (BUILD_DEPLOY_RELEASE > 0)
    processControlAlgorithm();
#endif

    //-------------------------------------------------------------------------------------------
    //
    //
    if (triggerMqttSend)
    {
      triggerMqttSend = false;
      if (isConsumptionPowerValid)
      {
        _ewcMqttHA.publishState("consumption" + I::get().config().getChipId(), String(consumptionPower));
      }
      _ewcMqttHA.publishState("feedIn" + I::get().config().getChipId(), String(ftFeedInPowerP, 0));
      _ewcMqttHA.publishState("totalConsumption" + I::get().config().getChipId(), String(ftTotalConsumptionP, 0));
      _ewcMqttHA.publishState("batteryCurrent" + I::get().config().getChipId(), String(ftBatteryCurrentP, 0));
      _ewcMqttHA.publishState("mpptPowerYieldToday" + I::get().config().getChipId(), String(ftMpptYieldTodayP, 0));
      _ewcMqttHA.publishState("actualVoltageMppt" + I::get().config().getChipId(), String(ftMpptBatteryVoltageP, 0));
      _ewcMqttHA.publishState("actualCurrentMppt" + I::get().config().getChipId(), String(ftMpptBatteryCurrentP, 0));
      _ewcMqttHA.publishState("actualVoltagePsu1" + I::get().config().getChipId(), String(aftActualVoltageOfPsuP[0], 0));
      _ewcMqttHA.publishState("actualVoltagePsu2" + I::get().config().getChipId(), String(aftActualVoltageOfPsuP[1], 0));
      _ewcMqttHA.publishState("actualCurrentPsu1" + I::get().config().getChipId(), String(aftActualCurrentOfPsuP[0], 0));
      _ewcMqttHA.publishState("actualCurrentPsu2" + I::get().config().getChipId(), String(aftActualCurrentOfPsuP[1], 0));
      _ewcMqttHA.publishState("batteryGuardState" + I::get().config().getChipId(), String(clBatGuardP.state()), 0);
      _ewcMqttHA.publishState("mpptOperatingState" + I::get().config().getChipId(), String(ubMpptStateOfOperationP), 0);
      _ewcMqttHA.publishState("pvzFeedInPowerToday" + I::get().config().getChipId(), String(ftFeedInPowerTodayP, 0));
      _ewcMqttHA.publishState("pvzExpectedYieldToday" + I::get().config().getChipId(), String(ftExpectedYieldTodayP, 0));
      _ewcMqttHA.publishState("yieldEfficiencyToday" + I::get().config().getChipId(), String(ftYieldEfficiencyTodayP, 0));

      _ewcMqttHA.publishState("mpptPanelVoltage" + I::get().config().getChipId(), String(ftMpptPanelVoltageP, 0));
      _ewcMqttHA.publishState("mpptPanelPower" + I::get().config().getChipId(), String(ftMpptPanelPowerP, 0));
    }
  }

  if (_config.isEnabledLcd())
  {
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
        if (abtPsuIsAvailableP[0])
        {
          atsLcdScreenP[0].aclLine[1] = String("A[Wh]: " + String(clCaP.feedInTargetPower(), 0) + " | " + String(aftActualVoltageOfPsuP[0] * aftActualCurrentOfPsuP[0], 0));
          slLcdScreenNrT++;

          atsLcdScreenP[slLcdScreenNrT].aclLine[0] = String("Feed-in via PSU A");
          atsLcdScreenP[slLcdScreenNrT].aclLine[1] = String("(" + String(clCaP.feedInTargetPower(), 0) + ")" + String(clCaP.feedInTargetPowerApprox(), 0) + "Wh=" +
                                                            String(clCaP.feedInTargetDcVoltage(), 0) + "V+" +
                                                            String(clCaP.feedInTargetDcCurrent(), 1) + "A");

          atsLcdScreenP[slLcdScreenNrT].aclLine[2] = String("" + String(aftActualVoltageOfPsuP[0] * aftActualCurrentOfPsuP[0], 0) + " Wh = " +
                                                            String(aftActualVoltageOfPsuP[0], 0) + "V + " +
                                                            String(aftActualCurrentOfPsuP[0], 1) + "A");
        }
        else
        {
          atsLcdScreenP[0].aclLine[1] = String(" - No PSU A connected");
        }

        //-----------------------------------------------------------------------------------
        // display info that depends on PSU A
        //
        if (abtPsuIsAvailableP[1])
        {
          atsLcdScreenP[0].aclLine[2] = String("B[Wh]: " + String(clCaP.feedInTargetPower(), 0) + " | " + String(aftActualVoltageOfPsuP[1] * aftActualCurrentOfPsuP[1], 0));

          slLcdScreenNrT++;

          atsLcdScreenP[slLcdScreenNrT].aclLine[0] = String("Feed-in via PSU B");
          atsLcdScreenP[slLcdScreenNrT].aclLine[1] = String("(" + String(clCaP.feedInTargetPower(), 0) + ")" + String(clCaP.feedInTargetPowerApprox(), 0) + "Wh=" +
                                                            String(clCaP.feedInTargetDcVoltage(), 0) + "V+" +
                                                            String(clCaP.feedInTargetDcCurrent(), 1) + "A");

          atsLcdScreenP[slLcdScreenNrT].aclLine[2] = String("" + String(aftActualVoltageOfPsuP[1] * aftActualCurrentOfPsuP[1], 0) + " Wh = " +
                                                            String(aftActualVoltageOfPsuP[1], 0) + "V + " +
                                                            String(aftActualCurrentOfPsuP[1], 1) + "A");
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
    else
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
    // Trigger the LCD application
    //
    _lcd.process();
  }
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
  json["feed_in_power"] = ftFeedInPowerP; // consider 95% efficiency of the inverter
  json["enable_second_psu"] = consumptionPower;
  json["battery_state"] = strBatteryState;
  json["mppt_state"] = strMpptState;
  json["total_consumption"] = ftTotalConsumptionP;
  json["battery_current"] = ftBatteryCurrentP;
  json["feedin_powert_today"] = ftFeedInPowerTodayP;
  json["feedin_powert_today_real"] = ftExpectedYieldTodayP;
  json["battery_state_info"] = strBatteryStateInfo;
  json["check_interval"] = PZI::get().config().getCheckInterval();
  json["next_check"] = _shelly3emConnector.infoSleepUntil();
  json["mppt_available"] = clMpptP.available();
  json["mppt_w"] = ftMpptBatteryVoltageP * ftMpptBatteryCurrentP;
  json["mppt_v"] = ftMpptBatteryVoltageP;
  json["mppt_a"] = ftMpptBatteryCurrentP;
  json["psu1_available"] = abtPsuIsAvailableP[0];
  json["psu1_w"] = (aftActualVoltageOfPsuP[0] * aftActualCurrentOfPsuP[0]);
  json["psu1_v"] = aftActualVoltageOfPsuP[0];
  json["psu1_a"] = aftActualCurrentOfPsuP[0];
  json["psu1_target_w"] = aclPsuP[0].targetVoltage() * aclPsuP[0].targetCurrent();
  json["psu1_target_v"] = aclPsuP[0].targetVoltage();
  json["psu1_target_a"] = aclPsuP[0].targetCurrent();
  json["psu2_available"] = abtPsuIsAvailableP[1];
  json["psu2_w"] = (aftActualVoltageOfPsuP[1] * aftActualCurrentOfPsuP[1]);
  json["psu2_v"] = aftActualVoltageOfPsuP[1];
  json["psu2_a"] = aftActualCurrentOfPsuP[1];
  json["psu2_target_w"] = aclPsuP[1].targetVoltage() * aclPsuP[1].targetCurrent();
  json["psu2_target_v"] = aclPsuP[1].targetVoltage();
  json["psu2_target_a"] = aclPsuP[1].targetCurrent();
  json["analog_available"] = PZI::get().config().isEnabledAnalog() && !isnan(ftPsuVccT);
  json["analog_v"] = isnan(ftPsuVccT) ? 0.0 : ftPsuVccT;
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

  consumptionPower = totalWatt;
  isConsumptionPowerValid = state;
  triggerMqttSend = true;
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

void PVZeroClass::batteryGuard_TimeStorageCallback(uint64_t uqTimeV)
{
  EWC::I::get().logger() << F("Battery guard save time: ") << uqTimeV << endl;
  I::get().configFS().saveTo(BATTERY_GUARD_FILE, String(uqTimeV));
}

void PVZeroClass::batteryGuard_EventCallback(BatteryGuard::State_te teStateV)
{

  I::get().logger() << F("MPPT Values: ") << String(ftMpptBatteryVoltageP, 3) << " V, " << String(ftMpptBatteryCurrentP, 3) << " A, state " << ubMpptStateOfOperationP << ", is available " << clMpptP.available() << endl;
  I::get().logger() << F("PSU0 actual values: ") << String(aftActualVoltageOfPsuP[0], 3) << " V, " << String(aftActualVoltageOfPsuP[0], 3) << " A, is available " << abtPsuIsAvailableP[0] << endl;
  I::get().logger() << F("PSU0 target values: ") << String(aclPsuP[0].targetVoltage(), 3) << " V, " << String(aclPsuP[0].targetCurrent(), 3) << " A" << endl;
  I::get().logger() << F("PSU1 actual values: ") << String(aftActualVoltageOfPsuP[1], 3) << " V, " << String(aftActualCurrentOfPsuP[1], 3) << " A, is available " << abtPsuIsAvailableP[1] << endl;
  I::get().logger() << F("PSU1 target values: ") << String(aclPsuP[1].targetVoltage(), 3) << " V, " << String(aclPsuP[1].targetCurrent(), 3) << " A" << endl;

  switch (teStateV)
  {
  case BatteryGuard::State_te::eCharge:
    strBatteryState = String("charge");
    // reset power counter in case the battery was discharged before
    if (teBatteryGuardStatePreviousP == BatteryGuard::State_te::eDischarged)
    {
      ftFeedInPowerSumP = 0.0;
    }
    break;
  case BatteryGuard::State_te::eChargeAndDischarge:
    strBatteryState = String("charge and discharge");
    break;
  case BatteryGuard::State_te::eChargeUntilCharged:
    strBatteryState = String("charge until charged");
    break;
  case BatteryGuard::State_te::eCharged:
    strBatteryState = String("charged");
    break;
  case BatteryGuard::State_te::eDischarge:
    strBatteryState = String("discharge");
    break;
  case BatteryGuard::State_te::eDischarged:
    strBatteryState = String("discharged");
    break;
  default:
    strBatteryState = "-";
  }
  teBatteryGuardStatePreviousP = teStateV;
  strBatteryStateInfo = clBatGuardP.stateInfo(teStateV);
  EWC::I::get().logger() << F("Battery status: ") << strBatteryState << endl;
  EWC::I::get().logger() << F("Battery info: ") << strBatteryStateInfo << endl;
}
