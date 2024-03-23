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
  //---------------------------------------------------------------------------------------------------
  // copy provided data to object memory
  //
  std::lock_guard<std::mutex> lck(mpptMutexP);
  strcpy(ascFrameP, pszFrameV);
  slLengthP = slLengthV;
  //-------------------------------------------------------------------------------------------
  // calculate the checksum at first
  //
  uint32_t ulChecksumT = 0;
  for (int i = 0; i < slLengthP; i++)
  {
    ulChecksumT = ((ulChecksumT + ascFrameP[i]) & 0xFF);
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
  // if (ulChecksumT == 0)
  // {
  parseTable(ascFrameP);
  // }

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

    //---------------------------------------------------------------------------------------------------
    // kopiere die einzelnen Zeichenketten direkt in die entsprechende Werte
    //
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
