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

#include <unity.h>

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
  btEnabledP = true;
  slBatteryVoltageP = 0.0;
  slBatteryVoltageMinimalP = 0.0;
  slBatteryVoltageMaximalP = 0.0;

  slBatteryCurrentP = 0.0;
  slBatteryCurrentMinimalP = 0.0;
  slBatteryCurrentMaximalP = 0.0;

  clAddStateInfoP = "";

  pfnEventHandlerP = nullptr;
  pfnSaveTimeHandlerP = nullptr;

  teStateP = eDischarged;
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
  //---------------------------------------------------------------------------------------------------
  // take parameters
  //
  pfnSaveTimeHandlerP = pfnSaveTimeHandlerV;
  uqTimeP = uqTimeV;
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
  case eCharge:
    //-------------------------------------------------------------------------------------------
    // The current is limited to the Battery Current value of the Victron SmartSolar
    // nur wenn die Spannung einen gewissen wert unterschirtten hat. Im anderen Fall wird der Akku schon den
    // sollwert abfangen.
    //
    if (slTargeCurrentT > slBatteryCurrentP)
    {
      ftLimitCurrentT = (float)(slBatteryCurrentP);
      ftLimitCurrentT *= 0.01;
    }

    break;

  case eChargeUntilCharged:
  case eDischarged:
    //-------------------------------------------------------------------------------------------
    // The current is limited to the value 0.0 A, feed-in is stopped
    //
    ftLimitCurrentT = 0.0;

    break;

  case eChargeAndDischarge:
  case eCharged:
  case eDischarge:
  default:
    break;
  }

  return ftLimitCurrentT;
}

void BatteryGuard::installEventHandler(EventHandler_fn pfnEventHandlerV)
{
  pfnEventHandlerP = pfnEventHandlerV;
  if (pfnEventHandlerP != nullptr)
  {
    pfnEventHandlerP(teStateP);
  }
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
        // Grund für diese Zustand: Der Laderegler, regelt immer wieder den Strom gegen 0, was dazu führt, dass
        // der Wechselrichter getrennt wird und sich wieder auf Netz synchronisieren muss.
        // Dies kann bis zu einer Minute dauern, dass soll nicht so sein.
        //
      case eChargeAndDischarge:

        //---------------------------------------------------------------------------
        // 8. Charge Voltage <= ABSORPTION_VOLTAGE (51.2)
        //
        if (slBatteryVoltageP <= BG_ABSORPTION_VOLTAGE)
        {
          teStateP = eCharge;

          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // 1. Charge Voltage >= CHARGE_CUTOFF_VOLTAGE (58.4 V)
        //
        else if (slBatteryVoltageP >= BG_CHARGE_CUTOFF_VOLTAGE)
        {
          teStateP = eCharged;

          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // stay in this state
        //
        else
        {
          clAddStateInfoP = String("Der Strom wird nicht begrenzt, solange die Bedingung 1. oder 8. nicht erfüllt ist");
        }
        break;

        //---------------------------------------------------------------------------
        // The current is limited to the Charging Current value of the Victron SmartSolar
        //
      case eCharge:

        //---------------------------------------------------------------------------
        // 7. Charge Voltage >= (CHARGE_CUTOFF_VOLTAGE - 5.0 V)
        //
        if (slBatteryVoltageP >= (BG_CHARGE_CUTOFF_VOLTAGE - 50))
        {
          teStateP = eChargeAndDischarge;

          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // 9. (Zeitstempel - SavedZeitstempel) > 2 Wochen
        //
        else if ((uqTimeP - uqFullyChargedTimeP) > (uint64_t)BG_FULL_CHARGE_REPETITION_TIME)
        {
          teStateP = eChargeUntilCharged;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // 5. Charge Current < 0.2 A
        //
        else if (slBatteryCurrentP < 20)
        {
          teStateP = eDischarge;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // stay in this state
        //
        else
        {
          clAddStateInfoP = String("Der Strom wird auf den Wert Charge Current vom MPPT begrenzt, solange die Bedingung 5. oder 7. oder 9. nicht erfüllt ist");
        }

        break;

        //---------------------------------------------------------------------------
        // The current is not limited, time stamp is saved
        //
      case eCharged:

        //---------------------------------------------------------------------------
        // while we are charged, update the time
        //
        uqFullyChargedTimeP = uqTimeP;

        //---------------------------------------------------------------------------
        // 2. Charge Voltage < (CHARGE_CUTOFF_VOLTAGE - 0.4V) (58.0 V)
        //
        if (slBatteryVoltageP < (BG_CHARGE_CUTOFF_VOLTAGE - 4))
        {
          //---------------------------------------------------------------------------
          // save time when the battery has been fully charged
          //
          if (pfnSaveTimeHandlerP != nullptr)
          {
            pfnSaveTimeHandlerP(uqTimeP);
          }

          teStateP = eDischarge;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // stay in this state
        //
        else
        {
          clAddStateInfoP = String("Der Strom wird nicht begrenzt und der Zeitstempel wird gespeichert, solange die Bedingung 2. nicht erfüllt ist");
        }

        break;

        //---------------------------------------------------------------------------
        // The current is not limited and no other
        //
      case eDischarge:

        //---------------------------------------------------------------------------
        // 3. Charge Voltage <= (DISCHARGE_VOLTAGE) (42.0 V)
        //
        if (slBatteryVoltageP <= BG_DISCHARGE_VOLTAGE)
        {
          teStateP = eDischarged;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // 6. Charge Current > 0.5 A
        //
        else if (slBatteryCurrentP > 50)
        {
          teStateP = eCharge;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // stay in this state
        //
        else
        {
          clAddStateInfoP = String("Der Strom wird nicht begrenzt, solange die Bedingung 3. oder 5. nicht erfüllt ist");
        }

        break;

        //---------------------------------------------------------------------------
        // The current is limited to the value 0.0 A, feed-in is stopped
        //
      case eDischarged:

        //---------------------------------------------------------------------------
        // 4. (Charge Voltage > (ABSORPTION_VOLTAGE) (51.2 V)) && (Charge Current > 0.1 A)
        //
        if ((slBatteryVoltageP > BG_ABSORPTION_VOLTAGE) && (slBatteryCurrentP > 10))
        {
          teStateP = eCharge;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // stay in this state
        //
        else
        {
          clAddStateInfoP = String("Der Strom wird auf den Wer 0.0 A begrenzt, solange die Bedingung 4. nicht erfüllt ist");
        }

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
void BatteryGuard::updateMpptState(uint8_t ubStateV)
{
  //---------------------------------------------------------------------------------------------------
  // save value for further usage
  //
  ubMpptStateOfOperationP = ubStateV;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void BatteryGuard::updateCurrent(float ftCurrentV)
{
  //---------------------------------------------------------------------------------------------------
  // take and scale value for comparison
  //
  slBatteryCurrentP = (int32_t)(ftCurrentV * 100.0); // for fast comparison

  //---------------------------------------------------------------------------------------------------
  // if this is the initial set of voltage, than update minimal and maximal value
  //
  if ((slBatteryCurrentMaximalP == 0) && (slBatteryCurrentMinimalP == 0))
  {
    slBatteryCurrentMaximalP = slBatteryCurrentP;
    slBatteryCurrentMinimalP = slBatteryCurrentP;
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void BatteryGuard::updateVoltage(float ftVoltageV)
{
  //---------------------------------------------------------------------------------------------------
  // take new value and scale another one for comparison
  //
  ftBatteryVoltageP = ftVoltageV;
  slBatteryVoltageP = (int32_t)(ftBatteryVoltageP * 10.0); // for fast comparison

  //---------------------------------------------------------------------------------------------------
  // if this is the initial set of voltage, than update minimal and maximal value
  //
  if ((slBatteryVoltageMinimalP == 0) && (slBatteryVoltageMaximalP == 0))
  {
    slBatteryVoltageMaximalP = slBatteryVoltageP;
    slBatteryVoltageMinimalP = slBatteryVoltageP;
  }
}
