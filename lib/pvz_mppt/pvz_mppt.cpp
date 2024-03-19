//====================================================================================================================//
// File:          pvz_mppt.cpp                                                                                        //
// Description:   PhotoVoltaics Zero - MPPT (Maximum Power Point Tracker)                                             //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include "pvz_mppt.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzMppt::PvzMppt()
{
  pclMpptP = NULL;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzMppt::~PvzMppt()
{
}

static void mpptCallback(uint16_t id, int32_t value)
{
  // if (id == VEDirect_kPanelVoltage)
  // {
  //   ftPanelVoltageP = value;
  //   // Serial.print(F("Vpv : "));
  //   // Serial.println(value * 0.01);
  // }
  // if (id == VEDirect_kChargeVoltage)
  // {
  //   ftBatteryVoltageP = value;
  //   // Serial.print(F("Vpv : "));
  //   // Serial.println(value * 0.01);
  // }
  // if (id == VEDirect_kChargeCurrent)
  // {
  //   ftBatteryCurrentP = value;
  //   // Serial.print(F("Ich : "));
  //   // Serial.println(value * 0.1);
  // }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzMppt::init(HardwareSerial &clSerialR)
{
  //---------------------------------------------------------------------------------------------------
  // init serial interface and PSU
  //
  // slModelNumberP = clPsuP.begin(clSerialR, 1);
  pclMpptP = new VEDirect(Serial2, mpptCallback);

  return 0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzMppt::process(bool btForceV)
{
  static unsigned long ulOldTimeS;
  static uint32_t ulRefreshTimeT;

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
  if (ulRefreshTimeT > MPPT_REFRESH_TIME)
  {
    ulRefreshTimeT = 0;

    if (pclMpptP != NULL)
    {
      pclMpptP->ping(); // send ping every MPPT_REFRESH_TIME
    }
  }
}