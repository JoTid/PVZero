//====================================================================================================================//
// File:          pvz_mppt.cpp                                                                                        //
// Description:   PhotoVoltaics Zero - MPPT (Maximum Power Point Tracker)                                             //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include "pvz_mppt.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzMppt::PvzMppt()
{
  btBatteryCurrentReadOkP = false;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
PvzMppt::~PvzMppt()
{
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzMppt::updateFrame(const char *pszFrameV, const int32_t slLengthV)
{
  std::lock_guard<std::mutex> lck(mpptMutexP);

  //---------------------------------------------------------------------------------------------------
  // copy provided data to object memory
  //
  memcpy(ascFrameP, pszFrameV, slLengthV);
  slLengthP = slLengthV;

  //---------------------------------------------------------------------------------------------------
  // start value of the checksum
  //
  uint32_t ulChecksumT = 0;

  //---------------------------------------------------------------------------------------------------
  // if the leng is less that 120 bytes than that is not plausible and not valid
  // he
  if (slLengthP > 120)
  {
    //-------------------------------------------------------------------------------------------
    // calculate the checksum
    //
    for (int i = 0; i < slLengthP; i++)
    {
      ulChecksumT = ((ulChecksumT + ascFrameP[i]) & 0xFF);
    }
  }
  else
  {
    ulChecksumT = 1; // Not Valid
  }

#ifdef MPPT_LOG_REQ_RESP
  //-------------------------------------------------------------------------------------------
  // print the frame for debugging
  //
  Serial.println();
  Serial.print("MPPT Text Frame length ");
  Serial.print(slLengthV);
  Serial.print(", data: ");
  Serial.println(ascFrameP);
#endif

  //-------------------------------------------------------------------------------------------
  // parse only if checksum is valid
  //
  if (ulChecksumT == 0)
  {
    parseTable(ascFrameP);
    btCrcIsValidP = true;
  }
  else
  {
    btCrcIsValidP = false;
  }

#ifdef MPPT_LOG_REQ_RESP
  //-------------------------------------------------------------------------------------------
  // print the parsed data for debugging
  //
  Serial.print("CRC result is: ");
  Serial.println(btCrcIsValidP);
  Serial.println();
  if (btCrcIsValidP)
  {
    for (int i = 0; i < slNumberOfParsedValuesP; i++)
    {
      Serial.print(atsMpptDataP[i].pscName);
      Serial.print(": ");
      Serial.println(atsMpptDataP[i].pscValue);
    }
  }
#endif
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//

float PvzMppt::batteryVoltage()
{
  std::lock_guard<std::mutex> lck(mpptMutexP);
  return ftBatteryVoltageP;
}
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float PvzMppt::batteryCurrent()
{
  float ftValueT;
  std::lock_guard<std::mutex> lck(mpptMutexP);

  ftValueT = slBatteryCurrentP;
  ftValueT *= 0.001; // scale from mA to A

  return ftValueT;
}

String PvzMppt::productId()
{
  std::lock_guard<std::mutex> lck(mpptMutexP);
  return clProductIdP;
}

uint8_t PvzMppt::stateOfOperation()
{
  std::lock_guard<std::mutex> lck(mpptMutexP);
  return ubStateOfOperationP;
}

bool PvzMppt::available()
{
  std::lock_guard<std::mutex> lck(mpptMutexP);
  return btCrcIsValidP;
}

float PvzMppt::powerYieldToday()
{
  std::lock_guard<std::mutex> lck(mpptMutexP);
  return ftPowerYieldTodayP;
}

float PvzMppt::panelVoltage()
{
  std::lock_guard<std::mutex> lck(mpptMutexP);
  return ftPanelVoltageP;
}

float PvzMppt::panelPower()
{
  std::lock_guard<std::mutex> lck(mpptMutexP);
  return ftPanelPowerP;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
bool PvzMppt::isNumber(const std::string &s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it))
    ++it;
  return !s.empty() && it == s.end();
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzMppt::parseTable(char *pscTextFrameV)
{
  const char *EOL = "\r\n"; // Trennzeichen für parametersatz
  const char *SEP = "\t";   // Trennzeichen zwischen parameter name und parameter wert
  int32_t slParamSetIndexT = 0;
  char *ptr, *savePtr, *p, *saveP;
  int32_t slValueT;

  //---------------------------------------------------------------------------------------------------
  // Teile die Tabelle in Parametersätze auf und verarbeite jeden
  //
  ptr = strtok_r(pscTextFrameV, EOL, &savePtr);
  while (ptr != NULL)
  {
    //-------------------------------------------------------------------------------------------
    // Hole alles was lins von dem Parametertrennzeichen steht
    //
    p = strtok_r(ptr, SEP, &saveP);
    atsMpptDataP[slParamSetIndexT].pscName = p;

    //-------------------------------------------------------------------------------------------
    // Hole das was rechts von dem Parametertrennzeichen steht
    //
    p = strtok_r(NULL, SEP, &saveP);
    atsMpptDataP[slParamSetIndexT].pscValue = p;

    //-------------------------------------------------------------------------------------------
    // kopiere die einzelnen Zeichenketten direkt in die entsprechende Werte
    //
    if (String("PID").equals(atsMpptDataP[slParamSetIndexT].pscName))
    {
      // SmartSolar MPPT 150 | 35: 0xA058
      clProductIdP = p;
    }

    //-------------------------------------------------------------------------------------------
    // proceed to process values only if product ID is valid
    //
    if ((p != nullptr) &&
        (clProductIdP != nullptr) &&
        (isNumber(p) == true) &&
        (clProductIdP == "0xA058"))
    {
      if (String("V").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        ftBatteryVoltageP = (float)atoi(p);
        ftBatteryVoltageP *= 0.001; // scale from mV to V
      }

      if (String("VPV").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        ftPanelVoltageP = (float)atoi(p);
        ftPanelVoltageP *= 0.001; // scale from mV to V
      }

      if (String("PPV").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        ftPanelPowerP = (float)atoi(p); // value provided in W
      }

      if (String("I").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        slValueT = atoi(p);

        if (((slValueT > (slBatteryCurrentP + 200)) || (slValueT < (slBatteryCurrentP - 200))) &&
            (btBatteryCurrentReadOkP) && (slBatteryCurrentIgnoreCounterP > 0))
        {
          slBatteryCurrentIgnoreCounterP--;
          if (slValueT > slBatteryCurrentP)
          {
            slBatteryCurrentP += 50;
          }
          else
          {
            slBatteryCurrentP -= 50;
            if (slBatteryCurrentP < 0)
            {
              slBatteryCurrentP = 0;
            }
          }
        }
        else
        {
          btBatteryCurrentReadOkP = true;
          slBatteryCurrentIgnoreCounterP = 30;
          slBatteryCurrentP = slValueT;
        }

        // ftBatteryCurrentP *= 0.001; // scale from mA to A
      }

      if (String("CS").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        ubStateOfOperationP = (uint8_t)atoi(p);
      }

      if (String("H20").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        ftPowerYieldTodayP = (float)atoi(p);
        ftPowerYieldTodayP *= 0.01; // H20 0.01 kWh
      }
    }

    //-------------------------------------------------------------------------------------------
    // iteriere zum nächsten Parametersatz
    //
    slParamSetIndexT++;

    //-------------------------------------------------------------------------------------------
    // Verarbeite jeden Parametersatz
    //
    ptr = strtok_r(NULL, EOL, &savePtr);
  }

  slNumberOfParsedValuesP = slParamSetIndexT;
}
