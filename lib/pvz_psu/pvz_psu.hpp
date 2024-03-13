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

  /**
   * @brief Process the PSU operation.
   * This method must be called from main loop.
   */
  void process(bool btForceV = false);

  int32_t set(float ftVoltageV, float ftCurrentV);
  int32_t enable(bool);
  float actualVoltage()
  {
    return ftActualVoltageP;
  }

  float actualCurrent()
  {
    return ftActualCurrentP;
  }

  bool isEnabled()
  {
    return btIsEnabledP;
  }

  bool isAvailable()
  {
    if (slModelNumberP > 0)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  int32_t model() { return slModelNumberP; }

private:
#define PSU_REFRESH_TIME 1000

  int32_t slModelNumberP;
  DPM8600 clPsuP;

  float ftActualVoltageP;
  float ftActualCurrentP;
  float ftActualTemperatureP;
  bool btIsEnabledP;
};

#endif // PVZ_PSU_HPP
