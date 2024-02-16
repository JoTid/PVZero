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
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInPower());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInDcCurrent());
  TEST_ASSERT_EQUAL(36.0, clCaG.feedInDcVoltage());
  TEST_ASSERT_EQUAL(3, clCaG.filterOrder());
}

void test_ca_set_consumption_power(void)
{

  TEST_ASSERT_EQUAL(1, 1);
}

void test_ca_filter(void)
{
  
}

void test_ca_set_power_to_min_at_start(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *       consumption [Wh]      current [A]
   * min       0                      0
   * max      600                     9
   * 
   */

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.setConsumptionPower(0.0);

  //--------------------------------------------------------------------------------------------------- 
  // run process defined number before test
  // 
  int32_t slPCyclesT = 5;
  while (slPCyclesT > 0)
  {
    clCaG.process();
    slPCyclesT--;
  }

  //--------------------------------------------------------------------------------------------------- 
  // check expected values
  // 
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInPower());
  TEST_ASSERT_EQUAL(0.0, clCaG.feedInDcCurrent());
}



void test_ca_set_power_to_max_at_start(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *       consumption [Wh]      current [A]     
   * min       0                     0
   * max      600                    9
   * => gain = 9 / 600 = 0,015; offset = 0 
   */ 

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.setConsumptionPower(600.0);

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
  // check expected values:  => y=mx+b : DcInCurrent = 600 * 0,015 + 0 = 9 A
  //                         => P=UI : 36 V * 9 A = 324 W
  TEST_ASSERT_EQUAL(9.0, clCaG.feedInDcCurrent());
  TEST_ASSERT_EQUAL(324.0, clCaG.feedInPower());
 
}

void test_ca_set_power_to_middle_after_max(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *            current [A] = x     consumption [Wh] 
   * min             0    => 0 * 36 =      0   
   * max             9    => 9 * 36 =     324
   * => gain = 9 / 600 = 0,015; offset = 0 
   */ 

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.setConsumptionPower(300.0);

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
  TEST_ASSERT_EQUAL(9.0, clCaG.feedInDcCurrent());
  TEST_ASSERT_EQUAL(324.0, clCaG.feedInPower());
 
}

void test_ca_set_power_to_zero_after_middle(void)
{
  /**
   * @brief Set consumption power to 0 Wh after power up / start, the current output should be also at 0.
   * Test parameter while test:
   *            current [A] = x     consumption [Wh] 
   * min             0    => 0 * 36 =      0   
   * max             9    => 9 * 36 =     324
   * => gain = 9 / 600 = 0,015; offset = 0 
   */ 

  //--------------------------------------------------------------------------------------------------- 
  // trigger test method
  // 
  clCaG.setConsumptionPower(0.0);

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
  // check expected values:  => y=mx+b, x=(0+324)=600 : DcInCurrent = 600 * 0,015 + 0 = 9 A
  //                         => P=UI : 36 V * 9 A = 324 W
  TEST_ASSERT_EQUAL(9.0, clCaG.feedInDcCurrent());
  TEST_ASSERT_EQUAL(324.0, clCaG.feedInPower());
 
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
  // run tests step by step
  //
  RUN_TEST(test_ca_set_power_to_min_at_start);
  RUN_TEST(test_ca_set_power_to_max_at_start);
  RUN_TEST(test_ca_set_power_to_middle_after_max);
  RUN_TEST(test_ca_set_power_to_zero_after_middle);

  UNITY_END(); // stop unit testing
  while (1) {}; // only if only one test is available
  // }
}