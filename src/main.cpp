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
bool onceTimeSet = false;

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void setup()
{
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
  EWC::I::get().config().paramHostname = String("pvz-") + EWC::I::get().config().getChipId();
  EWC::I::get().server().setBrand("PVZero", FIRMWARE_VERSION);
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
  }
  else
  {
    Serial.println("TIMEOUT_WIFI");
  }
  Serial.println("initialized");
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void loop()
{
  EWC::I::get().server().loop();
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!onceAfterConnect)
    {
      I::get().logger() << "connected, " << I::get().time().str() << endl;
      onceAfterConnect = true;
    }
    if (!onceTimeSet && I::get().time().timeAvailable())
    {
      I::get().logger() << "Send email about reboot" << endl;
      PZI::get().mail().sendChange("PVZ rebooted", I::get().time().str().c_str());
      onceTimeSet = true;
    }
  }
  else
  {
    // TODO: test if we should reinit after reconnect?
    onceAfterConnect = false;
  }

  pvz.loop();
}
