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
 * [V] as fixed comma value with 1 DD
 */
#define BG_NOMINAL_VOLTAGE 512

/**
 * [Ah] as fixed comma value with 1 DD
 */
#define BG_NOMINAL_POWER 500

/**
 * [V] as fixed comma value with 1 DD
 */
#define BG_ABSORPTION_VOLTAGE 512

/**
 * [V] as fixed comma value with 1 DD
 */
#define BG_CHARGE_CUTOFF_VOLTAGE 584

/**
 * [V] as fixed comma value with 1 DD
 */
#define BG_DISCHARGE_VOLTAGE 400

/**
 * [A] as fixed comma value with 1 DD
 */
#define BG_CHARGE_CURRENT_MAXIMAL 250

/**
 * [A] as fixed comma value with 1 DD
 */
#define BG_CHARGE_CURRENT_NOMINAL 500

/**
 * [A] as fixed comma value with 1 DD
 */
#define BG_DISCHARGE_CURRENT_MAXIMAL 500

/**
 * Refresh time of the battery guard
 */
#define BG_REFRESH_TIME 1000

/*--------------------------------------------------------------------------------------------------------------------*\
** Declaration                                                                                                        **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
class BatteryGuard
{

private:
  bool btDischargeAlarmP;
  float ftActualVoltageP;
  int32_t slActualVoltageP;
  int32_t slMinVoltageP;
  int32_t slMaxVoltageP;

  //---------------------------------------------------------------------------------------------------
  // that float values are used to calculate the gain and offset to determine the maximal current
  // depending on the PSUs supply voltage.
  //
  // Here is a sample calculation:
  // At 41 V supply voltage, the current should be limited to 0.01 A.
  // If the supply voltage is 58.4 V, the current limit can be set to 12.0 A.
  //
  // This results in the following values: y = m * x + b => y = 0.68908046 * x + -28.24229885
  //   Voltage [V]    Current Limit [A]      max. Feed In Power
  //      41              0.01               ~    0.4 Wh = 40 V * 0.01 A
  //      45              2.76               ~ 121.44 Wh = 44 V * 2.76 A
  //      48              4.83               ~ 227.01 Wh = 47 V * 4.83 A
  //      50              6.21               ~ 304.29 Wh = 49 V * 6.21 A
  //      52              7.59               ~ 379.50 Wh = 50 V * 7.59 A   // Voltage is limited by PUS to 50 V and the current will be limited by the inverter input
  //      58             11.72               ~ 586.00 Wh = 50 V * 11.72 A  // Voltage is limited by PUS to 50 V and the current will be limited by the inverter input
  //
  float ftNominalVoltageMinP; // 41.00 V
  float ftNominalVoltageMaxP; // 58.40 V
  float ftNominalCurrentMinP; //  0.01 A
  float ftNominalCurrentMaxP; // 12.00 A

  float ftScaleGainP;
  float ftScaleOffsetP;

  int32_t slRecoverVoltageP;

  uint32_t ulMinimumChargingTimeP;

  // This value is set to ulMinimumChargingTimeP when at alarm and is decremented when actual voltage is in valid range
  // value is given is [sec]
  uint32_t ulChargingTimeP;

  uint32_t ulAlarmPendingTimeP;

  bool btEnabledP;

public:
  BatteryGuard();
  ~BatteryGuard();

  void enable(bool btStateV) { btEnabledP = btStateV; }

  /**
   * @brief Initialisation of Control Algorithm
   * @param[in] ftOperatingVoltageV This is the required operating voltage [V] so that the discharge alarm can be reset.
   * @param[in] ulMinimalChargingTimeV This is the minimum time [sec] that the operating voltage must be applied before
   *                                   the discharge alarm is reset.
   *
   * Initially, after power up or reset the battery guard module starts with valid values so the alarm is not set.
   *
   * When the actual voltage falls below the discharge voltage (defined by \c #BG_DISCHARGE_VOLTAGE),
   * the discharge alarm is set and the charge time is cleared.
   *
   * In order for the discharge alarm to be reset, the operating voltage must be present for the minimum charging time.
   * The charging time only runs if the actual voltage is greater than or equal to the provided operating voltage.
   */
  void init(float ftRecoverVoltageV, uint32_t ulRecoverTimeV);

  /**
   * @brief Process actual battery voltage and update the state the battery guard is true or false
   *
   * This method must be called from main loop.
   */
  void process();

  /**
   * @brief update actual Feed In values for Voltage and Current
   *
   * @param[in] ftVoltageV actual pending feed-in DC voltage given in [V]
   * @return 0 in case of success or a negative value in case of an error
   */
  void updateVoltage(float ftVoltageV);

  float limitedCurrent(float ftFeedTargetInCurrentV);
  //---------------------------------------------------------------------------------------------------
  // getter methods
  //
  float maximalVoltage()
  {
    return ((float)(slMaxVoltageP) / 10.0);
  }

  float minimalVoltage()
  {
    return ((float)(slMinVoltageP) / 10.0);
  }

  bool alarm()
  {
    return btDischargeAlarmP;
  }

  uint32_t alarmRecoverTime()
  {
    return ulChargingTimeP;
  }
  float alarmRecoverVoltage()
  {
    return ((float)(slRecoverVoltageP) / 10.0);
  }
  uint32_t alarmPendingTime()
  {
    return ulAlarmPendingTimeP;
  }
  bool isEnabled()
  {
    return btEnabledP;
  }
};

#endif // BATTERY_GUARD_HPP_
