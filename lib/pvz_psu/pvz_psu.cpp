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
    slReturnT = clPsuP.writeFunction(DPM86xx::eFUNC_OUTPUT_STATUS, (uint16_t)btEnableV);
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
  clPsuP.init(clSerialR);

  slModelNumberP = clPsuP.readFunction(DPM86xx::eFUNC_MAX_VOLTAGE);
  // Serial.print("My Max Voltage: ");
  // Serial.println(slModelNumberP);

  slModelNumberP = clPsuP.readFunction(DPM86xx::eFUNC_MAX_CURRENT);
  // Serial.print("My Max Current: ");
  // Serial.println(slModelNumberP);

  //---------------------------------------------------------------------------------------------------
  // read the model number of PSU, or negative value in case of an error
  //
  if (slModelNumberP > 0)
  {
    // slModelNumberP = clPsuP.writeFunction(DPM86xx::eFUNC_OUTPUT_STATUS, 0);
    //   Serial.print("write Result: ");
    //   Serial.println(slModelNumberP);
    // if (slModelNumberP == DPM86xx::eFUNC_WRITE_OK)
    // {
    slModelNumberP = 1;
    // }
  }
  // else
  // {
  //   // reset actual values
  //   ftActualVoltageP = 0.0;
  //   ftActualCurrentP = 0.0;
  //   ftActualTemperatureP = -7000.0; // invalid temperature
  // }

  return slModelNumberP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::read()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  int32_t slStatusT = 0;
  // float ftReadValueT;

  // Serial.print("s: ");
  //---------------------------------------------------------------------------------------------------
  // read status
  //
  if (slStatusT >= 0)
  {
    // ftReadValueT = clPsuP.read('s');
    slStatusT = clPsuP.readFunction(DPM86xx::eFUNC_CONSTANT_OUTPUT);

    //-------------------------------------------------------------------------------------------
    // constant voltage output
    //
    if (slStatusT == 0)
    {
      //-----------------------------------------------------------------------------------
      // read voltage
      //
      slReadVoltageTriggerP = 0;
      if (slReadCurrenTriggerP > 0)
      {
        slReadCurrenTriggerP--;
      }
    }

    //-------------------------------------------------------------------------------------------
    // constant current output
    //
    else if (slStatusT == 1)
    {
      //-----------------------------------------------------------------------------------
      // read current
      //
      slReadCurrenTriggerP = 0;
      if (slReadVoltageTriggerP > 0)
      {
        slReadVoltageTriggerP--;
      }
    }
  }

  //-------------------------------------------------------------------------------------------
  // constant voltage output
  //
  if ((slStatusT >= 0) && (slReadVoltageTriggerP == 0))
  {
    //-----------------------------------------------------------------------------------
    // read voltage
    //
    slStatusT = clPsuP.readFunction(DPM86xx::eFUNC_MEASURED_VOLTAGE);
    if (slStatusT >= 0)
    {
      if (((slStatusT > (slActualVoltageP + 100)) || (slStatusT < (slActualVoltageP - 100))) &&
          (slActualVoltageIgnoreCounterP > 0))
      {
        slActualVoltageIgnoreCounterP--;
        if (slStatusT > slActualVoltageP)
        {
          slActualVoltageP += 20;
        }
        else
        {
          slActualVoltageP -= 20;
          if (slActualVoltageP < 0)
          {
            slActualVoltageP = 0;
          }
        }
        ftActualVoltageP = (float)slActualVoltageP;
        ftActualVoltageP *= 0.01;
      }
      else
      {
        slActualVoltageIgnoreCounterP = 30;
        slActualVoltageP = slStatusT;
        ftActualVoltageP = clPsuP.measuredVoltage();
      }
      slReadVoltageTriggerP = 20;
    }
  }

  //-------------------------------------------------------------------------------------------
  // constant current output
  //
  if ((slStatusT >= 0) && (slReadCurrenTriggerP == 0))
  {
    //-----------------------------------------------------------------------------------
    // read current
    //
    slStatusT = clPsuP.readFunction(DPM86xx::eFUNC_MEASURED_CURRENT);
    if (slStatusT >= 0)
    {
      if (((slStatusT > (slActualCurrentP + 300)) || (slStatusT < (slActualCurrentP - 300))) &&
          (slActualCurrentIgnoreCounterP > 0))
      {
        slActualCurrentIgnoreCounterP--;
        if (slStatusT > slActualCurrentP)
        {
          slActualCurrentP += 100;
        }
        else
        {
          slActualCurrentP -= 100;
          if (slActualCurrentP < 0)
          {
            slActualCurrentP = 0;
          }
        }
        ftActualCurrentP = (float)slActualCurrentP;
        ftActualCurrentP *= 0.001;
      }
      else
      {
        slActualCurrentIgnoreCounterP = 10;
        slActualCurrentP = slStatusT;
        ftActualCurrentP = clPsuP.measuredCurrent();
      }
      slReadCurrenTriggerP = 20;
    }
  }

  //---------------------------------------------------------------------------------------------------
  // read temperature only if PSU is available an no previous errors occur
  //
  if (slStatusT >= 0)
  {
    // ftReadValueT = clPsuP.read('t');
    slStatusT = clPsuP.readFunction(DPM86xx::eFUNC_TEMPERATURE);
    if (slStatusT >= 0)
    {
      ftActualTemperatureP = clPsuP.temperature();
    }
  }

  // Serial.print(" V");
  // Serial.print(ftActualVoltageP, 1);
  // Serial.print(" C");
  // Serial.print(ftActualCurrentP, 1);

  // Serial.print(" T");
  // Serial.println(ftActualTemperatureP, 1);

  if (slStatusT > 0)
  {
    slStatusT = 0;
  }

  return slStatusT;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzPsu::write()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  int32_t slReturnT = 0;
  uint16_t uwVoltageT = (uint16_t)(ftTargetVoltageP * 100);
  uint16_t uwCurrentT = (uint16_t)(ftTargetCurrentP * 1000);

  //---------------------------------------------------------------------------------------------------
  // Write new values to the PSU
  //
  // slReturnT = clPsuP.writeVC(ftTargetVoltageP, ftTargetCurrentP);
  slReturnT = clPsuP.writeFunction(DPM86xx::eFUNC_SET_VC, uwVoltageT, uwCurrentT);

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
int32_t PvzPsu::writeCurrent()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  int32_t slReturnT = 0;
  uint16_t uwCurrentT = (uint16_t)(ftTargetCurrentP * 1000);

  //---------------------------------------------------------------------------------------------------
  // Write new values to the PSU
  //
  slReturnT = clPsuP.writeFunction(DPM86xx::eFUNC_SET_CURRENT, uwCurrentT);

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
int32_t PvzPsu::writeVoltage()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  int32_t slReturnT = 0;
  uint16_t uwVoltageT = (uint16_t)(ftTargetVoltageP * 100);

  //---------------------------------------------------------------------------------------------------
  // Write new values to the PSU
  //
  // slReturnT = clPsuP.writeVC(ftTargetVoltageP, ftTargetCurrentP);
  slReturnT = clPsuP.writeFunction(DPM86xx::eFUNC_SET_VOLTAGE, uwVoltageT);

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
void PvzPsu::set(float ftVoltageV, float ftCurrentV)
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  ftTargetVoltageP = ftVoltageV;
  ftTargetCurrentP = ftCurrentV;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float PvzPsu::actualVoltage()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  // Serial.print("PSU VR:");
  // Serial.println(String(ftActualVoltageP, 3));
  return ftActualVoltageP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float PvzPsu::actualCurrent()
{
  std::lock_guard<std::mutex> lck(uartMutexP);
  // Serial.print("PSU IR:");
  // Serial.println(String(ftActualCurrentP, 3));
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