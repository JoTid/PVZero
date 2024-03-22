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

static void mpptCallback(uint16_t id, int32_t value)
{
  // if (id == VEDirect_kPanelVoltage)
  // {
  //   ftPanelVoltageP = value;
  //   // Serial.print(F("Vpv : "));
  //   // Serial.println(value * 0.01);
  // }
  // if (id == VEDirect_kChargeVoltage)
  // {
  //   ftBatteryVoltageP = value;
  //   // Serial.print(F("Vpv : "));
  //   // Serial.println(value * 0.01);
  // }
  // if (id == VEDirect_kChargeCurrent)
  // {
  //   ftBatteryCurrentP = value;
  //   // Serial.print(F("Ich : "));
  //   // Serial.println(value * 0.1);
  // }
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
int32_t PvzMppt::init(HardwareSerial &clSerialR)
{
  //---------------------------------------------------------------------------------------------------
  // init serial interface and PSU
  //
  // slModelNumberP = clPsuP.begin(clSerialR, 1);
  // pclMpptP = new VEDirect(Serial2, mpptCallback);

  return 0;
}

void PvzMppt::updateFrame(const char *pszFrameV, const int32_t slByteNumberV)
{
  // std::lock_guard<std::mutex> lck(mppt_mutex);
  strcpy(aszFrameP, pszFrameV);
  slByteNumberP = slByteNumberV;
  btNewFrameP = true;
}

// Definieren Sie einen struct, um die Daten aus der Tabelle zu speichern
struct MpptData
{
  char *parameterName;
  char *value;
};

// Deklarieren Sie ein Array von SensorData-Elementen
MpptData MpptData[20]; // Anpassen Sie die Größe an die Anzahl der Zeilen in der Tabelle

void PvzMppt::parseTable(char *tableData)
{
  // Definieren Sie die Trennzeichen
  const char *EOL = "\r\n"; // Zeilentrauf
  const char *SEP = "\t";   // Spaltenbegrenzer
  // char str[256];
  // strcpy(str, tableData);
  // // int i = 0;
  // // while (i < 256)
  // // {
  // //   str[i] = tableData[i];

  // // }
  // char *line = strtok(str, EOL); // Zeile für Zeile parsen
  // EWC::I::get().logger() << " str : " << str << endl;
  // char *token;
  int i = 0;

  // while (line != NULL)
  // {
  //   EWC::I::get().logger() << " Line : " << line << endl;
  //   token = strtok(line, SEP); // Spalten parsen
  //   sensorData[i].parameterName = token;
  //   EWC::I::get().logger() << " token A : " << token << endl;
  //   token = strtok(NULL, SEP);
  //   sensorData[i].value = token;
  //   EWC::I::get().logger() << " token B : " << token << endl;

  //   i++;
  //   line = strtok(NULL, EOL);
  // }
  char *ptr, *savePtr, *p, *saveP;
  // const char delim[] = "|";

  ptr = strtok_r(tableData, EOL, &savePtr);
  while (ptr != NULL)
  {
    // EWC::I::get().logger() << " Line : " << ptr << endl;
    // hier haben wir alles zwischen | (außen)
    // Serial.println(ptr);
    p = strtok_r(ptr, SEP, &saveP);
    // while (p != NULL)
    // {
    MpptData[i].parameterName = p;
    // EWC::I::get().logger() << " token : " << p << endl;

    // // alles zwischen : (innen)
    // Serial.print("____");
    // Serial.println(p);
    p = strtok_r(NULL, SEP, &saveP);
    MpptData[i].value = p;

    if (String("V").equals(MpptData[i].parameterName))
    {
      ftBatteryVoltageP = (float)atoi(MpptData[i].value);
      ftBatteryVoltageP *= 0.001; // scale from mV to V
    }

    if (String("I").equals(MpptData[i].parameterName))
    {
      ftBatteryCurrentP = (float)atoi(MpptData[i].value);
      ftBatteryCurrentP *= 0.001; // scale from mA to A
    }
    // }
    i++;
    // außen neu testen
    ptr = strtok_r(NULL, EOL, &savePtr);
  }

  slNumberOfParsedValuesP = i;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void PvzMppt::process(bool btForceV)
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

  // std::lock_guard<std::mutex> lck(mppt_mutex);
  if (btNewFrameP == true)
  {
    btNewFrameP = false;

    uint32_t ulChecksumT = 0;
    for (int i = 0; i < slByteNumberP; i++)
    {
      ulChecksumT = ((ulChecksumT + aszFrameP[i]) & 0xFF); /* Take modulo 256 in account */
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

    // if (ulChecksumT == 0)
    // {
    /* Checksum is valid => process message */
    // Parsen Sie die Tabellendaten
    parseTable(aszFrameP);
    // }

    //-------------------------------------------------------------------------------------------
    // print the parsed data for debugging
    //
    // for (int i = 0; i < slNumberOfParsedValuesP; i++)
    // {
    //   Serial.print(MpptData[i].parameterName);
    //   Serial.print(": ");
    //   Serial.println(MpptData[i].value);
    // }
  }

  //---------------------------------------------------------------------------------------------------
  // refresh the PSU only within define time
  //
  // if (ulRefreshTimeT > MPPT_REFRESH_TIME)
  // {
  //   ulRefreshTimeT = 0;

  //   // if (pclMpptP != NULL)
  //   // {
  //   //   pclMpptP->ping(); // send ping every MPPT_REFRESH_TIME
  //   // }
  // }
}
