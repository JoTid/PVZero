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

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void test_bg_current_limit(void)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  clGuardG.updateVoltage(48.0);

  //---------------------------------------------------------------------------------------------------
  // run process defined time: a change is done each BG_REFRESH_TIME msec
  //
  int32_t slPCyclesT = millis() + (BG_REFRESH_TIME * 1) + 1;
  while (slPCyclesT > millis())
  {
    clGuardG.process();
  }

  //---------------------------------------------------------------------------------------------------
  // check the current limit is calculated correct
  //
  TEST_MESSAGE(String("Limited Current of 3.0 A @ 48.0 V: " + String(clGuardG.limitedCurrent(3.0), 1) + "A").c_str()); // run as "Verbose Test" to see that
  TEST_ASSERT_EQUAL(3.0, clGuardG.limitedCurrent(3.0));
  TEST_MESSAGE(String("Limited Current of 13.0 A @ 48.0 V: " + String(clGuardG.limitedCurrent(13.0), 1) + "A").c_str()); // run as "Verbose Test" to see that
  TEST_ASSERT_EQUAL(3.3, clGuardG.limitedCurrent(13.0));

  //---------------------------------------------------------------------------------------------------
  // just print limits for different supply voltages
  //
  float ftVoltage = 40.0;
  while (ftVoltage < 60.0)
  {
    clGuardG.updateVoltage(ftVoltage);
    TEST_MESSAGE(String("Limited Current at " + String(ftVoltage, 1) + "V is " + String(clGuardG.limitedCurrent(200.0), 1) + "A").c_str()); // run as "Verbose Test" to see that
    ftVoltage += 1.0;
  }
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
  // RUN_TEST(test_bg_init_parameter);
  // RUN_TEST(test_bg_normal_operating);
  // RUN_TEST(test_bg_alarm_set);
  // RUN_TEST(test_bg_alarm_recover);

  RUN_TEST(test_bg_current_limit);

  UNITY_END(); // stop unit testing
  while (1)
  {
  };
}