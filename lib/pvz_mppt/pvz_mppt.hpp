//====================================================================================================================//
// File:          pvz_mppt.hpp                                                                                        //
// Description:   PhotoVoltaics Zero - MPPT (Maximum Power Point Tracker)                                             //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

#ifndef PVZ_MPPT_HPP
#define PVZ_MPPT_HPP

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include <Arduino.h>
#include <mutex>

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and Enums                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

// std::mutex mppt_mutex;

#define MPPT_TEXT_FRAME_LENGTH 256
/**
 * @brief
 *
 */
class PvzMppt
{

public:
  PvzMppt();
  ~PvzMppt();

  /**
   * @brief Initialisation of PSU
   * This method should be called only once while setup.
   */
  int32_t init(HardwareSerial &clSerialR);

  /**
   * @brief Parse received Text Frame from Victron SmartSolar.
   * This method must be called from main loop.
   */
  void parse();

  /**
   * @brief Provide new received Frame to the MPPT class
   *
   * @param[in] pszFrameV Pointer to the frame
   * @param[in] slLengthV Length of the frame given in bytes
   */
  void updateFrame(const char *pszFrameV, int32_t slLengthV);

  //---------------------------------------------------------------------------------------------------
  // get method for parameter extracted from frame
  //

  /**
   * @brief Pending battery voltage
   * @return float value given in [V]
   */
  float batteryVoltage() { return ftBatteryVoltageP; }
  /**
   * @brief Pending battery current
   * @return float value given in [A]
   */
  float batteryCurrent() { return ftBatteryCurrentP; }

private:
  /**
   * @brief This struct is used to store parameter from the text frame
   *
   */
  typedef struct MpptData_s
  {
    char *pscName;
    char *pscValue;
  } MpptData_ts;

  /**
   * @brief actually there up to 20 entries possible for MPPT devices,
   *        so reserve the memory for them
   */
  MpptData_ts atsMpptDataP[20];

  /**
   * @brief This char array is used to store the received data from the MPPT device.
   *        An parse it later using \c #parseTable() method.
   */
  char ascFrameP[MPPT_TEXT_FRAME_LENGTH];

  /**
   * @brief holds number of valid bytes in \c #aszFrameP[]
   */
  int32_t slLengthP;

  /**
   * @brief This flag is set to true when \c #updateFrame() has been called.
   * And it is cleared in \c #parse() when the data has been parsed
   */
  bool btNewFrameP;

  /**
   * @brief Hold number of parsed value, this value also provides number entries within \c #tsMpptDataP[] array.
   */
  int32_t slNumberOfParsedValuesP;

  /**
   * @brief Parse the test frame
   *
   * @param[in] pscTextFrameV
   */
  void parseTable(char *pscTextFrameV);

  //---------------------------------------------------------------------------------------------------
  // parameter extract from frame
  // corresponding to https://www.victronenergy.de/upload/documents/VE.Direct-Protocol-3.33.pdf
  //

  /**
   * @brief Label 'V': Main or channel 1(battery)voltage, given in [V]
   */
  float ftBatteryVoltageP;

  /**
   * @brief Label 'I': Main or channel 1 battery current, given in [A]
   */
  float ftBatteryCurrentP;
};

#endif // PVZ_MPPT_HPP
