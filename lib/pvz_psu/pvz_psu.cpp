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
#include "pvz_psu.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzPsu::PvzPsu()
{
  slModelNumberP = -1; // not initialised
  ftActualVoltageP = 0.0;
  ftActualCurrentP = 0.0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzPsu::~PvzPsu()
{
}

int32_t PvzPsu::enable(bool btEnableV)
{
  int32_t slReturnT = slModelNumberP;

  if (slModelNumberP > 0)
  {
    slReturnT = clPsuP.power(btEnableV);
  }

  return slReturnT;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::init(HardwareSerial &clSerialR)
{
  //---------------------------------------------------------------------------------------------------
  // init serial interface and PSU
  //
  clSerialR.begin(115200);
  slModelNumberP = clPsuP.begin(clSerialR, 1);

  //---------------------------------------------------------------------------------------------------
  // read the model number of PSU, or negative value in case of an error
  //
  if (slModelNumberP > 0)
  {
    clPsuP.power(false);
  }

  return slModelNumberP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzPsu::process(bool btForceV)
{
  static unsigned long ulOldTimeS;
  static uint32_t ulRefreshTimeT;

  //---------------------------------------------------------------------------------------------------
  // Model number is negative in case of failure at initialisation.
  // In other case it contain the maximal A value (5, 8, 16 or 24)
  //
  if ((slModelNumberP < 0) && (btForceV == false))
  {
    return;
  }

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
  // refresh the PSU only within define time
  //
  if (ulRefreshTimeT > PSU_REFRESH_TIME)
  {
    ulRefreshTimeT = 0;

    //-------------------------------------------------------------------------------------------
    // simple poll actual values from PSU
    //
    if (clPsuP.read('p') > 0)
    {
      btIsEnabledP = true;
    }
    else
    {
      btIsEnabledP = false;
    }

    ftActualVoltageP = clPsuP.read('v');
    ftActualCurrentP = clPsuP.read('c');
    ftActualTemperatureP = clPsuP.read('t');
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::set(float ftVoltageV, float ftCurrentV)
{
  int32_t slReturnT = slModelNumberP;

  if (slModelNumberP > 0)
  {
    slReturnT = clPsuP.writeVC(ftVoltageV, ftCurrentV);
  }

  return slReturnT;
}
