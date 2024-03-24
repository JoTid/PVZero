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
  // int32_t init(HardwareSerial &clSerialR);

  /**
   * @brief Provide new received Frame to the MPPT class.
   * The frame is parsed and the values are then available
   * protected by mutex via \c #batteryVoltage() and \c #batteryCurrent()
   * @param[in] pszFrameV Pointer to the frame
   * @param[in] slLengthV Length of the frame given in bytes
   */
  void updateFrame(const char *pszFrameV, int32_t slLengthV);

  //---------------------------------------------------------------------------------------------------
  // get method for parameter extracted from frame
  //

  /**
   * @brief Pending battery voltage.
   * Protected by mutex against competing calls in \c #updateFrame()
   * @return float value given in [V]
   */
  float batteryVoltage();
  /**
   * @brief Pending battery current
   * Protected by mutex against competing calls in \c #updateFrame()
   * @return float value given in [A]
   */
  float batteryCurrent();

  uint8_t stateOfOperation();

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
   *        And parse it later using \c #parseTable() method.
   */
  char ascFrameP[MPPT_TEXT_FRAME_LENGTH];

  /**
   * @brief holds number of valid bytes in \c #aszFrameP[]
   */
  int32_t slLengthP;

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

  uint8_t ubStateOfOperationP;

  std::mutex mpptMutexP;
};

#endif // PVZ_MPPT_HPP
