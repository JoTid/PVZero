/**************************************************************

This file is a part of
https://github.com/JoTid/PVZero

Copyright [2020] Johann Tiderko

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
#include "lcd.h"
#include <U8g2lib.h>
#include <Wire.h>
#ifdef ESP8266
    #include <ewcRTC.h>
#endif
#include <ewcLogger.h>
#include "shelly_em3_connector.h"
#include "config.h"
#include "device_state.h"


using namespace EWC;
using namespace PVZERO;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

void u8g2_prepare()
{
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);
}

void progressBar2()
{
    static int32_t ulMoveS = 0;
    static bool btOperatorT = true;

    if (btOperatorT)
    {
        ulMoveS += 2;
        if (ulMoveS >= 64)
        {
            btOperatorT = false;
        }
    }
    else
    {
        ulMoveS -= 2;
        if (ulMoveS <= 0)
        {
            btOperatorT = true;
        }
    }
    u8g2.drawLine(64 - ulMoveS, 52, 64 + ulMoveS, 52);
}

LCD::LCD() 
{
}

LCD::~LCD()
{
}

void LCD::setup(bool resetConfig)
{
    I::get().logger() << "[LCD] Initialise..." << endl;
    u8g2.begin();
    u8g2_prepare();
}

void LCD::loop()
{
    if (PZI::get().config().lcdEnabled) {
        String clLineT;
        char battVoltageStr[20]; // array is used for spirntf operations
        static int slOldTimeS = millis();

        //---------------------------------------------------------------------------------------------------
        // collect data
        //

        //---------------------------------------------------------------------------------------------------
        // preopare the LCD
        //

        if (millis() > (slOldTimeS + 20))
        {
            u8g2.clearBuffer();
            u8g2_prepare();
            u8g2.setFontDirection(0);

            u8g2.drawStr(0, 0, "PVZero");
            u8g2.drawLine(0, 12, 128, 12);

            progressBar2();
            // u8g2.drawLine(0, 52, 128, 52);
            slOldTimeS = millis();
            if (PZI::get().deviceState().currentState() == DeviceState::RUNNING)
            {
                u8g2.drawStr(128 - u8g2.getStrWidth(String("v1.00").c_str()), 0, String("v1.00").c_str());
            }
        }
        u8g2.sendBuffer();
    }
}
