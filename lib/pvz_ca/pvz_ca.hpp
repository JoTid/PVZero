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
#include <Arduino.h>


/*--------------------------------------------------------------------------------------------------------------------*\
** Declaration                                                                                                        **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
class PvzCa
{
private:
  float ftConsumptionPowerP;
  float ftFeedInPowerP;
  float ftFeedInDcCurrentP;
  float ftFeedInDcVoltageP;
  float ftFilterOrderP;
  float aftConsumptionPowerLimitP[2]; // index 0 contains min. value and index 1 contains max. value 
  float aftFeedInDcCurrentLimitP[2];  // index 0 contains min. value and index 1 contains max. value 
  
  void CalculateGainOffset(void);
  float ftCurrentGainP;
  float ftCurrentOffsetP;

public:
  PvzCa(void);
  ~PvzCa(void);

  /**
   * @brief Initialisation of Control Algorithm 
   * This method should be called only once while setup.
   */
  void init(void);

  /**
   * @brief Process the Control Algorithm.
   * This method must be called from main loop.
   */
  void process(void);

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
   * @brief Set the Consumption Power Limits 
   * 
   * @param ftMinV minimal power value that should be considered, given in [Wh] 
   * @param ftMaxV maximal power value that should be considered, given in [Wh]
   * @return 0 in case of success or a negative value in case of an error
   * 
   * This methods provides limit values for consumption power that should be considered by the control algorithm.
   * These values have an influence on the value set in \c #setConsumptionPower(). In this method, the current value is 
   * limited by \c #ftMinB and \c #ftMaxV.
   * 
   * Make sure that ftMinV value is smaller than ftMaxV, in other case -1 is returned.
   * 
   * Default values are set in \c #init() to 0.0 Wh and 600.0 Wh
   */
  // int32_t setConsumptionPowerLimits(float ftMinV, float ftMaxV);

  // /**
  //  * @brief Set the Feed In Power Limits
  //  * 
  //  * @param ftMinV minimal power value, given in [Wh] 
  //  * @param ftMaxV maximal power value, given in [Wh]
  //  * @return 0 in case of success or a negative value in case of an error
  //  */
  // int32_t setFeedInPowerLimits(float, float);

  /**
   * @brief Set the Feed In Current Limits
   * 
   * @return 0 in case of success or a negative value in case of an error
   */
  int32_t setFeedInDcCurrentLimits(float ftMinV, float ftMaxV);

  /**
   * @brief Set the Feed In Current Limits
   * 
   * @return 0 in case of success or a negative value in case of an error
   */
  int32_t setFeedInDcVoltage(float);

  /**
   * @brief Set the Consumption Power 
   * 
   * Accept all values and limit them corresponding to the limits provided by the 
   * \c #setConsumptionPowerLimits() method. 
   * This method update only the value, the calculation is performed  by the \c process() method.
   * 
   * The currently set and modified value can be queried using the \c #consumptionPower() method.
   */
  void setConsumptionPower(float);


  //--------------------------------------------------------------------------------------------------- 
  // getter methods
  //
  float consumptionPower(void) {
    return ftConsumptionPowerP;
  }

  float feedInDcCurrent(void) {
    return ftFeedInDcCurrentP;
  }

  float feedInDcVoltage(void) {
    return ftFeedInDcVoltageP;
  }

  float feedInPower(void) {
    return ftFeedInPowerP;
  }

  uint8_t filterOrder(void) {
    return (uint8_t)ftFilterOrderP;
  }

};

#endif // PVZ_CA_HPP
