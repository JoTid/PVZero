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
  // prepare parameters for scaling: y = m * x + b
  //

  // setup minimal voltage value, provided by user
  ftNominalVoltageMinP = ftMinimalOperatingVoltageV;

  // setup maximal voltage value, provided by battery
  ftNominalVoltageMaxP = BG_CHARGE_CUTOFF_VOLTAGE; // 58.40 V
  ftNominalVoltageMaxP /= 10.0;

  // setup minimal current that corresponds to the minimal voltage value
  ftNominalCurrentMinP = 0.01; //  0.01 A
  // setup maximal current that corresponds to the maximal voltage value
  ftNominalCurrentMaxP = 12.0; // 12.00 A

  // m = (y2-y1) / (x2-x1)
  ftScaleGainP = (ftNominalCurrentMaxP - ftNominalCurrentMinP) / (ftNominalVoltageMaxP - ftNominalVoltageMinP);

  // b = y1 - (m * x1)
  ftScaleOffsetP = (ftNominalCurrentMinP - (ftScaleGainP * ftNominalVoltageMinP));

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
  ftActualVoltageP = ftVoltageV;
  slActualVoltageP = (int32_t)(ftActualVoltageP * 10.0);
}
#include <unity.h>
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float BatteryGuard::limitedCurrent(float ftFeedTargetInCurrentV)
{
  float ftLimitedCurrentT;
  int32_t slLimitedCurrentT;
  int32_t slFeedInCurrent = (int32_t)(ftFeedTargetInCurrentV * 100);

  //---------------------------------------------------------------------------------------------------
  // calculate the current limit depending on the actual voltage
  //
  ftLimitedCurrentT = ftScaleGainP * ftActualVoltageP;
  ftLimitedCurrentT += ftScaleOffsetP;
  slLimitedCurrentT = (int32_t)(ftLimitedCurrentT * 100);

  //---------------------------------------------------------------------------------------------------
  // avoid negative values
  //
  if (slLimitedCurrentT < 0)
  {
    slLimitedCurrentT = 0;
    ftLimitedCurrentT = 0.0;
  }

  //---------------------------------------------------------------------------------------------------
  // limit the provide current if required
  //
  if (slFeedInCurrent > slLimitedCurrentT)
  {
    return ftLimitedCurrentT;
  }

  //---------------------------------------------------------------------------------------------------
  // feed in current is within valid range do not change it
  //
  return ftFeedTargetInCurrentV;
}
