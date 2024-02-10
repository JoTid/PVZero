#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "lcd.hpp"
#include "led.hpp"

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
   Serial.printf_P(PSTR("Time: %d\n"), millis()); // using PROGMEM

   //--------------------------------------------------------------------------------------------------- 
   // initialise the LCD
   //
   lcdInit();
   lcdProcess(0);

}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void loop() {
   static uint32_t ulLedTimeS = millis(); 

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
   //
   //
   lcdProcess(0);

}