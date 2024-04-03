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
  // pclMpptP = NULL;
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
  strcpy(ascFrameP, pszFrameV);
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

  //-------------------------------------------------------------------------------------------
  // print the frame for debugging
  //
  // Serial.println();
  // Serial.println();
  // Serial.println();
  // Serial.print(aszFrameP);
  // Serial.println();
  // Serial.println();
  // Serial.println();

  //-------------------------------------------------------------------------------------------
  // parse only if checksum is valid
  //
  if (ulChecksumT == 0)
  {
    parseTable(ascFrameP);
    bAvailableP = true;
  }
  else
  {
    bAvailableP = false;
  }

  //-------------------------------------------------------------------------------------------
  // print the parsed data for debugging
  //
  // for (int i = 0; i < slNumberOfParsedValuesP; i++)
  // {
  //   Serial.print(atsMpptDataP[i].pscName);
  //   Serial.print(": ");
  //   Serial.println(atsMpptDataP[i].pscValue);
  // }
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
  std::lock_guard<std::mutex> lck(mpptMutexP);
  return ftBatteryCurrentP;
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
  return bAvailableP;
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
      clProductIdP = atsMpptDataP[slParamSetIndexT].pscValue;
    }

    //-------------------------------------------------------------------------------------------
    // proceed to process values only if product ID is valid
    //
    if (clProductIdP == "0xA058")
    {
      if (String("V").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        ftBatteryVoltageP = (float)atoi(atsMpptDataP[slParamSetIndexT].pscValue);
        ftBatteryVoltageP *= 0.001; // scale from mV to V
      }

      if (String("I").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        ftBatteryCurrentP = (float)atoi(atsMpptDataP[slParamSetIndexT].pscValue);
        ftBatteryCurrentP *= 0.001; // scale from mA to A
      }

      if (String("CS").equals(atsMpptDataP[slParamSetIndexT].pscName))
      {
        ubStateOfOperationP = (uint8_t)atoi(atsMpptDataP[slParamSetIndexT].pscValue);
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
