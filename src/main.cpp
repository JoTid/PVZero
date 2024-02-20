#include <Arduino.h>
#include <ewcInterface.h>
#include "pvzero_class.h"

using namespace EWC;
using namespace PVZ;

#define LED1_PIN 12
uint32_t TIMEOUT_WIFI = 60000;
PVZeroClass pvz;


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
   // Use serial interface for debug
   //
   Serial.begin(115200); // Setup serial object and define transmission speed
   Serial.println("Starting setup...");
   
   //---------------------------------------------------------------------------------------------------
   // 
   //
   Serial.println();
   Serial.print("ESP heap: ");
   Serial.println(ESP.getFreeHeap());

   //---------------------------------------------------------------------------------------------------
   // initialise EspWebConfig
   //
   EWC::I::get().config().paramDeviceName = String("pvz-") + EWC::I::get().config().getChipId();
   EWC::I::get().config().paramAPName = String("pvz-") + EWC::I::get().config().getChipId();
   EWC::I::get().server().setBrand("PVZero", FRIMWARE_VERSION);
   EWC::I::get().led().enable(true, LED1_PIN, HIGH);

   //---------------------------------------------------------------------------------------------------
   // initialise PVZero
   //
   pvz.setup();
   Serial.print("ESP heap: ");
   Serial.println(ESP.getFreeHeap());
   Serial.println("acconfig");
   if (I::get().config().paramWifiDisabled)
   {
      Serial.println("STANDALONE");
      PZI::get().lcd().warning("Configuration required", "Connect to AP:", "- Name of AP -");
   }
   else
   {
      Serial.print("TIMEOUT_WIFI");
      Serial.println(WiFi.SSID());
      PZI::get().lcd().busy("Connecting to SSID:", "unknown");
   }
   Serial.println("initialized");
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void loop() {
   EWC::I::get().server().loop();
   if (WiFi.status() == WL_CONNECTED)
   {
      if (!onceAfterConnect)
      {
        I::get().logger() << "connected, " << PZI::get().time().str() << endl;
        onceAfterConnect = true;
      }
   } else {
      // TODO: test if we should reinit after reconnect?
      onceAfterConnect = false;
   }

   pvz.loop();
}
