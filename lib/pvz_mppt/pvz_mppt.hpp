//====================================================================================================================//
// File:          pvz_mppt.hpp                                                                                        //
// Description:   PhotoVoltaics Zero - MPPT (Maximum Power Point Tracker)                                             //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

#ifndef PVZ_MPPT_HPP
#define PVZ_MPPT_HPP

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include <Arduino.h>
#include <mutex>
// #include "VEDirect.h"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and Enums                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

// static void mpptCallback(uint16_t id, int32_t value);

// std::mutex mppt_mutex;

#define MPPT_TEXT_FRAME_LENGTH 256
/**
 * @brief
 *
 */
class PvzMppt
{

public:
  PvzMppt();
  ~PvzMppt();

  /**
   * @brief Initialisation of PSU
   * This method should be called only once while setup.
   */
  int32_t init(HardwareSerial &clSerialR);

  /**
   * @brief Process the PSU operation.
   * This method must be called from main loop.
   */
  void process(bool btForceV = false);

  int32_t set(float ftVoltageV, float ftCurrentV);

  void updateFrame(const char *pszFrameV, int32_t slByteNumberV);
  float batteryVoltage() { return ftBatteryVoltageP; }
  float batteryCurrent() { return ftBatteryCurrentP; }

private:
  char aszFrameP[MPPT_TEXT_FRAME_LENGTH];
  int32_t slByteNumberP;
  bool btNewFrameP;
  int32_t slNumberOfParsedValuesP;
  void parseTable(char *tableData);

  float ftBatteryVoltageP;
  float ftBatteryCurrentP;
  // #define MPPT_REFRESH_TIME 1000

  // VEDirect *pclMpptP;

  // static float ftPanelVoltageP;
  // static float ftBatteryVoltageP;
  // static float ftBatteryCurrentP;
};

#endif // PVZ_MPPT_HPP
