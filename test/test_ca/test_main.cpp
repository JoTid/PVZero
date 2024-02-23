#include <Arduino.h>
#include <unity.h>

#include <pvz_ca.hpp>

/**
 * @brief The Control Algorithm class to test
 */
PvzCa clCaG;

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
void test_ca_init_parameter(void)
{
  
  TEST_ASSERT_EQUAL(0.0, clCaG.consumptionPower());

  TEST_ASSERT_EQUAL(0.0, clCaG.feedInTargetPower());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInTargetDcCurrentMin());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInTargetDcCurrentMax());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInTargetDcVoltage());

  TEST_ASSERT_EQUAL(0.0, clCaG.feedInActualPower());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInActualDcCurrent());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInActualDcVoltage());  

  TEST_ASSERT_EQUAL(1, clCaG.filterOrder());

  //--------------------------------------------------------------------------------------------------- 
  // perform setup and check value have been set
  //
  clCaG.setFeedInTargetDcVoltage(36.0);
  clCaG.setFeedInTargetDcCurrentLimits(0.0, 9.0);

  TEST_ASSERT_EQUAL(0.0, clCaG.feedInTargetDcCurrentMin());
  TEST_ASSERT_EQUAL(9.0, clCaG.feedInTargetDcCurrentMax());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());

}

void test_ca_filter(void)
{
  clCaG.setFilterOrder(1);
  TEST_ASSERT_EQUAL(0.0, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(100.0, clCaG.updateConsumptionPower(100.0));
  TEST_ASSERT_EQUAL(-100.0, clCaG.updateConsumptionPower(-100.0));
  TEST_ASSERT_EQUAL(-100.0, clCaG.updateConsumptionPower(-100.0));
  TEST_ASSERT_EQUAL(395.3, clCaG.updateConsumptionPower(395.3));
  TEST_ASSERT_EQUAL(0.0, clCaG.updateConsumptionPower(0.0));

  clCaG.setFilterOrder(2);
  TEST_ASSERT_EQUAL(200.0, clCaG.updateConsumptionPower(200.0)); // first value after filter setup is not filtered
  TEST_ASSERT_EQUAL(100.0, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(50.0, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(25.0, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(12.0, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(6.0, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(3.0, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(1.5, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(0.85, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(0.0, clCaG.updateConsumptionPower(0.0));
  clCaG.setFilterOrder(5);
  TEST_ASSERT_EQUAL(0.0, clCaG.updateConsumptionPower(0.0));
  TEST_ASSERT_EQUAL(20.0, clCaG.updateConsumptionPower(100.0));
  TEST_ASSERT_EQUAL(36.0, clCaG.updateConsumptionPower(100.0));
  TEST_ASSERT_EQUAL(48.0, clCaG.updateConsumptionPower(100.0));
  TEST_ASSERT_EQUAL(59.0, clCaG.updateConsumptionPower(100.0));
  clCaG.setFilterOrder(1);
}

void test_ca_set_power_to_min_at_start(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *            current [A] = x     consumption [Wh] 
   * min             0    => 0 * 36 =      0   
   * max             9    => 9 * 36 =     324
   * => gain = 9 / 324 = 0,0278; offset = 0 
   * 
   */

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.updateFeedInActualDcValues(0.0, 0.0);
  TEST_ASSERT_EQUAL(clCaG.updateConsumptionPower(0.0), 0);

  //--------------------------------------------------------------------------------------------------- 
  // run process defined number before test
  // 
  int32_t slPCyclesT = 1;
  while (slPCyclesT > 0)
  {
    clCaG.process();
    slPCyclesT--;
  }

  //--------------------------------------------------------------------------------------------------- 
  // check expected values
  // 
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInTargetPower());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInTargetDcCurrent());
}



void test_ca_set_power_to_max_at_start(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *            current [A] = x     consumption [Wh] 
   * min             0    => 0 * 36 =      0   
   * max             9    => 9 * 36 =     324
   * => gain = 9 / 324 = 0,0278; offset = 0 
   */ 

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.updateFeedInActualDcValues(0.0, 0.0);
  TEST_ASSERT_EQUAL(clCaG.updateConsumptionPower(600.0), 600);

  //--------------------------------------------------------------------------------------------------- 
  // run process defined time: a change from 0A to 9A is done in 0,5A/0,5sec steps, so we need 9 sec
  // 
  int32_t slPCyclesT = millis() + 5700;
  while (slPCyclesT >  millis())
  {
    clCaG.process();

    // simulate the actual value so the "discrete approximation" is considered here
    clCaG.updateFeedInActualDcValues(36.0, clCaG.feedInTargetDcCurrent());
  }
  TEST_MESSAGE("STOP -------------------"); // run as "Verbose Test" to see that
  String clStringT = String(" actual: " + String(clCaG.feedInActualDcCurrent()) + " target: " + String(clCaG.feedInTargetDcCurrent()));
  TEST_MESSAGE(clStringT.c_str()); // run as "Verbose Test" to see that

  //--------------------------------------------------------------------------------------------------- 
  // check expected values:  => y=mx+b : DcInCurrent = 600 * 0,0278 + 0 = 9 A
  //                         => P=UI : 36 V * 9 A = 324 W
  TEST_ASSERT_EQUAL(5.5, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());
  TEST_ASSERT_EQUAL(5.5 * 36.0, clCaG.feedInTargetPower());

  slPCyclesT = millis() + 4000;
  while (slPCyclesT >  millis())
  {
    clCaG.process();

    // simulate the actual value so the "discrete approximation" is considered here
    clCaG.updateFeedInActualDcValues(36.0, clCaG.feedInTargetDcCurrent());
  }
  TEST_ASSERT_EQUAL(9, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());
  TEST_ASSERT_EQUAL(9 * 36.0, clCaG.feedInTargetPower());
}

void test_ca_set_power_to_middle_after_max(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *            current [A] = x     consumption [Wh] 
   * min             0    => 0 * 36 =      0   
   * max             9    => 9 * 36 =     324
   * => gain = 9 / 324 = 0,0278; offset = 0 
   */ 

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.updateConsumptionPower(300.0);
  // actual values are equal to the previous set target values that correspond to 324Wh=36V*9A
  clCaG.updateFeedInActualDcValues(36.0, 9.0); 

  //--------------------------------------------------------------------------------------------------- 
  // run process defined number before test
  // 
  int32_t slPCyclesT = 20;
  while (slPCyclesT > 0)
  {
    clCaG.process();
    slPCyclesT--;
  }

  //--------------------------------------------------------------------------------------------------- 
  // check expected values:  => y=mx+b, x=(300+324)=600 : DcInCurrent = 600 * 0,015 + 0 = 9 A
  //                         => P=UI : 36 V * 9 A = 324 W
  TEST_ASSERT_EQUAL(9.0, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());
  TEST_ASSERT_EQUAL(324.0, clCaG.feedInTargetPower());
 
}

void test_ca_set_power_to_zero_after_middle(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *            current [A] = x     consumption [Wh] 
   * min             0    => 0 * 36 =      0   
   * max             9    => 9 * 36 =     324
   * => gain = 9 / 324 = 0,0278; offset = 0 
   */ 

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.updateConsumptionPower(0.0);
  // actual values are equal to the previous set target values that correspond to 324Wh=36V*9A
  clCaG.updateFeedInActualDcValues(36.0, 9.0); 

  //--------------------------------------------------------------------------------------------------- 
  // run process defined number before test
  // 
  int32_t slPCyclesT = 20;
  while (slPCyclesT > 0)
  {
    clCaG.process();
    slPCyclesT--;
  }

  //--------------------------------------------------------------------------------------------------- 
  // check expected values:  => y=mx+b, x=(0+324)=324 : DcInCurrent = 324 * 0,015 + 0 = 9 A
  //                         => P=UI : 36 V * 9 A = 324 W
  TEST_ASSERT_EQUAL(9.0, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());
  TEST_ASSERT_EQUAL(324.0, clCaG.feedInTargetPower());
 
}


void test_ca_set_power_to_negative_after_zero(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *            current [A] = x     consumption [Wh] 
   * min             0    => 0 * 36 =      0   
   * max             9    => 9 * 36 =     324
   * => gain = 9 / 324 = 0,0278; offset = 0 
   */ 

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.updateConsumptionPower(-160.0);
  // actual values are equal to the previous set target values that correspond to 324Wh=36V*9A
  clCaG.updateFeedInActualDcValues(36.0, 9.0); 

  //--------------------------------------------------------------------------------------------------- 
  // run process defined number before test
  // 
  int32_t slPCyclesT = millis() + 1000;
  while (slPCyclesT >  millis())
  {
    clCaG.process();
  }

  //--------------------------------------------------------------------------------------------------- 
  // check expected values:  => y=mx+b, x=(-160+324)=164 : DcInCurrent = 164 * 0,0278 + 0 = 4,56 A
  //                         => P=UI : 36 V * 4,56 A = 164,16 W
  TEST_ASSERT_EQUAL(4.0, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(164.16, clCaG.feedInTargetPower());
 

  //--------------------------------------------------------------------------------------------------- 
  //--------------------------------------------------------------------------------------------------- 
  // in case of zero the current should not be changed
  // 
  clCaG.updateConsumptionPower(0.0);
  // actual values are equal to the previous set target values that correspond to 164Wh=36V*4,56A
  clCaG.updateFeedInActualDcValues(36.0, 4.56);

  //--------------------------------------------------------------------------------------------------- 
  // run process defined number before test
  // 
  slPCyclesT = 200;
  while (slPCyclesT > 0)
  {
    clCaG.process();
    slPCyclesT--;
  }
  //--------------------------------------------------------------------------------------------------- 
  // check expected values:  => y=mx+b, x=(0+164)=164 : DcInCurrent = 164 * 0,0278 + 0 = 4,56 A
  //                         => P=UI : 36 V * 4,56 A = 164,16 W
  TEST_ASSERT_EQUAL(4.0, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());
  TEST_ASSERT_EQUAL(164.16, clCaG.feedInTargetPower());
}


void test_ca_set_power_to_negative_to_zero(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *            current [A] = x     consumption [Wh] 
   * min             0    => 0 * 36 =      0   
   * max             9    => 9 * 36 =     324
   * => gain = 9 / 324 = 0,0278; offset = 0 
   */ 

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.updateConsumptionPower(-100.0);
  // actual values are equal to the previous set target values that correspond to 164Wh=36V*4,56A
  clCaG.updateFeedInActualDcValues(36.0, 4.56);

  //--------------------------------------------------------------------------------------------------- 
  // run process defined number before test
  // 
  int32_t slPCyclesT = millis() + 1000;
  while (slPCyclesT >  millis())
  {
    clCaG.process();
  }

  //--------------------------------------------------------------------------------------------------- 
  // check expected values:  => y=mx+b, x=(-100+164)=64 : DcInCurrent = 64 * 0,0278 + 0 = 1,78 A
  //                         => P=UI : 36 V * 1,78 A = 64,08 W
  TEST_ASSERT_EQUAL(1.78, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());
  TEST_ASSERT_EQUAL(64.16, clCaG.feedInTargetPower());

  //--------------------------------------------------------------------------------------------------- 
  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.updateConsumptionPower(-100.0);
    // actual values are equal to the previous set target values that correspond to 64Wh=36V*1,78A
  clCaG.updateFeedInActualDcValues(36.0, 1.78);

  //--------------------------------------------------------------------------------------------------- 
  // run process defined number before test
  // 
  slPCyclesT = millis() + 1000;
  while (slPCyclesT >  millis())
  {
    clCaG.process();
  }

  //--------------------------------------------------------------------------------------------------- 
  // check expected values:  => y=mx+b, x=(-100+64,08)=-36 : DcInCurrent = -36 * 0,0278 + 0 = -1 A
  //                         => P=UI : 36 V * 1,78 A = 64,08 W
  TEST_ASSERT_EQUAL(0, clCaG.feedInTargetDcCurrent());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInTargetDcVoltage());
  TEST_ASSERT_EQUAL(0, clCaG.feedInTargetPower());
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
  clCaG.init();

  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  
  UNITY_BEGIN(); // IMPORTANT LINE!


  RUN_TEST(test_ca_init_parameter);
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
  // The sequence of the following tests must be adhered to in order to pass them!
  //
  RUN_TEST(test_ca_filter);
  RUN_TEST(test_ca_set_power_to_min_at_start);
  RUN_TEST(test_ca_set_power_to_max_at_start);
  RUN_TEST(test_ca_set_power_to_middle_after_max);
  RUN_TEST(test_ca_set_power_to_zero_after_middle);
  RUN_TEST(test_ca_set_power_to_negative_after_zero);
  RUN_TEST(test_ca_set_power_to_negative_to_zero);
  
  UNITY_END(); // stop unit testing
  while (1) {}; // only if only one test is available
  // }
}