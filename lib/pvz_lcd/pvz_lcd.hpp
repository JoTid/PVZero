//====================================================================================================================//
// File:          pvz_lcd.hpp                                                                                         //
// Description:   PhotoVoltaics Zero - LCD                                                                            //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//


#ifndef PVZ_LCD_HPP
#define PVZ_LCD_HPP


/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/ 
#include <Arduino.h>


/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and Enums                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief 
 * 
 */
class PvzLcd
{

public:
  PvzLcd(void);
  ~PvzLcd(void);

  typedef struct WifiConfig_s{
    String clSsid;
    IPAddress clIp;
  } WifiConfig_ts;

  typedef struct Screen_s{
    String aclLine[3];
  } Screen_ts;

  /**
   * @brief Initialisation of LCD
   * This method should be called only once while setup.
   */
  void init(String clFwVersionV);

  /**
   * @brief Process the LCD operation.
   * This method must be called from main loop.
   */
  void process(void);

  /**
   * @brief Set LCD in to normal operating mode
   */
  void ok(void);

  /**
   * @brief Set LCD to busy mode and provide reason and value to display
   *
   * @param clReasonV
   * @param clValueV
   */
  void busy(String clReasonV, String clValueV);

  /**
   * @brief Set LCD to warning mode and provide 3 lines with information
   *
   * @param clLine1V
   * @param clLine2V
   * @param clLine3V
   */
  void warning(String clLine1V = "", String clLine2V = "", String clLine3V = "");

  void updateTime(String clTimeV);
  void updateWifiRssi(int32_t ulValueV);
  void updateWifiInfo(WifiConfig_ts *ptsInfoV);

  void setScreen(Screen_ts *pclScreenV, int32_t slCountV=1);

private:
  /**
   * @brief
   *
   */
  typedef enum lcd_e
  {
    eLCD_OK = 0,
    eLCD_Busy,
    eLCD_Warning
  } lcd_te;

  /**
   * @brief This variable hold the pending screen, defined by \c #lcd_e enumeration.
   */
  int32_t slPendingScreenP;

  String clBusyReasonP;
  String clBusyValueP;

  String aclWarnLinesP[3];

  String clTimeP;
  String clVersionP;

  int32_t clWifiRssiP;
  WifiConfig_ts clWifiInfoP;

  Screen_ts *pclScreenP;
  int32_t    slScreenCountP;

/**
 * @brief Refresh time of LCD given in milliseconds
 */
#define LCD_REFRESH_TIME 100

/**
 * @brief Time for toggle information in the footer line, given in milliseconds
 */
#define LCD_FOOTER_TOGGLE_TIME 5000

/**
 * @brief Time for toggle information in the header line, given in milliseconds
 */
#define LCD_HEADER_TOGGLE_TIME 10000

/**
 * @brief Time for toggle information in the middle area, given in milliseconds
 */
#define LCD_MIDDLE_TOGGLE_TIME 5000

/**
 * @brief Time for toggle warning icon, given in milliseconds
 */
#define LCD_WARNING_TOGGLE_TIME 500

  /**
   * @brief show and animate the busy screen
   * This function considers the \c #LCD_REFRESH_TIME and is called from \c #process() method.
   */
  void busyScreen(void);

  /**
   * @brief show and animate the warn screen
   * This function considers the \c #LCD_REFRESH_TIME and is called from \c #process() method.
   */
  void warnScreen(void);

  /**
   * @brief draw the header with all infos and animations
   */
  void header(void);

  /**
   * @brief draw the footer with all infos and animations
   */
  void footer(void);

  void middle(void);

  int32_t getRSSIasQuality(int32_t slRssiV);
};

#endif // PVZ_LCD_HPP
