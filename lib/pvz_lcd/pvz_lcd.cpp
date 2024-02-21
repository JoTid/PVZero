//====================================================================================================================//
// File:          pvz_lcd.cpp                                                                                         //
// Description:   PhotoVoltaics Zero - LCD                                                                            //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//


/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/ 
#include <U8g2lib.h>
#include <Wire.h>
#include "pvz_lcd.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzLcd::PvzLcd()
{
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzLcd::~PvzLcd()
{
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::busy(String clReasonV, String clValueV)
{
  slPendingScreenP = eLCD_Busy;
  clBusyReasonP = clReasonV;
  clBusyValueP = clValueV;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::busyScreen(void)
{
  static int32_t ulMoveS = 0;
  static bool btOperatorT = true;

  if (btOperatorT)
  {
    ulMoveS += (400/LCD_REFRESH_TIME);
    if (ulMoveS >= 64)
    {
      btOperatorT = false;
    }
  }
  else
  {
    ulMoveS -= (400/LCD_REFRESH_TIME);
    if (ulMoveS <= 0)
    {
      btOperatorT = true;
    }
  }

  u8g2.drawLine(64 - ulMoveS, 52, 64 + ulMoveS, 52);

}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::footer(void)
{
  String clStringLeftT;
  String clStringRightT;
  static int32_t slDelayTimeT = ((LCD_FOOTER_TOGGLE_TIME >> 1) / LCD_REFRESH_TIME);

  //---------------------------------------------------------------------------------------------------
  // show the info for the footer that should be shown alternately
  //
  if (slDelayTimeT > 0)
  {
    clStringLeftT = clWifiInfoP.clSsid;

    clStringRightT = getRSSIasQuality(clWifiRssiP);
    clStringRightT += String(" %");
    slDelayTimeT--;
  }
  else 
  {
    clStringLeftT = clWifiInfoP.clIp.toString().c_str();

    clStringRightT = String(clWifiRssiP);
    clStringRightT += String(" dBm");

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
  u8g2.setCursor(0, 56);
  u8g2.print(clStringLeftT);

  //---------------------------------------------------------------------------------------------------
  // print the right side of line
  //
  u8g2.setCursor(128 - u8g2.getStrWidth(clStringRightT.c_str()), 56);
  u8g2.print(clStringRightT);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzLcd::getRSSIasQuality(int32_t slRssiV)
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
void PvzLcd::header(void)
{
  String clStringT;
  String clTimeT;
  int32_t slSplitT;
  static int32_t slDelayTimeT = ((LCD_HEADER_TOGGLE_TIME >> 1) / LCD_REFRESH_TIME);

  //--------------------------------------------------------------------------------------------------- 
  // Check the time has been updated
  //
  if (clTimeP != "")
  {
    //------------------------------------------------------------------------------------------- 
    // show the info for the footer that should be shown alternately
    //
    if (slDelayTimeT > 0)
    {
      //----------------------------------------------------------------------------------- 
      // format date value to 12.02.24
      //
      clTimeT = clTimeP; // get time in format 2018-05-28T16:00:13Z
      clStringT = clTimeT.substring(8, 10);
      clStringT += ".";
      clStringT += clTimeT.substring(5, 7);
      clStringT += ".";
      clStringT += clTimeT.substring(2, 4);
      slDelayTimeT--;
    }
    else
    {
      //----------------------------------------------------------------------------------- 
      // format time value to 14:15
      //
      clTimeT = clTimeP; // get time in format 2018-05-28T16:00:13Z
      slSplitT = clTimeT.indexOf("T");
      clStringT = clTimeT.substring(slSplitT + 1, clTimeT.length() - 3);
      slDelayTimeT--;
    }
  }

  //---------------------------------------------------------------------------------------------------
  // if time value is not available than display version number.
  //
  else
  {
    clStringT = "v";
    clStringT += clVersionP;
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
  // print the right side of line
  //
  u8g2.setCursor(128 - u8g2.getStrWidth(clStringT.c_str()), 0);
  u8g2.print(clStringT);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::init(String clFwVersionV)
{
  clTimeP = "";
  clVersionP = clFwVersionV;

  clWifiRssiP = 0;
  clWifiInfoP.clIp = 0x0100A8C0;
  clWifiInfoP.clSsid = "Unknown";

  setScreen(NULL, 0);

  u8g2.begin();
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void PvzLcd::middle(void)
{
  String clStringT;
  static int32_t slDelayTimeS = (LCD_MIDDLE_TOGGLE_TIME / LCD_REFRESH_TIME);
  static int32_t slScreenNumberS = 0;
  int32_t slStartT;
  int32_t slStopT;

  //---------------------------------------------------------------------------------------------------
  // show all screens in the middle alternatively
  //
  if (pclScreenP != NULL)
  {
    // u8g2.setCursor(16, 55);
    // u8g2.print("B");

    if (slDelayTimeS > 0)
    {
      slDelayTimeS--;

      u8g2.setCursor(0, 16);
      u8g2.print(pclScreenP[slScreenNumberS].aclLine[0]);

      u8g2.setCursor(0, 16 + 12);
      u8g2.print(pclScreenP[slScreenNumberS].aclLine[1]);

      u8g2.setCursor(0, 16 + 12 + 12);
      u8g2.print(pclScreenP[slScreenNumberS].aclLine[2]);

      // display screen number as line 
      // calculate start and stop of line depending on screen number
      // length of a line : 128 / slScreenCountP
      // start of a line : slScreenNumber
      slStartT = (128 / slScreenCountP) * slScreenNumberS;
      slStopT = slStartT + (128 / slScreenCountP);
      u8g2.drawLine(slStartT, 52, slStopT, 52);
      u8g2.drawLine(0, 53, 128, 53);
    }

    // switch to next screen that will be display at next cycle
    if (slDelayTimeS == 0)
    {
      slDelayTimeS = (LCD_MIDDLE_TOGGLE_TIME / LCD_REFRESH_TIME);
      slScreenNumberS++;
      if (slScreenNumberS >= slScreenCountP)
      {
        slScreenNumberS = 0;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::ok()
{
  slPendingScreenP = eLCD_OK;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::process(void)
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
    // display middle
    //
    if (slPendingScreenP == eLCD_Busy)
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
      u8g2.setCursor(0, 16);
      u8g2.print(aclWarnLinesP[0]);
      u8g2.setCursor(0, 16 + 12);
      u8g2.print(aclWarnLinesP[1]);
      u8g2.setCursor(0, 16 + 12 + 12);
      u8g2.print(aclWarnLinesP[2]);
    } 

    else 
    {
      middle();
    }


    //-----------------------------------------------------------------------------------
    // footer is only printed for WiFi info, so display it only if WiFi RSSI is != 0
    //
    if (clWifiRssiP != 0)
    {
      footer();
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
void PvzLcd::setScreen(Screen_ts *pclScreenV, int32_t slCountV)
{
  if (pclScreenV != NULL)
  {
    pclScreenP = pclScreenV;
    slScreenCountP = slCountV;
  }
  else
  {
    pclScreenP = NULL;
    slScreenCountP = 0;
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::updateTime(String clTimeV)
{
  clTimeP = clTimeV;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::updateWifiInfo(WifiConfig_ts *ptsInfoV)
{
  //--------------------------------------------------------------------------------------------------- 
  // copy the content to local variables
  // 
  clWifiInfoP.clIp = ptsInfoV->clIp;
  clWifiInfoP.clSsid = ptsInfoV->clSsid;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::updateWifiRssi(int32_t ulValueV)
{
  //--------------------------------------------------------------------------------------------------- 
  // copy the content to local variables
  // 
  clWifiRssiP = ulValueV;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzLcd::warning(String clLine1V, String clLine2V, String clLine3V)
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
void PvzLcd::warnScreen(void)
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
