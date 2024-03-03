//====================================================================================================================//
// File:          battery_guard.cpp                                                                                   //
// Description:   PhotoVoltaics Zero - Battery guard                                                                  //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

/*--------------------------------------------------------------------------------------------------------------------*\
** Include Files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include "Arduino.h"
#include "battery_guard.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
BatteryGuard::BatteryGuard()
{
  btDischargeAlarmP = true;
  btEnabledP = true;
  slActualVoltageP = 0.0;
  slMinVoltageP = 0.0;
  slMaxVoltageP = 0.0;
  ulAlarmPendingTimeP = 0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
BatteryGuard::~BatteryGuard()
{
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void BatteryGuard::init(float ftMinimalOperatingVoltageV, uint32_t ulMinimumChargingTimeV)
{
  btDischargeAlarmP = false;
  slActualVoltageP = 0.0;
  slMinVoltageP = BG_CHARGE_CUTOFF_VOLTAGE;
  slMaxVoltageP = BG_DISCHARGE_VOLTAGE;

  //---------------------------------------------------------------------------------------------------
  // take operating value and limit it to plausible range
  //
  slRecoverVoltageP = (int32_t)(ftMinimalOperatingVoltageV * 10);
  if ((slRecoverVoltageP <= BG_DISCHARGE_VOLTAGE) || (slRecoverVoltageP >= BG_CHARGE_CUTOFF_VOLTAGE))
  {
    slRecoverVoltageP = BG_ABSORPTION_VOLTAGE;
  }

  //---------------------------------------------------------------------------------------------------
  // do not check time value, as it depends on application
  //
  ulMinimumChargingTimeP = ulMinimumChargingTimeV;

  // assume that the battery is charged
  ulChargingTimeP = 0;
  ulAlarmPendingTimeP = 0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void BatteryGuard::process(void)
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
  // process the guarding in the defined time interval
  //
  if (ulRefreshTimeT > BG_REFRESH_TIME)
  {
    ulRefreshTimeT = 0;

    //---------------------------------------------------------------------------------------------------
    // update min & max values
    //
    if (slActualVoltageP > slMaxVoltageP)
    {
      slMaxVoltageP = slActualVoltageP;
    }

    if (slActualVoltageP < slMinVoltageP)
    {
      slMinVoltageP = slActualVoltageP;
    }

    //-------------------------------------------------------------------------------------------
    // in case we want disable guarding for application
    //
    if (btEnabledP == true)
    {
      //---------------------------------------------------------------------------------------------------
      // check normal operation when all is OK and battery voltage is > BG_DISCHARGE_VOLTAGE
      //
      if (btDischargeAlarmP == false)
      {
        //-------------------------------------------------------------------------------------------
        // check for battery limits
        //
        if (slActualVoltageP < BG_DISCHARGE_VOLTAGE)
        {
          btDischargeAlarmP = true;
          ulChargingTimeP = ulMinimumChargingTimeP;
        }
      }

      //-------------------------------------------------------------------------------------------
      // alarm is pending
      //
      else
      {
        ulAlarmPendingTimeP++;

        //-----------------------------------------------------------------------------------
        //
        //
        if (slActualVoltageP >= slRecoverVoltageP)
        {
          if (ulChargingTimeP > 0)
          {
            ulChargingTimeP--;
          }
          else
          {
            //-------------------------------------------------------------------
            // Now is time to recover
            // The required voltage was applied for the defined time.
            //
            btDischargeAlarmP = false;
            ulAlarmPendingTimeP = 0;
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void BatteryGuard::updateVoltage(float ftVoltageV)
{
  slActualVoltageP = (int32_t)(ftVoltageV * 10.0);
}
