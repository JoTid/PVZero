#include <Arduino.h>
#include <unity.h>

#include <battery_guard.hpp>

/**
 * @brief The Control Algorithm class to test
 */
BatteryGuard clGuardG;
unsigned long uqTimeG;
unsigned long uqCallbackTimeG;
int32_t slCallbackEventG;

/*--------------------------------------------------------------------------------------------------------------------*\
** Prepare tests                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
void setUp(void)
{
  // The setUp function can contain anything you would like to run before each test.
}

void tearDown(void)
{
  // The tearDown function can contain anything you would like to run after each test.
}

/*--------------------------------------------------------------------------------------------------------------------*\
** Callbacks                                                                                                          **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

void batteryGuard_TimeStorageCallback(uint64_t uqTimeV)
{
  uqCallbackTimeG = uqTimeV;
  TEST_MESSAGE(String("------------------------- > Battery Guard Time Storage Callback: " + String(uqTimeV)).c_str()); // run as "Verbose Test" to see that
}

void batteryGuard_EventCallback(BatteryGuard::State_te teStateV)
{
  slCallbackEventG = (int32_t)teStateV;
  TEST_MESSAGE(String("------------------------- > Battery Guard Event Callback: " + String(teStateV)).c_str()); // run as "Verbose Test" to see that
}

/*--------------------------------------------------------------------------------------------------------------------*\
** Implement tests cases                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void test_bg_init_parameter(void)
{
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalVoltage());

  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());

  TEST_ASSERT_EQUAL(true, clGuardG.isEnabled());

  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state());
  uqTimeG = 0;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void test_bg_discharging(void)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(42.5);
  clGuardG.updateCurrent(0.0);
  clGuardG.updateTime(uqTimeG += 250);

  slCallbackEventG = -1; // reset callback value

  //---------------------------------------------------------------------------------------------------
  // run process defined time: a change is done each BG_REFRESH_TIME msec
  //
  TEST_MESSAGE(String("Trigger battery guard 6 times.").c_str()); // run as "Verbose Test" to see that
  int32_t slPCyclesT = millis() + BG_REFRESH_TIME * 6 + 1;        // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(42.5, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(42.5, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, clGuardG.state());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, slCallbackEventG);
  TEST_ASSERT_EQUAL(5.0, clGuardG.limitedCurrent(5.0));

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(45.6);
  clGuardG.updateCurrent(0.0);
  clGuardG.updateTime(uqTimeG += 250);

  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1;                // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(42.5, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, clGuardG.state());
  TEST_ASSERT_EQUAL(5.0, clGuardG.limitedCurrent(5.0));

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(41.2);
  clGuardG.updateCurrent(0.0);
  clGuardG.updateTime(uqTimeG += 250);

  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(41.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, clGuardG.state());
  TEST_ASSERT_EQUAL(5.0, clGuardG.limitedCurrent(5.0));
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void test_bg_discharged(void)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(40.5);
  clGuardG.updateCurrent(0.0);
  clGuardG.updateTime(uqTimeG += 250);

  slCallbackEventG = -1; // reset callback value

  //---------------------------------------------------------------------------------------------------
  // run process defined time: a change is done each BG_REFRESH_TIME msec
  //
  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  int32_t slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1;        // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(40.5, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, clGuardG.state());
  TEST_ASSERT_EQUAL(5.0, clGuardG.limitedCurrent(5.0)); // No limitation

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(40.0);
  clGuardG.updateCurrent(0.0);
  clGuardG.updateTime(uqTimeG += 250);

  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1;                // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(40.0, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarged, clGuardG.state());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarged, slCallbackEventG);
  TEST_ASSERT_EQUAL(0.0, clGuardG.limitedCurrent(5.0)); // limitation

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(38.2);
  clGuardG.updateCurrent(0.0);
  clGuardG.updateTime(uqTimeG += 250);

  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(38.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarged, clGuardG.state());
  TEST_ASSERT_EQUAL(0.0, clGuardG.limitedCurrent(5.0));

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(41.2);
  clGuardG.updateCurrent(0.0);
  clGuardG.updateTime(uqTimeG += 250);
  slCallbackEventG = -1; // reset callback value

  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(38.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarged, clGuardG.state());
  TEST_ASSERT_EQUAL(-1, slCallbackEventG); // no callback should be called
  TEST_ASSERT_EQUAL(0.0, clGuardG.limitedCurrent(5.0));

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(42.2);
  clGuardG.updateCurrent(0.0);
  clGuardG.updateTime(uqTimeG += 250);

  slCallbackEventG = -1; // reset callback value

  slPCyclesT = millis() + BG_REFRESH_TIME * 1 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state()); // change to charging state
  TEST_ASSERT_EQUAL(0, (clGuardG.limitedCurrent(5.0) * 10.0));

  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(38.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(0.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, clGuardG.state()); // as the current is still 0 A change to discharging
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, slCallbackEventG);
  TEST_ASSERT_EQUAL(5.0, clGuardG.limitedCurrent(5.0));
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void test_bg_charging(void)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(45.5);
  clGuardG.updateCurrent(1.0);
  clGuardG.updateTime(uqTimeG += 250);
  slCallbackEventG = -1; // reset callback value

  //---------------------------------------------------------------------------------------------------
  // run process defined time: a change is done each BG_REFRESH_TIME msec
  //
  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  int32_t slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1;        // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(38.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(1.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(1.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state());
  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, slCallbackEventG);
  TEST_ASSERT_EQUAL(10, (clGuardG.limitedCurrent(5.0) * 10)); // Limit to battery current + 0.5 A

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(55.0);
  clGuardG.updateCurrent(6.0);
  clGuardG.updateTime(uqTimeG += 250);
  slCallbackEventG = -1; // reset callback value

  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1;                // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(38.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(55.0, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(1.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(6.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state());
  TEST_ASSERT_EQUAL(-1, slCallbackEventG);                    // no callback should be called
  TEST_ASSERT_EQUAL(60, (clGuardG.limitedCurrent(6.0) * 10)); // No limit required as the current is equal or higher

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(58.2);
  clGuardG.updateCurrent(20.0);
  clGuardG.updateTime(uqTimeG += 250);
  slCallbackEventG = -1; // reset callback value

  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(38.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(58.2, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(1.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(20.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state());
  TEST_ASSERT_EQUAL(-1, slCallbackEventG);                    // no callback should be called
  TEST_ASSERT_EQUAL(60, (clGuardG.limitedCurrent(6.0) * 10)); // No limit required as the current is equal or higher
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void test_bg_charged(void)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(58.4);
  clGuardG.updateCurrent(6.0);
  clGuardG.updateTime(uqTimeG += 250);
  slCallbackEventG = -1; // reset callback value
  uqCallbackTimeG = 0;

  //---------------------------------------------------------------------------------------------------
  // run process defined time: a change is done each BG_REFRESH_TIME msec
  //
  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  int32_t slPCyclesT = millis() + BG_REFRESH_TIME * 1 + 1;        // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(38.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(58.4, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(1.0, clGuardG.minimalCurrent());
  TEST_ASSERT_EQUAL(20.0, clGuardG.maximalCurrent());
  TEST_ASSERT_EQUAL(BatteryGuard::eCharged, clGuardG.state());
  TEST_ASSERT_EQUAL(BatteryGuard::eCharged, slCallbackEventG);
  TEST_ASSERT_EQUAL(uqTimeG, uqCallbackTimeG);                // store callback with time has been called
  TEST_ASSERT_EQUAL(50, (clGuardG.limitedCurrent(5.0) * 10)); // No current limit

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(58.4);
  clGuardG.updateCurrent(6.0);
  clGuardG.updateTime(uqTimeG += 250);
  uqCallbackTimeG = 0;

  slCallbackEventG = -1; // reset callback value

  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1;                // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(BatteryGuard::eCharged, clGuardG.state());
  TEST_ASSERT_EQUAL(-1, slCallbackEventG);                      // no callback should be called
  TEST_ASSERT_EQUAL(0, uqCallbackTimeG);                        // no callback should be called
  TEST_ASSERT_EQUAL(120, (clGuardG.limitedCurrent(12.0) * 10)); // No current limit

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(58.2);
  clGuardG.updateCurrent(6.0);
  clGuardG.updateTime(uqTimeG += 250);

  slPCyclesT = millis() + BG_REFRESH_TIME * 1 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, clGuardG.state());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, slCallbackEventG);

  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state()); // as the current is still > 0 change to charging state again
  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, slCallbackEventG);
  TEST_ASSERT_EQUAL(60, (clGuardG.limitedCurrent(6.0) * 10)); // No current limit
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void test_bg_force_charged(void)
{
  //---------------------------------------------------------------------------------------------------
  // make sure we are in charging state
  //
  clGuardG.updateVoltage(55.4);
  clGuardG.updateCurrent(6.0);
  clGuardG.updateTime(uqTimeG += 250);
  slCallbackEventG = -1; // reset callback value
  uqCallbackTimeG = 0;

  //---------------------------------------------------------------------------------------------------
  // run process defined time: a change is done each BG_REFRESH_TIME msec
  //
  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  int32_t slPCyclesT = millis() + BG_REFRESH_TIME * 3 + 1;        // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state());
  TEST_ASSERT_EQUAL(-1, slCallbackEventG);                    // no state change, so no event
  TEST_ASSERT_EQUAL(50, (clGuardG.limitedCurrent(5.0) * 10)); // No current limit

  //---------------------------------------------------------------------------------------------------
  // now increase the time to more than two weeks
  //
  clGuardG.updateVoltage(55.4);
  clGuardG.updateCurrent(0.0); // there is no current
  clGuardG.updateTime(uqTimeG += ((uint64_t)250 + (uint64_t)BG_FULL_CHARGE_REPETITION_TIME));
  uqCallbackTimeG = 0;

  slCallbackEventG = -1; // reset callback value

  TEST_MESSAGE(String("Trigger battery guard 2 times.").c_str()); // run as "Verbose Test" to see that
  slPCyclesT = millis() + BG_REFRESH_TIME * 2 + 1;                // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state()); // we should stay in charging state

  TEST_ASSERT_EQUAL(-1, slCallbackEventG);                    // no callback should be called
  TEST_ASSERT_EQUAL(0, uqCallbackTimeG);                      // no callback should be called
  TEST_ASSERT_EQUAL(0, (clGuardG.limitedCurrent(12.0) * 10)); // No current limit

  //---------------------------------------------------------------------------------------------------
  // change to charged state
  //
  clGuardG.updateVoltage(58.4);
  clGuardG.updateCurrent(6.0);
  clGuardG.updateTime(uqTimeG += 250);

  slPCyclesT = millis() + BG_REFRESH_TIME * 1 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }
  TEST_ASSERT_EQUAL(BatteryGuard::eCharged, clGuardG.state());
  TEST_ASSERT_EQUAL(BatteryGuard::eCharged, slCallbackEventG);
  TEST_ASSERT_EQUAL(uqTimeG, uqCallbackTimeG);                  // store callback with time has been called
  TEST_ASSERT_EQUAL(120, (clGuardG.limitedCurrent(12.0) * 10)); // no limit, as the battery is charged use those current

  clGuardG.updateVoltage(54.4);
  clGuardG.updateCurrent(20.0);

  slPCyclesT = millis() + BG_REFRESH_TIME * 1 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, clGuardG.state());
  TEST_ASSERT_EQUAL(BatteryGuard::eDischarging, slCallbackEventG);
  TEST_ASSERT_EQUAL(120, (clGuardG.limitedCurrent(12.0) * 10)); // no limit

  clGuardG.updateVoltage(52.4);
  clGuardG.updateCurrent(7.0);

  slPCyclesT = millis() + BG_REFRESH_TIME * 1 + 1; // trigger algorithm x times
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, clGuardG.state()); // as the current is still > 0 change to charging state again
  TEST_ASSERT_EQUAL(BatteryGuard::eCharging, slCallbackEventG);
  TEST_ASSERT_EQUAL(60, (clGuardG.limitedCurrent(6.0) * 10)); // limit to available current
}

/*--------------------------------------------------------------------------------------------------------------------*\
** Test environment                                                                                                   **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void setup()
{
  //---------------------------------------------------------------------------------------------------
  // initialise the LCD and trigger first display
  //
  clGuardG.init(0, batteryGuard_TimeStorageCallback);
  clGuardG.installEventHandler(batteryGuard_EventCallback);

  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  UNITY_BEGIN(); // IMPORTANT LINE!

  RUN_TEST(test_bg_init_parameter);
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void loop()
{
  //---------------------------------------------------------------------------------------------------
  // run tests step by step,
  //
  RUN_TEST(test_bg_discharging);
  RUN_TEST(test_bg_discharged);
  RUN_TEST(test_bg_charging);
  RUN_TEST(test_bg_charged);

  RUN_TEST(test_bg_force_charged); // when there was no charged state after two weeks

  UNITY_END(); // stop unit testing
  while (1)
  {
  };
}