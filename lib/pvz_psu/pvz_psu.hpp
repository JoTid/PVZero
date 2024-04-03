//====================================================================================================================//
// File:          pvz_psu.hpp                                                                                         //
// Description:   PhotoVoltaics Zero - PSU                                                                            //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

#ifndef PVZ_PSU_HPP
#define PVZ_PSU_HPP

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include <Arduino.h>
#include <mutex>
#include "DPM8600.h"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and Enums                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief
 *
 */
class PvzPsu
{

public:
  PvzPsu(void);
  ~PvzPsu(void);

  /**
   * @brief Initialisation of PSU
   * This method should be called only once while setup.
   */
  int32_t init(HardwareSerial &clSerialR);
  int32_t enable(bool);

  int32_t read();
  int32_t write();

  int32_t set(float ftVoltageV, float ftCurrentV);

  float actualVoltage();

  float actualCurrent();

  bool isEnabled();

  bool isAvailable();

  float targetVoltage();

  float targetCurrent();

  int32_t model();

private:
  int32_t slModelNumberP;
  DPM8600 clPsuP;

  int32_t slReadCurrenTriggerP;
  int32_t slReadVoltageTriggerP;

  float ftActualVoltageP;
  float ftActualCurrentP;
  float ftActualTemperatureP;
  bool btConstantCurrentOutputP;

  float ftTargetVoltageP;
  float ftTargetCurrentP;

  bool btIsEnabledP;

  std::mutex uartMutexP;
};

#endif // PVZ_PSU_HPP
