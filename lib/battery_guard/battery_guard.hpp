//====================================================================================================================//
// File:          battery_guard.hpp                                                                                   //
// Description:   PhotoVoltaics Zero - Battery guard                                                                  //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//


#ifndef BATTERY_GUARD_HPP_
#define BATTERY_GUARD_HPP_


/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/ 
#include <stdio.h>
#include <cstdint>


/*--------------------------------------------------------------------------------------------------------------------*\
** Defines                                                                                                            **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/ 

//--------------------------------------------------------------------------------------------------------- 
// Use value from 48V_50Ah_LiFePO4_Lithium_Batterie data sheet
//

/**
 * [V]
 */
#define BG_NOMINAL_VOLTAGE    51.2

/**
 * [Ah]
 */
#define  BG_NOMINAL_POWER      50.0

/**
 * [V]
 */
#define  BG_ABSORPTION_VOLTAGE   51.2

/**
 * [V]
 */
#define  BG_CHARGE_CUTOFF_VOLTAGE   58.4

/**
 * [V]
 */
#define  BG_DISCHARGE_VOLTAGE   40.0

/**
 * [A]
 */
#define BG_CHARGE_CURRENT_MAXIMAL 25.0

/**
 * [A]
 */
#define BG_CHARGE_CURRENT_NOMINAL 50.0

/**
 * [A]
 */
#define BG_DISCHARGE_CURRENT_MAXIMAL 50.0


/*--------------------------------------------------------------------------------------------------------------------*\
** Declaration                                                                                                        **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
class BatteryGuard
{

private:
  #define BG_REFRESH_TIME 500

  bool btDisconnectP;

public:
  BatteryGuard(void);
  ~BatteryGuard(void);

  /**
   * @brief Initialisation of Control Algorithm 
   * This method should be called only once while setup.
   */
  void init(void);

  /**
   * @brief Process actual battery voltage and update the state the battery guard is true or false
   * 
   * This method must be called from main loop.
   */
  void process(void);

  /**
   * @brief update actual Feed In values for Voltage and Current
   * 
   * @param[in] ftVoltageV actual pending feed-in DC voltage given in [V]
   * @return 0 in case of success or a negative value in case of an error
   */
  void updateVoltage(float ftVoltageV);

  //--------------------------------------------------------------------------------------------------- 
  // getter methods
  //
  float maximalVoltage(void) {
    return 0;
  }

  float minimalVoltage(void) {
    return 0;
  }

  bool disconnect(void) {
    return false;
  }

};

#endif // BATTERY_GUARD_HPP_
