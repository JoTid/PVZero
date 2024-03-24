//====================================================================================================================//
// File:          pvz_psu.cpp                                                                                         //
// Description:   PhotoVoltaics Zero - PSU                                                                            //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include "pvz_psu.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzPsu::PvzPsu()
{
  slModelNumberP = -1; // not initialised
  ftActualVoltageP = 0.0;
  ftActualCurrentP = 0.0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzPsu::~PvzPsu()
{
}

int32_t PvzPsu::enable(bool btEnableV)
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  int32_t slReturnT = slModelNumberP;

  if (slModelNumberP > 0)
  {
    slReturnT = clPsuP.power(btEnableV);
  }

  return slReturnT;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::init(HardwareSerial &clSerialR)
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  //---------------------------------------------------------------------------------------------------
  // init serial interface and PSU
  //
  slModelNumberP = clPsuP.begin(clSerialR, 1);

  //---------------------------------------------------------------------------------------------------
  // read the model number of PSU, or negative value in case of an error
  //
  if (slModelNumberP > 0)
  {
    clPsuP.power(false);
  }
  else
  {
    // reset actual values
    ftActualVoltageP = 0.0;
    ftActualCurrentP = 0.0;
    ftActualTemperatureP = -7000.0; // invalid temperature
  }

  return slModelNumberP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::read()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  int32_t slReturnT = slModelNumberP;
  float ftReadValueT;

  //---------------------------------------------------------------------------------------------------
  // read voltage only if PSU is available an no previous errors occur
  //
  if (slReturnT >= 0)
  {
    ftReadValueT = clPsuP.read('v');
    slReturnT = (int32_t)ftReadValueT;
    if (slReturnT >= 0)
    {
      ftActualVoltageP = ftReadValueT;
    }
  }

  //---------------------------------------------------------------------------------------------------
  // read current only if PSU is available an no previous errors occur
  //
  if (slReturnT >= 0)
  {
    ftReadValueT = clPsuP.read('c');
    slReturnT = (int32_t)ftReadValueT;
    if (slReturnT >= 0)
    {
      ftActualCurrentP = ftReadValueT;
    }
  }

  //---------------------------------------------------------------------------------------------------
  // read temperature only if PSU is available an no previous errors occur
  //
  if (slReturnT >= 0)
  {
    ftReadValueT = clPsuP.read('t');
    slReturnT = (int32_t)ftReadValueT;
    if (slReturnT >= 0)
    {
      ftActualTemperatureP = ftReadValueT;
    }
  }

  if (slReturnT > 0)
  {
    slReturnT = 0;
  }

  return slReturnT;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::write()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  int32_t slReturnT;

  slReturnT = clPsuP.writeVC(ftTargetVoltageP, ftTargetCurrentP);

  if (slReturnT > 0)
  {
    slReturnT = 0;
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
// void PvzPsu::process(bool btForceV)
// {
//   std::lock_guard<std::mutex> lck(uartMutexP);
//   static unsigned long ulOldTimeS;
//   static uint32_t ulRefreshTimeT;

//   //---------------------------------------------------------------------------------------------------
//   // Model number is negative in case of failure at initialisation.
//   // In other case it contain the maximal A value (5, 8, 16 or 24)
//   //
//   if ((slModelNumberP < 0) && (btForceV == false))
//   {
//     return;
//   }

//   //---------------------------------------------------------------------------------------------------
//   // count the millisecond ticks and avoid overflow
//   //
//   unsigned long ulNewTimeT = millis();
//   if (ulNewTimeT != ulOldTimeS)
//   {
//     if (ulNewTimeT > ulOldTimeS)
//     {
//       ulRefreshTimeT += (uint32_t)(ulNewTimeT - ulOldTimeS);
//     }
//     else
//     {
//       ulRefreshTimeT += (uint32_t)(ulOldTimeS - ulNewTimeT);
//     }
//     ulOldTimeS = ulNewTimeT;
//   }

//   //---------------------------------------------------------------------------------------------------
//   // refresh the PSU only within define time
//   //
//   if (ulRefreshTimeT > PSU_REFRESH_TIME)
//   {
//     ulRefreshTimeT = 0;

//     //-------------------------------------------------------------------------------------------
//     // simple poll actual values from PSU
//     //
//     if (clPsuP.read('p') > 0)
//     {
//       btIsEnabledP = true;
//     }
//     else
//     {
//       btIsEnabledP = false;
//     }

//     ftActualVoltageP = clPsuP.read('v');
//     ftActualCurrentP = clPsuP.read('c');
//     ftActualTemperatureP = clPsuP.read('t');
//   }
// }

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::set(float ftVoltageV, float ftCurrentV)
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  int32_t slReturnT = slModelNumberP;

  ftTargetVoltageP = ftVoltageV;
  ftTargetCurrentP = ftCurrentV;

  return slReturnT;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float PvzPsu::actualVoltage()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  return ftActualVoltageP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float PvzPsu::actualCurrent()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  return ftActualCurrentP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
bool PvzPsu::isEnabled()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  return btIsEnabledP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
bool PvzPsu::isAvailable()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  if (slModelNumberP > 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float PvzPsu::targetVoltage()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  return ftTargetVoltageP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float PvzPsu::targetCurrent()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  return ftTargetCurrentP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::model()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  return slModelNumberP;
}