//====================================================================================================================//
// File:          pvz_ca.cpp                                                                                          //
// Description:   Photovoltaics Zero - Control Algorithm                                                              //
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
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzCa::init(void)
{
  //--------------------------------------------------------------------------------------------------- 
  // just reset all values to default
  // 
  ftConsumptionPowerP = 0.0;
  ftFeedInPowerP = 0.0;
  ftFeedInDcCurrentP = 0.0;
  ftFeedInDcVoltageP = 36.0;
  ftFilterOrderP = 3.0;

  aftFeedInDcCurrentLimitP[0] = 0.0;
  aftFeedInDcCurrentLimitP[1] = 9.0;

  aftConsumptionPowerLimitP[0] = ftFeedInDcVoltageP * aftFeedInDcCurrentLimitP[0];
  aftConsumptionPowerLimitP[1] = ftFeedInDcVoltageP * aftFeedInDcCurrentLimitP[1];

  //--------------------------------------------------------------------------------------------------- 
  // calc gain and offset
  //
  CalculateGainOffset();

  //--------------------------------------------------------------------------------------------------- 
  // trigger calculation fo gain and offset for scaling from feed-in power to 
  //
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
  // calculate gain
  //   m  = (  y2   -   y1  ) / (  x2    -   x1   )
  // Gain = (CurrentMax - CurrentMin) / (PowerMax - PowerMin)
  //
  ftCalcYT = (aftFeedInDcCurrentLimitP[1] - aftFeedInDcCurrentLimitP[0]);
  ftCalcXT = (aftConsumptionPowerLimitP[1] - aftConsumptionPowerLimitP[0]);

  if (ftCalcXT > 0)
  {
    ftCurrentGainP = (ftCalcYT / ftCalcXT);

    //-------------------------------------------------------------------------------------------
    // calculate offset
    //   b    =   y    - (  m  *    x   )
    // offset = CurrentMin - (Gain * PowerMin)
    //
    ftCalcXT = ftCurrentGainP * aftConsumptionPowerLimitP[0];
    ftCurrentOffsetP = aftFeedInDcCurrentLimitP[0] - ftCalcXT; 
  } 
  else
  {
    ftCurrentGainP = 0.0; // typically ist should 1.0 but set here 0 so the output goes to 0 A
    ftCurrentOffsetP = 0.0;
  }
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
  ftCalcT = ftConsumptionPowerP + ftFeedInPowerP;

  //---------------------------------------------------------------------------------------------------
  // scale values y = m * x + b
  //
  ftCalcT  = (ftCalcT * ftCurrentGainP);
  ftCalcT += ftCurrentOffsetP;

  //---------------------------------------------------------------------------------------------------
  // limit feed-in DC Current value 
  //
  if (ftCalcT < aftFeedInDcCurrentLimitP[0])
  {
    ftCalcT = aftFeedInDcCurrentLimitP[0];
  } 
  else if (ftCalcT > aftFeedInDcCurrentLimitP[1])
  {
    ftCalcT = aftFeedInDcCurrentLimitP[1];
  }

  ftFeedInDcCurrentP = ftCalcT;

  //---------------------------------------------------------------------------------------------------
  // calculate feed in power P = U * I
  //
  ftCalcT = ftFeedInDcVoltageP * ftFeedInDcCurrentP;
  ftFeedInPowerP = ftCalcT;

}

//--------------------------------------------------------------------------------------------------------------------//
// setFilterOrder()                                                                                                   //
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

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void PvzCa::setConsumptionPower(float ftPowerV)
{
  ftConsumptionPowerP = ftPowerV;
}

