#include <Arduino.h>
#include <unity.h>

#include <battery_guard.hpp>

/**
 * @brief The Control Algorithm class to test
 */
BatteryGuard clGuardG;

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
** Implement tests cases                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
void test_bg_init_parameter(void)
{
  TEST_ASSERT_EQUAL(false, clGuardG.alarm());
  TEST_ASSERT_EQUAL(BG_CHARGE_CUTOFF_VOLTAGE / 10, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(BG_DISCHARGE_VOLTAGE / 10, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0, clGuardG.alarmRecoverTime());
}

void test_bg_normal_operating(void)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(42.5);

  int32_t slPCyclesT = millis() + 1100;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(false, clGuardG.alarm());
  TEST_ASSERT_EQUAL(42.5, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(42.5, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0, clGuardG.alarmRecoverTime());

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(45.6);

  slPCyclesT = millis() + 1100;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(false, clGuardG.alarm());
  TEST_ASSERT_EQUAL(42.5, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0, clGuardG.alarmRecoverTime());

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(41.2);

  slPCyclesT = millis() + 1100;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(false, clGuardG.alarm());
  TEST_ASSERT_EQUAL(41.2, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0, clGuardG.alarmRecoverTime());
}

void test_bg_alarm_set(void)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(39.9);

  int32_t slPCyclesT = millis() + 1100;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(true, clGuardG.alarm());
  TEST_ASSERT_EQUAL(39.9, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(10, clGuardG.alarmRecoverTime());

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(35.0);

  slPCyclesT = millis() + 1100;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(true, clGuardG.alarm());
  TEST_ASSERT_EQUAL(35.0, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(10, clGuardG.alarmRecoverTime());
}

void test_bg_alarm_recover(void)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(42.0);

  int32_t slPCyclesT = millis() + 2200;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(true, clGuardG.alarm());
  TEST_ASSERT_EQUAL(35.0, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(10, clGuardG.alarmRecoverTime());

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(43.0);

  slPCyclesT = millis() + 1100;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(true, clGuardG.alarm());
  TEST_ASSERT_EQUAL(35.0, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(10, clGuardG.alarmRecoverTime());

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(44.0);

  slPCyclesT = millis() + 1100;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(true, clGuardG.alarm());
  TEST_ASSERT_EQUAL(35.0, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(45.6, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(9, clGuardG.alarmRecoverTime());

  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(51.0);

  slPCyclesT = millis() + 10000;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  TEST_ASSERT_EQUAL(false, clGuardG.alarm());
  TEST_ASSERT_EQUAL(35.0, clGuardG.minimalVoltage());
  TEST_ASSERT_EQUAL(51.0, clGuardG.maximalVoltage());
  TEST_ASSERT_EQUAL(0, clGuardG.alarmRecoverTime());
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
  clGuardG.init(44.0, 10);

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
  RUN_TEST(test_bg_init_parameter);
  RUN_TEST(test_bg_normal_operating);
  RUN_TEST(test_bg_alarm_set);
  RUN_TEST(test_bg_alarm_recover);

  UNITY_END(); // stop unit testing
  while (1)
  {
  };
}