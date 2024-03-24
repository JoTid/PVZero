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
  case eMpptNotBulk:
    // current is not limited
    break;

  case eChargingWithDischarge:
    // current is not limited
    break;

  case eCharging:
    //-------------------------------------------------------------------------------------------
    // The current is limited to the Battery Current value of the Victron SmartSolar
    // nur wenn die Spannung einen gewissen wert unterschirtten hat. Im anderen Fall wird der Akku schon den
    // sollwert abfangen.

    if (slTargeCurrentT > slBatteryCurrentP)
    {
      ftLimitCurrentT = (float)(slBatteryCurrentP);
      ftLimitCurrentT *= 0.01;

      // check the MPPT is in absorbtion phase
      //
      // if (slBatteryVoltageP > (BG_CHARGE_CUTOFF_VOLTAGE - 14))
      // {
      // ftLimitCurrentT += 0.2;
      // }
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
        // The current is limited to the Charging Current value of the Victron SmartSolar
        //
      case eMpptNotBulk:
        clAddStateInfoP = String("MPPT is no in Bulk operation State : " + String(ubMpptStateOfOperationP));
        break;

      case eChargingWithDischarge:
        //
        // Grund für diese Maßnahmen: Der Laderegler, regelet immer wieder den Strom gegen 0, was dazu führt, dass
        // der Wechselrichter getrennt wird und sich wieder auf Netz synchronisieren muss.
        // Dies kann bis zu einer Minute dauern, dass soll nicht so sein.
        //
        //
        //---------------------------------------------------------------------------
        // Battery Voltage < (52 V)
        //
        if (slBatteryVoltageP < 520)
        {
          teStateP = eCharging;

          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }
        else
        {
          clAddStateInfoP = String("the feed in current is not limited while charge voltage is >= 52.0 V.");
        }
        break;

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
          //---------------------------------------------------------------------------
          // save time when the battery has been fully charged
          //
          if (pfnSaveTimeHandlerP != nullptr)
          {
            pfnSaveTimeHandlerP(uqTimeP);
          }

          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }
        else if ((slBatteryVoltageP >= 530) && (slBatteryCurrentP > 0))
        {
          teStateP = eChargingWithDischarge;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
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
          if (1) //(uqTimeP - uqFullyChargedTimeP) < (uint64_t)BG_FULL_CHARGE_REPETITION_TIME)
          {
            teStateP = eDischarging;
            if (pfnEventHandlerP != nullptr)
            {
              pfnEventHandlerP(teStateP);
            }
          }
          else
          {
            clAddStateInfoP = String("full battery charge every 2 weeks, charging till Charge Voltage reaches " + String(((float)BG_CHARGE_CUTOFF_VOLTAGE) * 0.1, 1) + " V, discharging is blocked!");
          }
        }
        else
        {
          clAddStateInfoP = String("the feed in current is limited to the Charge Current value while charge voltage is < 53.0 V. ");
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
        // while we are charged, update the time
        //
        uqFullyChargedTimeP = uqTimeP;

        //---------------------------------------------------------------------------
        // Check condition for one possible transition to discharging:
        // Battery Voltage < CHARGE_CUTOFF_VOLTAGE (58.4 V)
        //
        if (slBatteryVoltageP < BG_CHARGE_CUTOFF_VOLTAGE)
        {
          teStateP = eDischarging;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }
        else
        {
          clAddStateInfoP = String("the feed in current is not limited, time stamp has been saved.");
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
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }

        //---------------------------------------------------------------------------
        // Battery Current > 0.0 A
        //
        else if (slBatteryCurrentP > 0)
        {
          teStateP = eCharging;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }
        else
        {
          clAddStateInfoP = String("the feed in current is not limited.");
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
        // Battery Voltage > (DISCHARGE_VOLTAGE + DISCHARGE_VOLTAGE_OFFSET) (40.0 V + 2.0V)
        //
        if (slBatteryVoltageP > (BG_DISCHARGE_VOLTAGE + BG_DISCHARGE_VOLTAGE_OFFSET))
        {
          teStateP = eCharging;
          if (pfnEventHandlerP != nullptr)
          {
            pfnEventHandlerP(teStateP);
          }
        }
        else
        {
          clAddStateInfoP = String("the feed in current is limited to the value 0.0 A, feed-in is stopped.");
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

    if (pfnEventHandlerP != nullptr)
    {
      pfnEventHandlerP(teStateP);
    }
  }
}

void BatteryGuard::updateMpptState(uint8_t ubStateV)
{
  if (ubMpptStateOfOperationP != ubStateV)
  {
    ubMpptStateOfOperationP = ubStateV;

    if ((ubMpptStateOfOperationP == 4) ||
        (ubMpptStateOfOperationP == 5))
    {
      teStateP = eMpptNotBulk;
    }
    else
    {
      teStateP = eCharging;
    }
  }
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
