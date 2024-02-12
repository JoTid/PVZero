#include <Arduino.h>
#include <ewcInterface.h>
#include "pvzero_class.h"

#include <U8g2lib.h>
#include <Wire.h>
#include "lcd.hpp"
#include "led.hpp"

using namespace EWC;
using namespace PVZERO;

const char *FRIMWARE_VERSION = "1.0.0";
uint32_t TIMEOUT_WIFI = 60000;
uint32_t msStart = 0;
PVZeroClass pvzero;


/*--------------------------------------------------------------------------------------------------------------------*\
** Global modules                                                                                                     **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/ 

/**
 * @brief Application LCD software module
 * 
 */
LCD *pclAppLcdG;

bool onceAfterConnect = false;

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void setup() {
   //--------------------------------------------------------------------------------------------------- 
   // initialise LED
   //
   ledInit();
   ledSet(0, true);

   //--------------------------------------------------------------------------------------------------- 
   // Use serial interface for debug
   //
   Serial.begin(115200); // Setup serial object and define transmission speed
   Serial.println("Starting setup...");
   
   //--------------------------------------------------------------------------------------------------- 
   // initialise the LCD
   //
   pclAppLcdG = new LCD();
   pclAppLcdG->init();

   //---------------------------------------------------------------------------------------------------
   // initialise the LCD
   //
   Serial.println();
   Serial.print("ESP heap: ");
   Serial.println(ESP.getFreeHeap());
   EWC::I::get().server().setBrand("PVZero", FRIMWARE_VERSION);
   pvzero.setup();
   Serial.print("ESP heap: ");
   Serial.println(ESP.getFreeHeap());
   Serial.println("acconfig");
   if (I::get().config().paramWifiDisabled)
   {
     PZI::get().deviceState().setState(DeviceState::State::STANDALONE);
   }
   else
   {
     PZI::get().deviceState().setState(DeviceState::State::INIT, TIMEOUT_WIFI);
   }
   Serial.println("initialized");
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void loop() {
   static uint32_t ulLedTimeS = millis();

   EWC::I::get().server().loop();
   if (WiFi.status() == WL_CONNECTED)
   {
     if (!onceAfterConnect)
     {
       I::get().logger() << "connected, " << PZI::get().time().str() << endl;
       onceAfterConnect = true;
       PZI::get().deviceState().setState(DeviceState::State::INIT);
     }
   }
   pvzero.loop();

   //--------------------------------------------------------------------------------------------------- 
   // toggle LED
   //
   if (millis() > (ulLedTimeS + 500))
   {
      ulLedTimeS = millis();
      if (ledIsOn(0))
      {
         ledSet(0, false);
      }
      else
      {
         ledSet(0, true);
      }
   }

   //--------------------------------------------------------------------------------------------------- 
   // Trigger the application LCD
   //
   pclAppLcdG->process();




}
