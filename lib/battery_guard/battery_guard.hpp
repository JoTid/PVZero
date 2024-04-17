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
 * Value in batter is defined to 40V.
 * Define a value that has an offset.
 */
#define BG_DISCHARGE_VOLTAGE 486

/**
 * [V] as fixed comma value with 1 DD
 * This is the offset required for the voltage to rise before the discharge state is exited.
 */
#define BG_DISCHARGE_VOLTAGE_OFFSET 60

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

/**
 * @brief Time specified in [sec] in which the battery should be fully charged once
 *
 * In case the battery should be loaded each 14 days this value is calculated as:
 * define = (14 days, 24 hours, 60 minutes, 60 seconds)
 */
#define BG_FULL_CHARGE_REPETITION_TIME (14 * 24 * 60 * 60)

/*--------------------------------------------------------------------------------------------------------------------*\
** Declaration                                                                                                        **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
class BatteryGuard
{

public:
  typedef enum State_e
  {

    eCharge = 0,
    eChargeAndDischarge,
    eChargeUntilCharged,
    eCharged,
    eDischarge,
    eDischarged

  } State_te;

  /**
   * @brief     This handler is called up as soon as the time value needs to be saved
   * @param[in] uqTimeV Unix time given in [sec] since 01.01.1970, that should be saved
   *
   * This callback is called when the the full charge of battery has been reached
   * that value should be provided a call of init()
   *
   */
  typedef std::function<void(uint64_t uqTimeV)> SaveTimeHandler_fn;
  typedef std::function<void(State_te teStateV)> EventHandler_fn;

private:
  float ftBatteryVoltageP;          // float value for calculation
  int32_t slBatteryVoltageP;        // int value for comparison with 1 DD
  int32_t slBatteryVoltageMinimalP; // int value for comparison with 1 DD
  int32_t slBatteryVoltageMaximalP; // int value for comparison with 1 DD

  int32_t slBatteryCurrentP;        // int value for comparison with 2 DD
  int32_t slBatteryCurrentMinimalP; // int value for comparison with 2 DD
  int32_t slBatteryCurrentMaximalP; // int value for comparison with 2 DD

  uint8_t ubMpptStateOfOperationP;
  float ftTargetCurrentP;
  float ftLimitedCurrentP;

  bool btEnabledP;

  State_te teStateP;
  State_te teStateOldP;

  EventHandler_fn pfnEventHandlerP;
  SaveTimeHandler_fn pfnSaveTimeHandlerP;

  String aclAddStateInfoP[6];

  // actual unix time given in [sec] since 01.01.1970
  uint64_t uqTimeP;
  // unix time in [sec] since 01.01.1970 when the battery was last fully charged
  uint64_t uqFullyChargedTimeP;

public:
  BatteryGuard();
  ~BatteryGuard();

  void enable(bool btStateV) { btEnabledP = btStateV; }

  State_te state() { return teStateP; }

  /**
   * @brief Initialisation of Control Algorithm
   * @param[in] uqTimeV Stored unix time given in [sec] since 01.01.1970
   * @param[in] pfnStoreTimeHandlerV The callback to store time when the battery has been fully charged
   */
  void init(uint64_t uqTimeV, SaveTimeHandler_fn pfnStoreTimeHandlerV);

  /**
   * @brief Install a callback handler to monitor actual state of the battery guard
   *
   * @param[in] pfnEventHandlerV The callback handler
   */
  void installEventHandler(EventHandler_fn pfnEventHandlerV);

  /**
   * @brief Process actual battery voltage and update the state the battery guard is true or false
   *
   * This method must be called from main loop.
   */
  void process();

  /**
   * @brief update actual charge voltage
   *
   * @param[in] ftVoltageV actual battery voltage
   *
   * The charge voltage is the voltage that is typically pending at the battery
   */
  void updateVoltage(float ftVoltageV);

  /**
   * @brief update actual charge current
   * @param[in] ftCurrentV actual battery current
   *
   * The charge current is the current that is typically flow to the battery and consumer
   */
  void updateCurrent(float ftCurrentV);
  void updateMpptState(uint8_t ubStateV);

  /**
   * @brief Update time
   *
   * @param[in] uqTimeV Unix time given in [sec] since 01.01.1970
   */
  void updateTime(uint64_t uqTimeV);

  /**
   * @brief This method returns limited current, that depends on battery guard parameters
   *
   * @param[in] ftTargetCurrentV
   * @return    float current value that has been evaluated from \c #ftTargetCurrentV
   */
  float limitedCurrent(float ftTargetCurrentV);

  //---------------------------------------------------------------------------------------------------
  // getter methods
  //
  float maximalVoltage() { return ((float)(slBatteryVoltageMaximalP) / 10.0); }

  float minimalVoltage() { return ((float)(slBatteryVoltageMinimalP) / 10.0); }

  float maximalCurrent() { return ((float)(slBatteryCurrentMaximalP) / 100.0); }

  float minimalCurrent() { return ((float)(slBatteryCurrentMinimalP) / 100.0); }

  bool isEnabled() { return btEnabledP; }

  String stateInfo(State_te teStateV) { return aclAddStateInfoP[teStateV]; }
};

#endif // BATTERY_GUARD_HPP_
