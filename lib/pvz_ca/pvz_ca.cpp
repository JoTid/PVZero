//====================================================================================================================//
// File:          pvz_ca.cpp                                                                                          //
// Description:   PhotoVoltaics Zero - Control Algorithm                                                              //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//


/*--------------------------------------------------------------------------------------------------------------------*\
** Include Files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include "Arduino.h"
#include "pvz_ca.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzCa::PvzCa()
{
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzCa::~PvzCa()
{
}


//--------------------------------------------------------------------------------------------------------------------//
// CalculateGainOffsetFV()                                                                                            //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzCa::CalculateGainOffset(void)
{
  float ftCalcT;
  float ftCalcXT;
  float ftCalcYT;

  //---------------------------------------------------------------------------------------------------
  // update the limits of the power, that depend on the voltage
  //
  aftConsumptionPowerLimitP[0] = ftFeedInTargetDcVoltageP * aftFeedInTargetDcCurrentLimitP[0];
  aftConsumptionPowerLimitP[1] = ftFeedInTargetDcVoltageP * aftFeedInTargetDcCurrentLimitP[1];

  //---------------------------------------------------------------------------------------------------
  // calculate gain
  //   m  = (  y2   -   y1  ) / (  x2    -   x1   )
  // Gain = (CurrentMax - CurrentMin) / (PowerMax - PowerMin)
  //
  ftCalcYT = (aftFeedInTargetDcCurrentLimitP[1] - aftFeedInTargetDcCurrentLimitP[0]);
  ftCalcXT = (aftConsumptionPowerLimitP[1] - aftConsumptionPowerLimitP[0]);

  //--------------------------------------------------------------------------------------------------- 
  // avoid division by 0
  //
  if (ftCalcXT > 0)
  {
    //-------------------------------------------------------------------------------------------
    // divisor is not 0, so perform the calculation
    //
    ftCurrentGainP = (ftCalcYT / ftCalcXT);

    //-------------------------------------------------------------------------------------------
    // calculate offset
    //   b    =   y    - (  m  *    x   )
    // offset = CurrentMin - (Gain * PowerMin)
    //
    ftCalcXT = ftCurrentGainP * aftConsumptionPowerLimitP[0];
    ftCurrentOffsetP = aftFeedInTargetDcCurrentLimitP[0] - ftCalcXT; 
  }

  //--------------------------------------------------------------------------------------------------- 
  // should never run in here, so set value to 0 to cut off the output current
  //
  else
  {
    //------------------------------------------------------------------------------------------- 
    // typically this value is 1.0 but set here 0 so the output goes to 0 A
    //
    ftCurrentGainP = 0.0;   
    ftCurrentOffsetP = 0.0;
  }
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzCa::init(void)
{
  //--------------------------------------------------------------------------------------------------- 
  // just reset all values to default, make sure this values do not damage the device
  // 
  ftConsumptionPowerP = 0.0;

  ftFeedInActualPowerP = 0.0;
  ftFeedInActualDcCurrentP = 0.0;
  ftFeedInActualDcVoltageP = 0.0;

  ftFeedInTargetPowerP = 0.0;
  ftFeedInTargetDcCurrentP = 0.0;
  ftFeedInTargetDcVoltageP = 0.0;

  ubFilterOrderP = 1;
  clConsPowerFilterP.init(ubFilterOrderP);

  aftFeedInTargetDcCurrentLimitP[0] = 0.0;
  aftFeedInTargetDcCurrentLimitP[1] = 0.0;

  //--------------------------------------------------------------------------------------------------- 
  // calc gain and offset
  //
  CalculateGainOffset();
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
float PvzCa::discreteApproximation(float ftActualV, float ftTargetV)
{
  float ftReturnT;

  //--------------------------------------------------------------------------------------------------- 
  // Jump function: Takes place during a downward correction so that a current flow is interrupted as 
  // quickly as possible. A change from 9 A to 1 A, for example, takes place in one step.
  //
  if (ftActualV > ftTargetV)
  {
    ftReturnT = ftTargetV;
  }

  //--------------------------------------------------------------------------------------------------- 
  // Stair function: The target value is approached in defined steps so that no components are damaged 
  // by current jumps.
  //
  else
  {
    ftReturnT = ftActualV + 0.5; // step up by 0,5 A 
    if (ftReturnT > ftTargetV)
    {
      ftReturnT = ftTargetV; 
    }
  }

  return ftReturnT;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzCa::process(void)
{ 
  static unsigned long ulOldTimeS;
  static uint32_t ulRefreshTimeT;

  float ftCalcT;

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
  // refresh the LCD only within define time
  //
  if (ulRefreshTimeT > CA_REFRESH_TIME)
  {
    ulRefreshTimeT = 0;

    //---------------------------------------------------------------------------------------------------
    // consider pending feed-in power
    //
    ftCalcT = ftConsumptionPowerP + ftFeedInActualPowerP;

    //---------------------------------------------------------------------------------------------------
    // scale values y = m * x + b
    //
    ftCalcT  = (ftCalcT * ftCurrentGainP);
    ftCalcT += ftCurrentOffsetP;

    //---------------------------------------------------------------------------------------------------
    // limit feed-in DC Current value 
    //
    if (ftCalcT < aftFeedInTargetDcCurrentLimitP[0])
    {
      ftCalcT = aftFeedInTargetDcCurrentLimitP[0];
    } 
    else if (ftCalcT > aftFeedInTargetDcCurrentLimitP[1])
    {
      ftCalcT = aftFeedInTargetDcCurrentLimitP[1];
    }

    //---------------------------------------------------------------------------------------------------
    // store feed-in target current and calculate feed in power P = U * I
    //
    ftFeedInTargetDcCurrentP = discreteApproximation(ftFeedInActualDcCurrentP, ftCalcT);

    ftCalcT = ftFeedInTargetDcVoltageP * ftFeedInTargetDcCurrentP;
    ftFeedInTargetPowerP = ftCalcT;
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
float PvzCa::updateConsumptionPower(float ftPowerV)
{
  ftConsumptionPowerP = clConsPowerFilterP.process(ftPowerV);

  btConsumptionPowerPendingP = true;

  return ftConsumptionPowerP;
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
int32_t PvzCa::updateFeedInActualDcValues(float ftVoltageV, float ftCurrentV)
{
  //--------------------------------------------------------------------------------------------------- 
  // store provided values and calculate power [Wh] using formula: P=UI
  // 
  ftFeedInActualDcVoltageP = ftVoltageV;
  ftFeedInActualDcCurrentP = ftCurrentV;
  ftFeedInActualPowerP = ftFeedInActualDcVoltageP * ftFeedInActualDcCurrentP;
 
  btActualValuesPendingP = true;

  return 0;
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
int32_t PvzCa::setFeedInTargetDcCurrentLimits(float ftMinV, float ftMaxV)
{
  int32_t slReturnT = -1;

  //--------------------------------------------------------------------------------------------------- 
  // check value for plausibility and store them
  //
  if ((ftMinV >= 0.0) && (ftMinV <= ftMaxV))
  {
    //------------------------------------------------------------------------------------------- 
    // store limits and calc gain and offset
    //
    aftFeedInTargetDcCurrentLimitP[0] = ftMinV;
    aftFeedInTargetDcCurrentLimitP[1] = ftMaxV;

    CalculateGainOffset();

    slReturnT = 0;
  }

  return slReturnT;
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
int32_t PvzCa::setFeedInTargetDcVoltage(float ftVoltageV)
{
  //--------------------------------------------------------------------------------------------------- 
  // update voltage and calc gain and offset
  //
  ftFeedInTargetDcVoltageP = ftVoltageV;
  CalculateGainOffset();

  return 0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
int32_t PvzCa::setFilterOrder(uint8_t ubFilterOrderV)
{
  int32_t slReturnT = -1;

  if ((ubFilterOrderV > 0) && (ubFilterOrderV <= 40))
  {
    ubFilterOrderP = ubFilterOrderV;
    clConsPowerFilterP.init(ubFilterOrderP);
    slReturnT = 0;
  }

  return slReturnT;
}
