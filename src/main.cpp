#include <Arduino.h>
#include <ewcInterface.h>
#include "pvzero_class.h"

#include <U8g2lib.h>
#include <Wire.h>
#include "lcd.hpp"
#include "led.hpp"

using namespace EWC;
using namespace PVZERO;

uint32_t TIMEOUT_WIFI = 60000;
uint32_t msStart = 0;
PVZeroClass pvzero;


/*--------------------------------------------------------------------------------------------------------------------*\
** Global modules                                                                                                     **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/ 


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
   // initialise the LCD and trigger first display
   //
   PZI::get().lcd().init();
   PZI::get().lcd().process();

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
     Serial.println("STANDALONE");
     PZI::get().lcd().warning("Configuration required", "Connect to AP:", "- Name of AP -");
     PZI::get().deviceState().setState(DeviceState::State::STANDALONE);
   }
   else
   {
    Serial.print("TIMEOUT_WIFI");
    Serial.println(WiFi.SSID());
    PZI::get().lcd().busy("Connecting to SSID:", "unknown");
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

       PZI::get().lcd().ok();
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
   PZI::get().lcd().process();
}
