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
  slReadCurrenTriggerP = 10;
  slReadVoltageTriggerP = 10;
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

  // Serial.print("s: ");
  //---------------------------------------------------------------------------------------------------
  // read status
  //
  if (slReturnT >= 0)
  {
    ftReadValueT = clPsuP.read('s');
    slReturnT = (int32_t)ftReadValueT;
    if (slReturnT > 0)
    {
      btConstantCurrentOutputP = true;
      // Serial.print("c: ");
    }
    else
    {
      btConstantCurrentOutputP = false;
      // Serial.print("v: ");
    }
  }

  //---------------------------------------------------------------------------------------------------
  // read voltage only if PSU is available an no previous errors occur
  //
  if (slReturnT >= 0)
  {
    ftReadValueT = clPsuP.read('v');
    slReturnT = (int32_t)ftReadValueT;
    if (slReturnT >= 0)
    {
      // take value only the PSU indicates constant voltage
      if (btConstantCurrentOutputP == false)
      {
        ftActualVoltageP = ftReadValueT;
        slReadVoltageTriggerP = 10;
      }
      else
      {
        slReadVoltageTriggerP--;
        if (slReadVoltageTriggerP == 0)
        {
          slReadVoltageTriggerP = 10;
          ftActualVoltageP = ftReadValueT;
        }
      }
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
      // take value only the PSU indicates constant current
      if (btConstantCurrentOutputP == true)
      {
        ftActualCurrentP = ftReadValueT;
        slReadCurrenTriggerP = 10;
      }
      else
      {
        slReadCurrenTriggerP--;
        if (slReadCurrenTriggerP == 0)
        {
          slReadCurrenTriggerP = 10;
          ftActualCurrentP = ftReadValueT;
        }
      }
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

  // Serial.print("V");
  // Serial.print(ftActualVoltageP, 1);
  // Serial.print(" C");
  // Serial.print(ftActualCurrentP, 1);

  // Serial.print(" T");
  // Serial.println(ftActualTemperatureP, 1);

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
  int32_t slReturnT = 0;

  //---------------------------------------------------------------------------------------------------
  // Write new values to the PSU
  //
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