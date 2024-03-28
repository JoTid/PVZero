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
#include <mutex>

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
//   Serial.print(F("------------------------------- > mpptCallback id: "));
//   Serial.println(value);

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

// unsigned long time_interval;
// unsigned long time_interval_stamp;
// unsigned long ts_after = millis();

// Serial.printf("Time reinit the USART: %i ", ts_after - ts_before);

// void onReceiveFunction()
// {
//   time_interval = millis() - time_interval_stamp;
//   time_interval_stamp = millis();

//   size_t available = Serial2.available();
//   while (available--)
//   {
//     Serial.print((char)Serial2.read());
//   }
//   Serial.println();

//   Serial.printf("onReceive Callback:: Interval %i: ", time_interval);
// }

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
  _ewcMqttHA.addProperty("sensor", "totalConsumption" + I::get().config().getChipId(), "Total Consumption", "power", "totalConsumption", "W", false);
  _ewcMqttHA.addProperty("sensor", "batteryCurrent" + I::get().config().getChipId(), "Battery Current", "current", "batteryCurrent", "A", false);

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

  EWC::I::get().logger() << F("setup() running on core ") << xPortGetCoreID() << endl;
  uartP.setup();

  ftBatteryCurrentSumInSecP = 0.0;
  ftBatteryCurrentSumOutSecP = 0.0;
  ftBatteryCurrentSumInP = 0.0;
  ftBatteryCurrentSumOutP = 0.0;

  //---------------------------------------------------------------------------------------------------
  // filter value from PSUs a little bit to avoid outlier
  //
  aclPsuActualCurrentFilterP[0].init(3);
  aclPsuActualCurrentFilterP[1].init(3);
  aclPsuActualVoltageFilterP[0].init(3);
  aclPsuActualVoltageFilterP[1].init(3);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PVZeroClass::processControlAlgorithm(void)
{
  float ftActualCurrentTotalT = 0.0; // holds the current sum of both PSUs
  float ftTargetVoltageT = 0.0;      // is typically is equal to the MPPT voltage except the PSUs should be switched off
  int32_t slNumberOfStringsT = 0;    // number of available strings, typically number of PSUs

  //---------------------------------------------------------------------------------------------------
  // take values from PSU, make sure they are not negative
  //
  if (((int32_t)(aclPsuP[0].actualVoltage() * 100)) > 0)
  {
    aftActualVoltageOfPsuP[0] = ((float)aclPsuActualVoltageFilterP[0].process((int32_t)(aclPsuP[0].actualVoltage() * 100))) * 0.01;
  }
  else
  {
    aftActualVoltageOfPsuP[0] = 0.0;
  }
  if (((int32_t)(aclPsuP[0].actualCurrent() * 100)) > 0)
  {
    aftActualCurrentOfPsuP[0] = ((float)aclPsuActualCurrentFilterP[0].process((int32_t)(aclPsuP[0].actualCurrent() * 100))) * 0.01;
  }
  else
  {
    aftActualCurrentOfPsuP[0] = 0.0;
  }

  if (((int32_t)(aclPsuP[1].actualVoltage() * 100)) > 0)
  {
    aftActualVoltageOfPsuP[1] = ((float)aclPsuActualVoltageFilterP[1].process((int32_t)(aclPsuP[1].actualVoltage() * 100))) * 0.01;
  }
  else
  {
    aftActualVoltageOfPsuP[1] = 0.0;
  }

  if (((int32_t)(aclPsuP[1].actualCurrent() * 100)) > 0)
  {
    aftActualCurrentOfPsuP[1] = ((float)aclPsuActualCurrentFilterP[1].process((int32_t)(aclPsuP[1].actualCurrent() * 100))) * 0.01;
  }
  else
  {
    aftActualCurrentOfPsuP[1] = 0.0;
  }

  //---------------------------------------------------------------------------------------------------
  // update availability flags
  //
  abtPsuIsAvailableP[0] = aclPsuP[0].isAvailable();
  abtPsuIsAvailableP[1] = aclPsuP[1].isAvailable();

  //---------------------------------------------------------------------------------------------------
  // depending on availability of the PSUs update number of Strings and Current sum
  //
  if (abtPsuIsAvailableP[0])
  {
    ftActualCurrentTotalT += aftActualCurrentOfPsuP[0];
    slNumberOfStringsT++;
  }

  if (abtPsuIsAvailableP[1])
  {
    ftActualCurrentTotalT += aftActualCurrentOfPsuP[1];
    slNumberOfStringsT++;
  }

  //---------------------------------------------------------------------------------------------------
  // take values from MPPT
  //
  ftMpptBatteryCurrentP = clMpptP.batteryCurrent();
  ftMpptBatteryVoltageP = clMpptP.batteryVoltage();

  //---------------------------------------------------------------------------------------------------
  // calculate the real feed in power and other values for display in GUI
  //
  ftRealFeedInPowerP = (((aftActualVoltageOfPsuP[0] * aftActualCurrentOfPsuP[0]) +
                         (aftActualVoltageOfPsuP[1] * aftActualCurrentOfPsuP[1])) *
                        0.95); // consider 95% efficiency of the inverter

  ftTotalConsumptionP = ftRealFeedInPowerP + (float)consumptionPower;
  ftBatteryCurrentP = (ftMpptBatteryCurrentP - (aftActualCurrentOfPsuP[0] + aftActualCurrentOfPsuP[1]));

  //---------------------------------------------------------------------------------------------------
  // update values for curren in and current out in [Ah]
  //

  // update current values each second
  ftBatteryCurrentSumInSecP += ftMpptBatteryCurrentP;
  ftBatteryCurrentSumOutSecP += (aftActualCurrentOfPsuP[0] + aftActualCurrentOfPsuP[1]);

  // convert Asec value to Ah value:
  ftBatteryCurrentSumInP = (ftBatteryCurrentSumInSecP / 3600.0);
  ftBatteryCurrentSumOutP = (ftBatteryCurrentSumOutSecP / 3600.0);

  //---------------------------------------------------------------------------------------------------
  // just show all data we handle with
  //
  // I::get().logger() << F("Current Time: ") << I::get().time().currentTime() << endl;
  // I::get().logger() << F("loop() running on core ") << xPortGetCoreID() << "..." << endl;
  // I::get().logger() << F("MPPT Values: ") << String(ftMpptBatteryVoltageP, 3) << " V, " << String(ftMpptBatteryCurrentP, 3) << " A, state " << clMpptP.stateOfOperation() << endl;
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
  // I::get().logger() << F("Actual Values for CA: Consumption ") << String((float)consumptionPower, 3) << " W " << String(ftActualVoltageTotalT, 3) << " V, " << String(ftActualCurrentTotalT, 3) << " A" << endl;

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
  clBatGuardP.updateTime(I::get().time().currentTime());
  clBatGuardP.updateMpptState(clMpptP.stateOfOperation());
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
    //-------------------------------------------------------------------------------------------
    // Each time the PSUs are switched on after the were off the new cycle begins and
    // we reset values for current in and out sums.
    //
    if (((int32_t)(ftTargetVoltageT * 100)) == 0)
    {
      ftBatteryCurrentSumInSecP = 0.0;
      ftBatteryCurrentSumOutSecP = 0.0;
    }

    ftTargetVoltageT = clCaP.feedInTargetDcVoltage();
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
  if (McOvsSample(&atsOvsInputsP[0], analogRead(33)) == true)
  {
    ftPsuVccT = (float)McOvsGetResult(&atsOvsInputsP[0]);
    ftPsuVccT *= ftPsuSupplyGainP;
    ftPsuVccT += ftPsuSupplyOffsetP;

    I::get().logger() << F("Measured Charge Voltage: ") << McOvsGetResult(&atsOvsInputsP[0]) << F(" V") << endl;
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

    processControlAlgorithm();

    //-------------------------------------------------------------------------------------------
    //
    //
    if (triggerMqttSend)
    {
      triggerMqttSend = false;
      _ewcMqttHA.publishState("consumption" + I::get().config().getChipId(), String(consumptionPower));
      _ewcMqttHA.publishState("feedIn" + I::get().config().getChipId(), String(ftRealFeedInPowerP, 0));
      _ewcMqttHA.publishState("totalConsumption" + I::get().config().getChipId(), String(ftTotalConsumptionP, 0));
      _ewcMqttHA.publishState("batteryCurrent" + I::get().config().getChipId(), String(ftBatteryCurrentP, 0));
    }
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
  json["feed_in_power"] = ftRealFeedInPowerP; // consider 95% efficiency of the inverter
  json["enable_second_psu"] = consumptionPower;
  json["battery_state"] = strBatteryState;
  json["total_consumption"] = ftTotalConsumptionP;
  json["battery_current"] = ftBatteryCurrentP;
  json["battery_current_sum_in"] = ftBatteryCurrentSumInP;
  json["battery_current_sum_out"] = ftBatteryCurrentSumOutP;
  json["battery_state_info"] = strBatteryStateInfo;
  json["check_interval"] = PZI::get().config().getCheckInterval();
  json["next_check"] = _shelly3emConnector.infoSleepUntil();
  json["mppt_available"] = true;
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
  json["analog_available"] = !isnan(ftPsuVccT);
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
  if (isConsumptionPowerValid)
  {
    triggerMqttSend = true;
  }
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

void PVZeroClass::batteryGuard_EventCallback(BatteryGuard::State_te teSStateV)
{
  switch (teSStateV)
  {
  case BatteryGuard::State_te::eCharging:
    strBatteryState = String("charging");
    break;
  case BatteryGuard::State_te::eCharged:
    strBatteryState = String("charged");
    break;
  case BatteryGuard::State_te::eDischarging:
    strBatteryState = String("discharging");
    break;
  case BatteryGuard::State_te::eDischarged:
    strBatteryState = String("discharged");
    break;
  case BatteryGuard::State_te::eError:
    strBatteryState = String("error");
    break;
  case BatteryGuard::State_te::eMpptNotBulk:
    strBatteryState = String("not bulk");
    break;
  case BatteryGuard::State_te::eChargingWithDischarge:
    strBatteryState = String("charging with discharge");
    break;
  default:
    strBatteryState = "-";
  }
  strBatteryStateInfo = clBatGuardP.stateInfo();
  EWC::I::get().logger() << F("Battery status: ") << strBatteryState << endl;
  EWC::I::get().logger() << F("Battery info: ") << strBatteryStateInfo << endl;
}
