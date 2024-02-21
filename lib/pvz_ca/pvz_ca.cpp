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

  ftFilterOrderP = 3.0;

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
void PvzCa::process(void)
{ 
  float ftCalcT;

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
  ftFeedInTargetDcCurrentP = ftCalcT;
  ftCalcT = ftFeedInTargetDcVoltageP * ftFeedInTargetDcCurrentP;
  ftFeedInTargetPowerP = ftCalcT;
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void PvzCa::updateConsumptionPower(float ftPowerV)
{
  ftConsumptionPowerP = ftPowerV;
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
    ftFilterOrderP = (float) ubFilterOrderV;
    slReturnT = 0;
  }

  return slReturnT;
}
