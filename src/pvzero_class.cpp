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

PvzMppt clMpptP;
UartMux clUartMuxP;
PvzPsu aclPsuP[2]; // support up to 2 PSUs

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

  // PID	0xA058
  // FW	163
  // SER#	HQ2302M4XVJ
  // V	56020
  // I	8300
  // VPV	101780
  // PPV	475
  // CS	3
  // MPPT	2
  // OR	0x00000000
  // ERR	0
  // LOAD	OFF
  // H19	3357
  // H20	385
  // H21	2003
  // H22	523
  // H23	1802
  // HSDS	12
  // Checksum	[19]

  // 0d 0a 50 49 44 09 30 78 41 30 35 38 0d 0a 46 57
  // 09 31 36 33 0d 0a 53 45 52 23 09 48 51 32 33 30
  // 32 4d 34 58 56 4a 0d 0a 56 09 35 36 33 32 30 0d
  // 0a 49 09 35 34 30 30 0d 0a 56 50 56 09 31 31 32
  // 39 39 30 0d 0a 50 50 56 09 33 32 30 0d 0a 43 53
  // 09 33 0d 0a 4d 50 50 54 09 31 0d 0a 4f 52 09 30
  // 78 30 30 30 30 30 30 30 30 0d 0a 45 52 52 09 30
  // 0d 0a 4c 4f 41 44 09 4f 46 46 0d 0a 48 31 39 09
  // 33 33 35 37 0d 0a 48 32 30 09 33 38 35 0d 0a 48
  // 32 31 09 32 30 30 33 0d 0a 48 32 32 09 35 32 33
  // 0d 0a 48 32 33 09 31 38 30 32 0d 0a 48 53 44 53
  // 09 31 32 0d 0a 43 68 65 63 6b 73 75 6d 09 1f

  // 16*12 = ca. 192 bytes

  // Serial2.begin(19200);
  // Serial2.setRxFIFOFull(5);
  // Serial2.onReceive(onReceiveFunction, false);
  // mppt.begin();

  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());

  //---------------------------------------------------------------------------------------------------
  // Configure a task for UART application that handles Serial2 communication with both PSUs and the
  // Victron SmartSolar charger.
  //
  xTaskCreatePinnedToCore(
      this->taskUartApp, /* Function to implement the task */
      "UartApp",         /* Name of the task */
      2048,              /* Stack size in words */
      NULL,              /* Task input parameter */
      0,                 /* Priority of the task */
      &clTaskUartAppP,   /* Task handle. */
      0);                /* Core where the task should run */
}

// // Definieren Sie einen struct, um die Daten aus der Tabelle zu speichern
// struct SensorData
// {
//   char *parameterName;
//   char *value;
// };

// // Deklarieren Sie ein Array von SensorData-Elementen
// SensorData sensorData[20]; // Anpassen Sie die Größe an die Anzahl der Zeilen in der Tabelle

// // Definieren Sie die Trennzeichen
// const char *EOL = "\r\n"; // Zeilentrauf
// const char *SEP = "\t";   // Spaltenbegrenzer

// void parseTable(char *tableData)
// {
//   // char str[256];
//   // strcpy(str, tableData);
//   // // int i = 0;
//   // // while (i < 256)
//   // // {
//   // //   str[i] = tableData[i];

//   // // }
//   // char *line = strtok(str, EOL); // Zeile für Zeile parsen
//   // EWC::I::get().logger() << " str : " << str << endl;
//   // char *token;
//   int i = 0;

//   // while (line != NULL)
//   // {
//   //   EWC::I::get().logger() << " Line : " << line << endl;
//   //   token = strtok(line, SEP); // Spalten parsen
//   //   sensorData[i].parameterName = token;
//   //   EWC::I::get().logger() << " token A : " << token << endl;
//   //   token = strtok(NULL, SEP);
//   //   sensorData[i].value = token;
//   //   EWC::I::get().logger() << " token B : " << token << endl;

//   //   i++;
//   //   line = strtok(NULL, EOL);
//   // }
//   char *ptr, *savePtr, *p, *saveP;
//   // const char delim[] = "|";

//   ptr = strtok_r(tableData, EOL, &savePtr);
//   while (ptr != NULL)
//   {
//     // EWC::I::get().logger() << " Line : " << ptr << endl;
//     // hier haben wir alles zwischen | (außen)
//     // Serial.println(ptr);
//     p = strtok_r(ptr, SEP, &saveP);
//     // while (p != NULL)
//     // {
//     sensorData[i].parameterName = p;
//     // EWC::I::get().logger() << " token : " << p << endl;

//     // // alles zwischen : (innen)
//     // Serial.print("____");
//     // Serial.println(p);
//     p = strtok_r(NULL, SEP, &saveP);
//     sensorData[i].value = p;
//     // }
//     i++;
//     // außen neu testen
//     ptr = strtok_r(NULL, EOL, &savePtr);
//   }
// }

// std::mutex serial_mtx;
// #include "vedirect_parser.h"

// static bool btVictronReceptionPendingT = true;
int32_t slReadBytesT = 0;
char aszReadDataT[256];
// static uint8_t ulChecksumT;

//---------------------------------------------------------------------------------------------------------
typedef enum UartAppSm_e
{
  eUART_APP_SM_MPPT_e = 0,
  eUART_APP_SM_PSU1_e,
  eUART_APP_SM_PSU2_e,
} UartAppSm_te;

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PVZeroClass::taskUartApp(void *pvParameters)
{
  static uint32_t uqTimeStampT = 0;
  static UartAppSm_te teUartAppStateG = eUART_APP_SM_MPPT_e;
  static int32_t aslPsuStateT[] = {0, 0};
  static int32_t aslPsuStateBeginT[] = {0, 0};
  static bool abtPsuInitSuccessT[] = {false, false};
  static int32_t slPsuReturnT;

  //---------------------------------------------------------------------------------------------------
  // intialise the multiplexer and enable IF 1 for victron
  //
  clUartMuxP.init();
  clUartMuxP.enable(UartMux::eIF_1);

  //---------------------------------------------------------------------------------------------------
  // perform configuration of serial 2
  //
  Serial2.begin(19200);

  while (true)
  {
    //-------------------------------------------------------------------------------------------
    // handle state and the corresponding transitions of the Serial2 state machine
    //
    switch (teUartAppStateG)
    {
    case eUART_APP_SM_MPPT_e:

      //-----------------------------------------------------------------------------------
      // store all incoming chars, this requires about 95 ms
      //
      while (Serial2.available())
      {
        // copy and increase char counter
        aszReadDataT[slReadBytesT] = Serial2.read();
        slReadBytesT++;

        //---------------------------------------------------------------------------
        // store time when read the last char for later evaluation
        //
        uqTimeStampT = millis();
      }

      //-----------------------------------------------------------------------------------
      // assume that after 20 ms of silence all data has been received from victron
      //
      if ((millis() > (uqTimeStampT + 10)) && (slReadBytesT > 0))
      {
        //---------------------------------------------------------------------------
        // append the a null to terminate the string
        //
        aszReadDataT[slReadBytesT] = '\0'; // Append a null

        //---------------------------------------------------------------------------
        // provide the received frame from victron to the mppt class for processing
        //
        clMpptP.updateFrame(aszReadDataT, slReadBytesT);

        //---------------------------------------------------------------------------
        // pre pare next reception data
        //
        slReadBytesT = 0;

        ///---------------------------------------------------------------------------
        ///---------------------------------------------------------------------------
        // before next send of frame from victron we have at least 900ms time
        // poll an set both PSUs
        //

        //---------------------------------------------------------------------------
        // perform transition to the next state
        //
        clUartMuxP.enable(UartMux::eIF_2);
        teUartAppStateG = eUART_APP_SM_PSU1_e;
      }
      break;

    case eUART_APP_SM_PSU1_e:
      //-----------------------------------------------------------------------------------
      // make sure we leave this state 300ms after final char of Victron Text frame
      // => 105 ms for reception and storage of MPPT Text Frame, than make uqTimeStampT
      //    + 400 ms for PSU0
      //
      if (millis() > (uqTimeStampT + 400))
      {
        //---------------------------------------------------------------------------
        // define where should we at new cycle
        //
        aslPsuStateT[0] = aslPsuStateBeginT[0];

        //---------------------------------------------------------------------------
        // perform transition to the next state
        //
        clUartMuxP.enable(UartMux::eIF_3);
        teUartAppStateG = eUART_APP_SM_PSU2_e;
      }

      //-----------------------------------------------------------------------------------
      // stay in this state and treat the PSU at MUX IF 2
      //
      else
      {
        switch (aslPsuStateT[0])
        {
          // init state of PSU
          // - in case of error this state needs about 105 ms
          // - in case success this state needs about 35 ms
        case 0:
          slPsuReturnT = aclPsuP[0].init(Serial2);

          if (slPsuReturnT == 1)
          {
            aclPsuP[0].enable(true);
            aslPsuStateBeginT[0] = 1;
            aslPsuStateT[0] = 1;
          }
          else
          {
            aslPsuStateT[0] = 10; // fail to init, may be not connected
          }
          break;

          // Write and Read
          // - in case of error this state needs about 105 ms
          // - in case of success this state needs about 55 ms
        case 1:
          slPsuReturnT = aclPsuP[0].write();
          if (slPsuReturnT < 0)
          {
            aslPsuStateT[0] = 11; // fail to init, may be not connected
          }
          else
          {
            slPsuReturnT = aclPsuP[0].read();
            if (slPsuReturnT < 0)
            {
              aslPsuStateT[0] = 12; // fail to init, may be not connected
            }
            else
            {
              aslPsuStateT[0] = 5;
            }
          }
          break;

        case 5: // Success, we are finish in this cycle, stay in here
          break;

        case 10: // Error State: Fail to init the PSU
          EWC::I::get().logger() << "PSU0 Error at init: " << slPsuReturnT << endl;
          aslPsuStateT[0] = 100;
          break;

        case 11: // Error State: while writing to PSU
          EWC::I::get().logger() << "PSU0 Error at write: " << slPsuReturnT << endl;
          aslPsuStateT[0] = 100;
          break;

        case 12: // Error State: while reading from PSU
          EWC::I::get().logger() << "PSU0 Error at read: " << slPsuReturnT << endl;
          aslPsuStateT[0] = 100;
          break;

        case 100: // Error State: prepare re-initialisation
          aslPsuStateBeginT[0] = 0;
        default:
          break;
        }
      }

      break;
    case eUART_APP_SM_PSU2_e:

      //-----------------------------------------------------------------------------------
      // make sure we leave this state 800ms after final char of Victron Text frame
      // => 105 ms for reception and storage of MPPT Text Frame, than make uqTimeStampT
      //    + 400 ms for PSU0
      //    + 400 ms for PSU1
      //
      if (millis() > (uqTimeStampT + 800))
      {

        //---------------------------------------------------------------------------
        // define where should we at new cycle
        //
        aslPsuStateT[1] = aslPsuStateBeginT[1];

        //---------------------------------------------------------------------------
        // perform transition to the next state
        //
        clUartMuxP.enable(UartMux::eIF_1);
        teUartAppStateG = eUART_APP_SM_MPPT_e;
      }

      //-----------------------------------------------------------------------------------
      // stay in this state and treat the PSU at MUX IF 3
      //
      else
      {
        switch (aslPsuStateT[1])
        {
          // init state of PSU
          // - in case of error this state needs about 105 ms
          // - in case success this state needs about 35 ms
        case 0:
          slPsuReturnT = aclPsuP[1].init(Serial2);

          if (slPsuReturnT == 1)
          {
            aclPsuP[1].enable(true);
            aslPsuStateBeginT[1] = 1;
            aslPsuStateT[1] = 1;
          }
          else
          {
            aslPsuStateT[1] = 10; // fail to init, may be not connected
          }
          break;

          // Write and Read
          // - in case of error this state needs about 105 ms
          // - in case of success this state needs about 55 ms
        case 1:
          slPsuReturnT = aclPsuP[1].write();
          if (slPsuReturnT < 0)
          {
            aslPsuStateT[1] = 11; // fail to init, may be not connected
          }
          else
          {
            slPsuReturnT = aclPsuP[1].read();
            if (slPsuReturnT < 0)
            {
              aslPsuStateT[1] = 12; // fail to init, may be not connected
            }
            else
            {
              aslPsuStateT[1] = 5;
            }
          }

          break;

        case 5: // Success, we are finish in this cycle, stay in here
          break;

        case 10: // Error State: Fail to init the PSU
          EWC::I::get().logger() << "PSU1 Error at init: " << slPsuReturnT << endl;
          aslPsuStateT[1] = 100;
          break;

        case 11: // Error State: while writing to PSU
          EWC::I::get().logger() << "PSU1 Error at write: " << slPsuReturnT << endl;
          aslPsuStateT[1] = 100;
          break;

        case 12: // Error State: while reading from PSU
          EWC::I::get().logger() << "PSU1 Error at read: " << slPsuReturnT << endl;
          aslPsuStateT[1] = 100;
          break;

        case 100: // Error State: prepare re-initialisation
          aslPsuStateBeginT[1] = 0;
        default:
          break;
        }
      }
      break;

    default:
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PVZeroClass::processControlAlgorithm(void)
{
  float ftActualVoltageT;
  float ftActualCurrentT;
  int32_t slNumberOfStringsT;

  //---------------------------------------------------------------------------------------------------
  // just show all data we handle with
  //
  I::get().logger() << F("Current Time: ") << I::get().time().currentTime() << endl;
  I::get().logger() << F("loop() running on core ") << xPortGetCoreID() << "..." << endl;
  I::get().logger() << F("MPPT Values: ") << String(clMpptP.batteryVoltage(), 3) << " V, " << String(clMpptP.batteryCurrent(), 3) << " A" << endl;
  I::get().logger() << F("PSU0 actual values: ") << String(aclPsuP[0].actualVoltage(), 3) << " V, " << String(aclPsuP[0].actualCurrent(), 3) << " A, is available " << aclPsuP[0].isAvailable() << endl;
  I::get().logger() << F("PSU0 target values: ") << String(aclPsuP[0].targetVoltage(), 3) << " V, " << String(aclPsuP[0].targetCurrent(), 3) << " A" << endl;
  I::get().logger() << F("PSU1 actual values: ") << String(aclPsuP[1].actualVoltage(), 3) << " V, " << String(aclPsuP[1].actualCurrent(), 3) << " A, is available " << aclPsuP[1].isAvailable() << endl;
  I::get().logger() << F("PSU1 target values: ") << String(aclPsuP[1].targetVoltage(), 3) << " V, " << String(aclPsuP[1].targetCurrent(), 3) << " A" << endl;

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
    //       provide this value from GUI
    //
    // Set the power to 350Wh
    //
    clCaP.updateConsumptionPower(350.0);
  }

  //---------------------------------------------------------------------------------------------------
  // consider feed in of both PSUs and provide values to the control algorithm
  //
  ftActualVoltageT = 0.0;
  ftActualCurrentT = 0.0;
  slNumberOfStringsT = 0;
  if (aclPsuP[0].isAvailable())
  {
    ftActualVoltageT = aclPsuP[0].actualVoltage();
    ftActualCurrentT += aclPsuP[0].actualVoltage();
    slNumberOfStringsT++;
  }
  else if (aclPsuP[1].isAvailable())
  {
    ftActualVoltageT = aclPsuP[1].actualVoltage();
    ftActualCurrentT += aclPsuP[1].actualVoltage();
    slNumberOfStringsT++;
  }

  clCaP.updateFeedInActualDcValues(ftActualVoltageT, ftActualCurrentT);

  //---------------------------------------------------------------------------------------------------
  // finally trigger the processing of data
  //
  clCaP.process();

  //---------------------------------------------------------------------------------------------------
  // update value for battery guard
  //
  clBatGuardP.updateVoltage(clMpptP.batteryVoltage());
  clBatGuardP.updateCurrent(clMpptP.batteryCurrent());
  clBatGuardP.updateTime(I::get().time().currentTime());
  clBatGuardP.process();

  //---------------------------------------------------------------------------------------------------
  // Update target data of the PSU only one time in second
  //
  if (slNumberOfStringsT > 0)
  {
    //-------------------------------------------------------------------------------------------
    // Consider current limit and adjust it corresponding to the number of connected PSUs
    //
    ftActualCurrentT = clBatGuardP.limitedCurrent(clCaP.feedInTargetDcCurrent());
    if (slNumberOfStringsT > 1)
    {
      ftActualCurrentT /= 2.0;
    }

    //-------------------------------------------------------------------------------------------
    // make sure current do not exceeds the limit provided by user
    //
    if (ftActualCurrentT > PZI::get().config().getMaxAmperage())
    {
      ftActualCurrentT = PZI::get().config().getMaxAmperage();
    }
  }

  //---------------------------------------------------------------------------------------------------
  // update PSU0 if available
  //
  if (aclPsuP[0].isAvailable())
  {
    aclPsuP[0].set(clCaP.feedInTargetDcVoltage(), clBatGuardP.limitedCurrent(ftActualCurrentT));
  }
  else
  {
    aclPsuP[0].set(0.0, 0.0);
  }

  //---------------------------------------------------------------------------------------------------
  // update PSU1 if available
  //
  //
  if (aclPsuP[1].isAvailable())
  {
    aclPsuP[1].set(clCaP.feedInTargetDcVoltage(), clBatGuardP.limitedCurrent(ftActualCurrentT));
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
  }

  //---------------------------------------------------------------------------------------------------
  // parse frame from MPPT device
  //
  clMpptP.parse();

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
  json["feed_in_power"] = String(clCaP.feedInTargetPower(), 0);
  json["psu_vcc"] = String(0.0, 2);
  json["enable_second_psu"] = consumptionPower;
  json["battery_state"] = strBatteryState;
  json["charge_voltage"] = String(clMpptP.batteryVoltage(), 2);
  json["charge_current"] = String(clMpptP.batteryCurrent(), 2);
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
    strBatteryState = String("charging" + clBatGuardP.stateInfo());
    break;
  case BatteryGuard::State_te::eCharged:
    strBatteryState = String("charged" + clBatGuardP.stateInfo());
    break;
  case BatteryGuard::State_te::eDischarging:
    strBatteryState = String("discharging" + clBatGuardP.stateInfo());
    break;
  case BatteryGuard::State_te::eDischarged:
    strBatteryState = String("discharged" + clBatGuardP.stateInfo());
    break;
  case BatteryGuard::State_te::eError:
    strBatteryState = String("error" + clBatGuardP.stateInfo());
    break;
  default:
    strBatteryState = "-";
  }
  EWC::I::get().logger() << F("Battery status: ") << strBatteryState << endl;
}
