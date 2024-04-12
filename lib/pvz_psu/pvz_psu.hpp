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
#include "DPM86xx.h"

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
  int32_t writeCurrent();
  int32_t writeVoltage();

  void set(float ftVoltageV, float ftCurrentV);

  float actualVoltage();

  float actualCurrent();

  bool isEnabled();

  bool isAvailable();

  float targetVoltage();

  float targetCurrent();

  int32_t model();

private:
  int32_t slModelNumberP;
  DPM86xx clPsuP;

  // values for current and voltage are taken in account only at corresponding constant phase of PSU
  // the slReadCurrenTriggerP and slReadVoltageTriggerP variables are used to take values after
  // given number of tries although when the corresponding constant phase was not available.
  int32_t slReadCurrenTriggerP;
  int32_t slReadVoltageTriggerP;

  float ftActualVoltageP;
  int32_t slActualVoltageP;
  int32_t slActualVoltageIgnoreCounterP;
  float ftActualCurrentP;
  int32_t slActualCurrentP;
  int32_t slActualCurrentIgnoreCounterP;
  float ftActualTemperatureP;
  bool btConstantCurrentOutputP;

  float ftTargetVoltageP;
  float ftTargetCurrentP;

  bool btIsEnabledP;

  std::mutex uartMutexP;
};

#endif // PVZ_PSU_HPP
