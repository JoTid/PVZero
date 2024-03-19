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
  slBatteryVoltageP = 0.0;
  slBatteryVoltageMinimalP = 0.0;
  slBatteryVoltageMaximalP = 0.0;
  ulAlarmPendingTimeP = 0;

  pfnEventHandlerP = (EventHandler_fn)NULL;
  pfnSaveTimeHandlerP = (SaveTimeHandler_fn)NULL;
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
void BatteryGuard::init(uint64_t uqTimeV, SaveTimeHandler_fn pfnSaveTimeHandlerV)
{

  pfnSaveTimeHandlerP = pfnSaveTimeHandlerV;
  // btDischargeAlarmP = false;
  // slBatteryVoltageP = 0.0;
  // slBatteryVoltageMinimalP = BG_CHARGE_CUTOFF_VOLTAGE;
  // slBatteryVoltageMaximalP = BG_DISCHARGE_VOLTAGE;

  // //---------------------------------------------------------------------------------------------------
  // // prepare parameters for scaling: y = m * x + b
  // //

  // // setup minimal voltage value, provided by user
  // ftNominalVoltageMinP = ftMinimalOperatingVoltageV;

  // // setup maximal voltage value, provided by battery
  // ftNominalVoltageMaxP = BG_CHARGE_CUTOFF_VOLTAGE; // 58.40 V
  // ftNominalVoltageMaxP /= 10.0;

  // // setup minimal current that corresponds to the minimal voltage value
  // ftNominalCurrentMinP = 0.01; //  0.01 A
  // // setup maximal current that corresponds to the maximal voltage value
  // ftNominalCurrentMaxP = 12.0; // 12.00 A

  // // m = (y2-y1) / (x2-x1)
  // ftScaleGainP = (ftNominalCurrentMaxP - ftNominalCurrentMinP) / (ftNominalVoltageMaxP - ftNominalVoltageMinP);

  // // b = y1 - (m * x1)
  // ftScaleOffsetP = (ftNominalCurrentMinP - (ftScaleGainP * ftNominalVoltageMinP));

  // //---------------------------------------------------------------------------------------------------
  // // take operating value and limit it to plausible range
  // //
  // slRecoverVoltageP = (int32_t)(ftMinimalOperatingVoltageV * 10);
  // if ((slRecoverVoltageP <= BG_DISCHARGE_VOLTAGE) || (slRecoverVoltageP >= BG_CHARGE_CUTOFF_VOLTAGE))
  // {
  //   slRecoverVoltageP = BG_ABSORPTION_VOLTAGE;
  // }

  // //---------------------------------------------------------------------------------------------------
  // // do not check time value, as it depends on application
  // //
  // ulMinimumChargingTimeP = ulMinimumChargingTimeV;

  // // assume that the battery is charged
  // ulChargingTimeP = 0;
  // ulAlarmPendingTimeP = 0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float BatteryGuard::limitedCurrent(float ftTargetCurrentV)
{
  float ftLimitCurrentT = ftTargetCurrentV;
  int32_t slTargeCurrentT = (int32_t)(ftTargetCurrentV * 100);

  //---------------------------------------------------------------------------------------------------
  // determine the limited value depending on the actual state
  //
  switch (teStateP)
  {
  case eCharging:
    //-------------------------------------------------------------------------------------------
    // The current is limited to the Battery Current value of the Victron SmartSolar
    //
    if (slTargeCurrentT > slBatteryCurrentP)
    {
      ftLimitCurrentT = (float)(slBatteryCurrentP);
      ftLimitCurrentT *= 0.01;
    }

    break;

  case eDischarged:
    //-------------------------------------------------------------------------------------------
    // The current is limited to the value 0.0 A, feed-in is stopped
    //
    ftLimitCurrentT = 0.0;

    break;

  default:
    break;
  }

  return ftLimitCurrentT;
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

    //-------------------------------------------------------------------------------------------
    // update min & max values for voltage
    //
    if (slBatteryVoltageP > slBatteryVoltageMaximalP)
    {
      slBatteryVoltageMaximalP = slBatteryVoltageP;
    }

    if (slBatteryVoltageP < slBatteryVoltageMinimalP)
    {
      slBatteryVoltageMinimalP = slBatteryVoltageP;
    }

    //-------------------------------------------------------------------------------------------
    // update min & max values for current
    //
    if (slBatteryCurrentP > slBatteryCurrentMaximalP)
    {
      slBatteryCurrentMaximalP = slBatteryCurrentP;
    }

    if (slBatteryCurrentP < slBatteryCurrentMinimalP)
    {
      slBatteryCurrentMinimalP = slBatteryCurrentP;
    }

    //-------------------------------------------------------------------------------------------
    // in case we want disable guarding for application
    //
    if (btEnabledP == true)
    {

      //-----------------------------------------------------------------------------------
      // handle state machine transitions
      //
      switch (teStateP)
      {
        //---------------------------------------------------------------------------
        // The current is limited to the Charging Current value of the Victron SmartSolar
        //
      case eCharging:
        //---------------------------------------------------------------------------
        // Battery Voltage >= CHARGE_CUTOFF_VOLTAGE (58.4 V)
        //
        if (slBatteryVoltageP >= BG_CHARGE_CUTOFF_VOLTAGE)
        {
          teStateP = eCharged;
        }
        //---------------------------------------------------------------------------
        // (**Battery Current** == 0.0 A) && (Zeitstempel < 2 Wochen)
        //
        else if (slBatteryCurrentP == 0)
        {
          //-------------------------------------------------------------------
          // calculate time difference to perform full charge each two weeks
          // it is treu when the time difference is < 2 weeks
          //
          if ((uqTimeP - uqFullyChargedTimeP) < (uint64_t)BG_FULL_CHARGE_REPETITION_TIME)
          {
            teStateP = eDischarging;
          }
        }

        //---------------------------------------------------------------------------
        //---------------------------------------------------------------------------
        // stay in this state
        //
        // limitation of the current is performed at request within limitedCurrent()
        //
        break;

        //---------------------------------------------------------------------------
        // The current is not limited, time stamp is saved
        //
      case eCharged:
        //---------------------------------------------------------------------------
        // save time when the battery has been fully charged
        //
        if (pfnSaveTimeHandlerP != nullptr)
        {
          pfnSaveTimeHandlerP(uqTimeP);
        }

        //---------------------------------------------------------------------------
        // Check condition for one possible transition to discharging:
        // Battery Voltage < CHARGE_CUTOFF_VOLTAGE (58.4 V)
        //
        if (slBatteryVoltageP < BG_CHARGE_CUTOFF_VOLTAGE)
        {
          teStateP = eDischarging;
        }

        //---------------------------------------------------------------------------
        //---------------------------------------------------------------------------
        // stay in this state
        //
        // limitation of the current is performed at request within limitedCurrent()
        //
        break;

        //---------------------------------------------------------------------------
        // The current is not limited and no other
        //
      case eDischarging:

        //---------------------------------------------------------------------------
        // Check condition for possible transition to discharged:
        // Battery Voltage <= DISCHARGE_VOLTAGE (40.0 V)
        //
        if (slBatteryVoltageP <= BG_DISCHARGE_VOLTAGE)
        {
          teStateP = eDischarged;
        }

        //---------------------------------------------------------------------------
        // Battery Current > 0.0 A
        //
        else if (slBatteryCurrentP > 0)
        {
          teStateP = eCharging;
        }

        //---------------------------------------------------------------------------
        //---------------------------------------------------------------------------
        // stay in this state
        //
        // limitation of the current is performed at request within limitedCurrent()
        //
        break;

        //---------------------------------------------------------------------------
        // The current is limited to the value 0.0 A, feed-in is stopped
        //
      case eDischarged:

        //---------------------------------------------------------------------------
        // Check condition for possible transition to charging:
        // Battery Voltage > DISCHARGE_VOLTAGE (40.0 V)
        //
        if (slBatteryVoltageP > BG_DISCHARGE_VOLTAGE)
        {
          teStateP = eCharging;
        }

        //---------------------------------------------------------------------------
        //---------------------------------------------------------------------------
        // stay in this state
        //
        // limitation of the current is performed at request within limitedCurrent()
        //
        break;

        //---------------------------------------------------------------------------
        // should never run in here
        //
      default:
        break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void BatteryGuard::updateCurrent(float ftCurrentV)
{
  slBatteryCurrentP = (int32_t)(ftCurrentV * 10.0); // for fast comparison
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void BatteryGuard::updateVoltage(float ftVoltageV)
{
  ftBatteryVoltageP = ftVoltageV;
  slBatteryVoltageP = (int32_t)(ftBatteryVoltageP * 10.0); // for fast comparison
}

// //--------------------------------------------------------------------------------------------------------------------//
// //                                                                                                                    //
// //                                                                                                                    //
// //--------------------------------------------------------------------------------------------------------------------//
// float BatteryGuard::limitedCurrent(float ftFeedTargetInCurrentV)
// {
//   float ftLimitedCurrentT;
//   int32_t slLimitedCurrentT;
//   int32_t slFeedInCurrent = (int32_t)(ftFeedTargetInCurrentV * 100);

//   //---------------------------------------------------------------------------------------------------
//   // consider the limit calculation only if battery guard is enabled
//   //
//   if (btEnabledP == true)
//   {
//     //-------------------------------------------------------------------------------------------
//     // calculate the current limit depending on the actual voltage
//     //
//     ftLimitedCurrentT = ftScaleGainP * ftBatteryVoltageP;
//     ftLimitedCurrentT += ftScaleOffsetP;
//     slLimitedCurrentT = (int32_t)(ftLimitedCurrentT * 100);

//     //-------------------------------------------------------------------------------------------
//     // avoid negative values
//     //
//     if (slLimitedCurrentT < 10)
//     {
//       slLimitedCurrentT = 10;
//       ftLimitedCurrentT = 0.10;
//     }

//     //-------------------------------------------------------------------------------------------
//     // limit the provide current if required
//     //
//     if (slFeedInCurrent > slLimitedCurrentT)
//     {
//       return ftLimitedCurrentT;
//     }
//   }

//   //---------------------------------------------------------------------------------------------------
//   // feed in current is within valid range do not change it
//   //
//   return ftFeedTargetInCurrentV;
// }
