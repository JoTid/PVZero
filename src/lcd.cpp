#include <U8g2lib.h>
#include <Wire.h>
#include "lcd.hpp"

#include <WiFi.h>
#include <ewcLogger.h>
#include "shelly_em3_connector.h"
#include "config.h"
#include "device_state.h"
#include "pvzero_class.h"

using namespace EWC;
using namespace PVZERO;

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
LCD::LCD()
{
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
LCD::~LCD()
{
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t getRSSIasQuality(int32_t slRssiV)
{
  int32_t quality = 0;
  if (slRssiV <= -100)
  {
    quality = 0;
  }
  else if (slRssiV >= -50)
  {
    quality = 100;
  }
  else
  {
    quality = 2 * (slRssiV + 100);
  }
  return quality;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::busy(String clReasonV, String clValueV)
{
  slPendingScreenP = eLCD_Busy;
  clBusyReasonP = clReasonV;
  clBusyValueP = clValueV;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::ok()
{
  slPendingScreenP = eLCD_OK;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::busyScreen(void)
{
  static int32_t slDelayTimeT = 0 / LCD_REFRESH_TIME;
  static int32_t ulMoveS = 0;
  static bool btOperatorT = true;

  if (btOperatorT)
  {
    if (slDelayTimeT == 0)
      ulMoveS += 2;
    if (ulMoveS >= 64)
    {
      btOperatorT = false;
    }
  }
  else
  {
    if (slDelayTimeT == 0)
      ulMoveS -= 2;
    if (ulMoveS <= 0)
    {
      btOperatorT = true;
    }
  }

  u8g2.drawLine(64 - ulMoveS, 52, 64 + ulMoveS, 52);

  if (slDelayTimeT > 0)
  {
    slDelayTimeT--;
  }
  else
  {
    slDelayTimeT = 0 / LCD_REFRESH_TIME;
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::init(void)
{
  u8g2.begin();
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::footer(void)
{
  String clStringT;
  static int32_t slDelayTimeT = ((LCD_FOOTER_TOGGLE_TIME >> 1) / LCD_REFRESH_TIME);

  //---------------------------------------------------------------------------------------------------
  // show the info for the footer that should be shown alternately
  //
  if (slDelayTimeT > 0)
  {
    clStringT = WiFi.SSID();
    slDelayTimeT--;
  }
  else 
  {
    clStringT = WiFi.localIP().toString().c_str();
    slDelayTimeT--;
  }

  //--------------------------------------------------------------------------------------------------- 
  // reset the delay time counter
  //
  if (slDelayTimeT < (((LCD_FOOTER_TOGGLE_TIME >> 1) / LCD_REFRESH_TIME) * (-1)))
  {
    slDelayTimeT = (((LCD_FOOTER_TOGGLE_TIME >> 1) / LCD_REFRESH_TIME));
  }

  //---------------------------------------------------------------------------------------------------
  // print the left side of line
  //
  u8g2.setCursor(0, 55);
  u8g2.print(clStringT);

  //---------------------------------------------------------------------------------------------------
  // print the right side of line
  //
  clStringT = String(getRSSIasQuality(WiFi.RSSI()));
  clStringT += String(" %");
  u8g2.setCursor(128 - u8g2.getStrWidth(clStringT.c_str()), 55);
  u8g2.print(clStringT);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::header(void)
{
  String clStringT;
  String clTimeT;
  int32_t slSplitT;
  static int32_t slDelayTimeT = ((LCD_HEADER_TOGGLE_TIME >> 1) / LCD_REFRESH_TIME);

  //---------------------------------------------------------------------------------------------------
  // show the info for the footer that should be shown alternately
  //
  if (slDelayTimeT > 0)
  {
    //------------------------------------------------------------------------------------------- 
    // format date value to 12.02.24
    //
    clTimeT = PZI::get().time().str(); // get time in format 2018-05-28T16:00:13Z
    clStringT  = clTimeT.substring(8,10);
    clStringT += ".";
    clStringT += clTimeT.substring(5,7);
    clStringT += ".";
    clStringT += clTimeT.substring(2,4);
    slDelayTimeT--;
  }
  else
  {
    //------------------------------------------------------------------------------------------- 
    // format time value to 14:15
    //
    clTimeT = PZI::get().time().str(); // get time in format 2018-05-28T16:00:13Z
    slSplitT = clTimeT.indexOf("T");
    clStringT = clTimeT.substring(slSplitT+1, clTimeT.length()-3);
    slDelayTimeT--;
  }

  //--------------------------------------------------------------------------------------------------- 
  // reset the delay time counter
  //  
  if (slDelayTimeT < (((LCD_HEADER_TOGGLE_TIME >> 1) / LCD_REFRESH_TIME) * (-1)))
  {
    slDelayTimeT = (((LCD_HEADER_TOGGLE_TIME >> 1) / LCD_REFRESH_TIME));
  }

  //---------------------------------------------------------------------------------------------------
  // print the left side of line
  //
  u8g2.drawLine(0, 12, 128, 12);
  u8g2.drawStr(0, 0, "PVZero");

  //---------------------------------------------------------------------------------------------------
  // if time value is not available than display version number.
  //
  if (WiFi.isConnected() != true)
  {
    clStringT = "v";
    clStringT += FRIMWARE_VERSION;
    u8g2.drawStr(128 - u8g2.getStrWidth(String(clStringT).c_str()), 0, String(clStringT).c_str());
  }

  //---------------------------------------------------------------------------------------------------
  // print the right side of line
  //
  u8g2.setCursor(128 - u8g2.getStrWidth(clStringT.c_str()), 0);
  u8g2.print(clStringT);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::process(void)
{
  static unsigned long ulOldTimeS;
  static uint32_t ulRefreshTimeT;
  String clStringT;

  //---------------------------------------------------------------------------------------------------
  // count the millisecond ticks and avoid overflow
  //
  unsigned long ulNewTimeT = millis();
  if (ulNewTimeT != ulOldTimeS)
  {
    if (ulNewTimeT > ulOldTimeS)
    {
      ulRefreshTimeT += (uint32_t)(ulNewTimeT - ulOldTimeS);
    }
    else
    {
      ulRefreshTimeT += (uint32_t)(ulOldTimeS - ulNewTimeT);
    }
    ulOldTimeS = ulNewTimeT;
  }

  //---------------------------------------------------------------------------------------------------
  // refresh the LCD only within define time
  //
  if (ulRefreshTimeT > LCD_REFRESH_TIME)
  {
    ulRefreshTimeT = 0;

    //-------------------------------------------------------------------------------------------
    // prepare the lcd for drawing
    //
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontRefHeightExtendedText();
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);

    //-------------------------------------------------------------------------------------------
    // print header
    //
    header();

    //-------------------------------------------------------------------------------------------
    // show informations in normal operation
    //
    if ((slPendingScreenP == eLCD_OK) && (WiFi.isConnected() == true))
    {
      //-----------------------------------------------------------------------------------
      // Check the connection to the 3EM Meter is established and data is available
      //
      if (PZI::get().shellyEm3Connector().isValid())
      {
        u8g2.drawLine(0, 52, 128, 52);

        u8g2.setCursor(4, 16);
        clStringT = "Cons power: ";
        clStringT += String(PZI::get().shellyEm3Connector().currentExcess());
        clStringT += " Wh";
        u8g2.print(clStringT);

        u8g2.setCursor(4, 16 + 12);
        u8g2.print("Feed power: --- Wh");

        u8g2.setCursor(4, 16 + 12 + 12);
        u8g2.print("Battery:    -- %");
      }

      //---------------------------------------------------------------------------
      // print waring with connection failure to the power meter
      //
      else
      {
        warnScreen();
        u8g2.drawLine(0, 52, 128, 52);
        u8g2.setCursor(4, 16);
        u8g2.print("Request ERROR at 3EM");
        u8g2.setCursor(4, 16 + 12);
        u8g2.print("Check URL:");
        u8g2.setCursor(4, 16 + 12 + 12);
        u8g2.print(PZI::get().config().shellyEm3Uri);
      }

      //-----------------------------------------------------------------------------------
      // footer is only printed in normal operation without busy and warning state
      //
      footer();
    }

    //-------------------------------------------------------------------------------------------
    // Display info while busy
    //
    else if (slPendingScreenP == eLCD_Busy)
    {
      busyScreen();
      u8g2.drawStr(0, 15, String(clBusyReasonP).c_str());
      u8g2.setFont(u8g2_font_10x20_tf);
      u8g2.drawStr((128 - u8g2.getStrWidth(String(clBusyValueP).c_str())) / 2, 30, String(clBusyValueP).c_str());
      u8g2.setFont(u8g2_font_6x10_tf);
    }

    //-------------------------------------------------------------------------------------------
    // Display info while warning
    //
    else if (slPendingScreenP == eLCD_Warning)
    {
      warnScreen();
      u8g2.drawLine(0, 52, 128, 52);
      u8g2.setCursor(4, 16);
      u8g2.print(aclWarnLinesP[0]);
      u8g2.setCursor(4, 16 + 12);
      u8g2.print(aclWarnLinesP[1]);
      u8g2.setCursor(4, 16 + 12 + 12);
      u8g2.print(aclWarnLinesP[2]);
    }

    //-------------------------------------------------------------------------------------------
    // perform update of display
    //
    u8g2.sendBuffer();
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::warning(String clLine1V, String clLine2V, String clLine3V)
{
  slPendingScreenP = eLCD_Warning;
  aclWarnLinesP[0] = clLine1V;
  aclWarnLinesP[1] = clLine2V;
  aclWarnLinesP[2] = clLine3V;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void LCD::warnScreen(void)
{
  static int32_t slDelayTimeT = (LCD_WARNING_TOGGLE_TIME / LCD_REFRESH_TIME);

  //---------------------------------------------------------------------------------------------------
  // let the warn symbol blink
  //
  if (slDelayTimeT > 0)
  {
    u8g2.drawLine(64, 0, 58, 10);
    u8g2.drawLine(64, 0, 70, 10);
    u8g2.drawLine(58, 10, 70, 10);
    u8g2.drawLine(64, 4, 64, 8);
    slDelayTimeT--;
  }
  else
  {
    slDelayTimeT--;
  }

  //--------------------------------------------------------------------------------------------------- 
  // reset the delay time counter
  //
  if (slDelayTimeT < ((LCD_WARNING_TOGGLE_TIME / LCD_REFRESH_TIME) * (-1)))
  {
    slDelayTimeT = (LCD_WARNING_TOGGLE_TIME / LCD_REFRESH_TIME);
  }
}
