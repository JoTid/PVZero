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

  pfnEventHandlerP = nullptr;
  pfnSaveTimeHandlerP = nullptr;

  teStateP = eDischarged;
  teStateOldP = teStateP;

  aclAddStateInfoP[eCharge] = String("Der Strom wird auf den Wert Charge Current vom MPPT begrenzt\n");
  aclAddStateInfoP[eCharge] += String("- Bei MPPT Current < 0.2 A erfolgt der Wechsel in den Zustand 'discharge', Übergang 5\n");
  aclAddStateInfoP[eCharge] += String("- Bei MPPT Voltage >= " + String(((float)(BG_CHARGE_CUTOFF_VOLTAGE - 50)) * 0.1, 1) + " V erfolgt der Wechsel in den Zustand 'charge and discharge', Übergang 7\n");
  aclAddStateInfoP[eCharge] += String("- Liegt eine Vollladung weiter als 2 Wochen zurück, erfolgt der Wechsel in den Zustand 'charge until charged', Übergang 9");

  aclAddStateInfoP[eChargeAndDischarge] = String("Der Strom wird nicht begrenzt\n");
  aclAddStateInfoP[eChargeAndDischarge] += String("- Bei MPPT Voltage >= " + String(((float)BG_CHARGE_CUTOFF_VOLTAGE) * 0.1, 1) + " V erfolgt der Wechsel in den Zustand 'charged', Übergang 1\n");
  aclAddStateInfoP[eChargeAndDischarge] += String("- Bei MPPT Voltage <= " + String(((float)BG_ABSORPTION_VOLTAGE) * 0.1, 1) + " V erfolgt der Wechsel in den Zustand 'charge', Übergang 8");

  aclAddStateInfoP[eChargeUntilCharged] = String("Der Strom wird auf den Wert 0.0 A begrenzt\n");
  aclAddStateInfoP[eChargeUntilCharged] += String("- Bei MPPT Voltage >= " + String(((float)BG_CHARGE_CUTOFF_VOLTAGE) * 0.1, 1) + " V erfolgt der Wechsel in den Zustand 'charged', Übergang 10");

  aclAddStateInfoP[eCharged] = String("Der Strom wird nicht begrenzt und der Zeitstempel wird beim Verlassen des Zustands gespeichert\n");
  aclAddStateInfoP[eCharged] += String("- Bei MPPT Voltage < " + String(((float)(BG_CHARGE_CUTOFF_VOLTAGE - 4)) * 0.1, 1) + " V erfolgt der Wechsel in den Zustand 'discharge', Übergang 2");

  aclAddStateInfoP[eDischarge] = String("Der Strom wird nicht begrenzt\n");
  aclAddStateInfoP[eDischarge] += String("- Bei MPPT Voltage <= " + String(((float)BG_DISCHARGE_VOLTAGE) * 0.1, 1) + " V erfolgt der Wechsel in den Zustand 'discharged', Übergang 3\n");
  aclAddStateInfoP[eDischarge] += String("- Bei MPPT Current > 0.5 A erfolgt der Wechsel in den Zustand 'charging', Übergang 6");

  aclAddStateInfoP[eDischarged] = String("Der Strom wird auf den Wert 0.0 A begrenzt\n");
  aclAddStateInfoP[eDischarged] += String("- Bei MPPT Voltage > " + String(((float)BG_ABSORPTION_VOLTAGE) * 0.1, 1) + " V und MPPT Current > 0.2 A erfolgt der Wechsel in den Zustand 'charge', Übergang 4");
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
  uqFullyChargedTimeP = uqTimeV;
  uqTimeP = 0; // no pending time has been provided yet
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
        }

        //---------------------------------------------------------------------------
        // 1. Charge Voltage >= CHARGE_CUTOFF_VOLTAGE (58.4 V)
        //
        else if (slBatteryVoltageP >= BG_CHARGE_CUTOFF_VOLTAGE)
        {
          teStateP = eCharged;
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
        }

        //---------------------------------------------------------------------------
        // 9. (Zeitstempel - SavedZeitstempel) > 2 Wochen
        //
        else if ((uqTimeP != 0) && ((uqTimeP - uqFullyChargedTimeP) > (uint64_t)BG_FULL_CHARGE_REPETITION_TIME))
        {
          teStateP = eChargeUntilCharged;
        }

        //---------------------------------------------------------------------------
        // 5. Charge Current < 0.4 A
        //
        else if (slBatteryCurrentP < 40)
        {
          teStateP = eDischarge;
        }

        break;

      case eChargeUntilCharged:
        //---------------------------------------------------------------------------
        // 110. Charge Voltage >= CHARGE_CUTOFF_VOLTAGE (58.4 V)
        //
        if ((slBatteryVoltageP >= BG_CHARGE_CUTOFF_VOLTAGE))
        {
          teStateP = eCharged;
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
        }

        //---------------------------------------------------------------------------
        // 6. Charge Current > 0.6 A
        //
        else if (slBatteryCurrentP > 60)
        {
          teStateP = eCharge;
        }

        break;

        //---------------------------------------------------------------------------
        // The current is limited to the value 0.0 A, feed-in is stopped
        //
      case eDischarged:

        //---------------------------------------------------------------------------
        // 4. (Charge Voltage > (ABSORPTION_VOLTAGE) (51.2 V))
        // do not check the current, as the start can be performed at evening
        //
        if ((slBatteryVoltageP > BG_ABSORPTION_VOLTAGE))
        {
          teStateP = eCharge;
        }

        break;

        //---------------------------------------------------------------------------
        // should never run in here
        //
      default:
        break;
      }

      //-----------------------------------------------------------------------------------
      // trigger event handler at state change
      //
      if (teStateOldP != teStateP)
      {
        teStateOldP = teStateP;
        if (pfnEventHandlerP != nullptr)
        {
          pfnEventHandlerP(teStateP);
        }
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
void BatteryGuard::updateTime(uint64_t uqTimeV)
{
  uint64_t uqTimeDeltaT;

  //---------------------------------------------------------------------------------------------------
  // check time for plausibility
  // If the time is further back than 4 weeks, then this value is invalid and we reset it from today to 1 week
  //
  uqTimeDeltaT = uqTimeV - uqFullyChargedTimeP;
  if (uqTimeDeltaT > (BG_FULL_CHARGE_REPETITION_TIME * 2))
  {
    uqFullyChargedTimeP = (uqTimeV - (BG_FULL_CHARGE_REPETITION_TIME / 2));
    if (pfnSaveTimeHandlerP)
    {
      pfnSaveTimeHandlerP(uqFullyChargedTimeP);
    }
  }

  uqTimeP = uqTimeV;
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
