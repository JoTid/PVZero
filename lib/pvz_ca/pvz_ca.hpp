//====================================================================================================================//
// File:          pvz_ca.hpp                                                                                          //
// Description:   PhotoVoltaics Zero - Control Algorithm                                                              //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

#ifndef PVZ_CA_HPP
#define PVZ_cA_HPP

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
// #include <Arduino.h>
#include <stdio.h>
#include <cstdint>
#include <mov_av_filter.hpp>

#ifndef CA_APPROX_STEP
#define CA_APPROX_STEP 1.0
#endif

#ifndef CA_REFRESH_TIME
#define CA_REFRESH_TIME 3000
#endif

/*--------------------------------------------------------------------------------------------------------------------*\
** Declaration                                                                                                        **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
class PvzCa
{
private:
  bool btConsumptionPowerPendingP;
  bool btActualValuesPendingP;

  float ftConsumptionPowerOffsetP;

  float ftConsumptionPowerP;

  float ftFeedInTargetPowerP;
  float ftFeedInTargetDcCurrentP;
  float ftFeedInTargetDcVoltageP;

  float ftFeedInTargetPowerApproxP;

  float ftFeedInActualPowerP;
  float ftFeedInActualDcCurrentP;
  float ftFeedInActualDcVoltageP;

  uint8_t ubFilterOrderP;

  float aftConsumptionPowerLimitP[2];      // index 0 contains min. value and index 1 contains max. value
  float aftFeedInTargetDcCurrentLimitP[2]; // index 0 contains min. value and index 1 contains max. value

  void CalculateGainOffset();
  float ftCurrentGainP;
  float ftCurrentOffsetP;

  MovAvFilter clConsPowerFilterP;

public:
  PvzCa();
  ~PvzCa();

  /**
   * @brief Initialisation of Control Algorithm
   * @param[in] ubStringCountT Number of strings, that should be considered
   * This method should be called only once while setup.
   */
  void init();

  /**
   * @brief Process the Control Algorithm.
   * This method must be called from main loop.
   */
  void process();

  /**
   * @brief Set the Filter Order object
   *
   * Valid values are 1..40
   *
   * @return 0 in case of success or a negative value in case of an error
   *
   */
  int32_t setFilterOrder(uint8_t);

  /**
   * @brief Set consumption power offset
   *
   * This value can be adjusted if we do not want to fall under the negative consumption value.
   * By setting this value to 50.0 Wh than this value wll be the new zero line.
   *
   * @return 0 in case of success or a negative value in case of an error
   *
   */
  int32_t setConsumptionPowerOffset(float ftValueV);

  /**
   * @brief update actual Feed In values for Voltage and Current
   *
   * @param[in] ftVoltageV actual pending feed-in DC voltage given in [V]
   * @param[in] ftCurrentV actual pending feed-in DC current given in [A]
   * @return 0 in case of success or a negative value in case of an error
   */
  int32_t updateFeedInActualDcValues(float ftVoltageV, float ftCurrentV);

  /**
   * @brief Set the Feed-in DC Current Limit values
   *
   * @param[in] ftMinV minimal target feed-in DC current value given in [A]
   * @param[in] ftMaxV maximal target feed-in DC current value given in [A]
   * @return 0 in case of success or a negative value in case of an error
   */
  int32_t setFeedInTargetDcCurrentLimits(float ftMinV, float ftMaxV);

  /**
   * @brief Set the Feed-in DC Voltage value
   *
   * @param[in] ftVoltageV DC voltage value for feed-in given in [V]
   * @return 0 in case of success or a negative value in case of an error
   */
  int32_t setFeedInTargetDcVoltage(float ftVoltageV);

  /**
   * @brief update the Consumption Power
   *
   * @param[in] ftPowerV actual consumption power value given in [Wh]
   * @return filtered value
   *
   * Accept all values and limit them corresponding to the limits provided by the
   * \c #updateConsumptionPowerLimits() method.
   * This method update only the value, the calculation is performed  by the \c process() method.
   *
   * The currently set and modified value can be queried using the \c #consumptionPower() method.
   */
  float updateConsumptionPower(float ftPowerV);

  float discreteApproximation(float ftActualV, float ftTargetV);

  //---------------------------------------------------------------------------------------------------
  // getter methods
  //
  float consumptionPower(void) { return ftConsumptionPowerP; }
  float feedInTargetDcCurrent(void) { return ftFeedInTargetDcCurrentP; }
  float feedInTargetDcCurrentMin(void) { return aftFeedInTargetDcCurrentLimitP[0]; }
  float feedInTargetDcCurrentMax(void) { return aftFeedInTargetDcCurrentLimitP[1]; }
  float feedInTargetDcVoltage(void) { return ftFeedInTargetDcVoltageP; }
  float feedInTargetPower(void) { return ftFeedInTargetPowerP; }
  float feedInTargetPowerApprox(void) { return ftFeedInTargetPowerApproxP; }
  float feedInActualDcCurrent(void) { return ftFeedInActualDcCurrentP; }
  float feedInActualDcVoltage(void) { return ftFeedInActualDcVoltageP; }
  float feedInActualPower(void) { return ftFeedInActualPowerP; }
  uint8_t filterOrder(void) { return ubFilterOrderP; }
};

#endif // PVZ_CA_HPP
