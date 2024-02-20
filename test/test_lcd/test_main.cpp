#include <Arduino.h>
#include <unity.h>

#include <pvz_lcd.hpp>

/**
 * @brief The Control Algorithm class to test
 */
PvzLcd clCaG;

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
void test_lcd_init(void)
{
  //--------------------------------------------------------------------------------------------------- 
  // initialise the LCD and trigger first display
  //
  clCaG.init("1.00");
  clCaG.process();
}

void test_lcd_default(void)
{
  /**
   * @brief this test shows the display content, when no set() Method has been called
   * 
   */

  uint32_t ulTimeT = 1000;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
  }
}

PvzLcd::Screen_ts atsLcdScreenG[5];

void test_lcd_middle_screen(void)
{
  /**
   * @brief this test shows the display content, when no set() Method has been called
   * 
   */
  atsLcdScreenG[0].aclLine[0] = "Hello World ;-)";
  atsLcdScreenG[0].aclLine[1] = "one line fits up to ";
  atsLcdScreenG[0].aclLine[2] = "20 chars";
  clCaG.setScreen(&atsLcdScreenG[0], 1);

  uint32_t ulTimeT = 2000;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
  }
}

void test_lcd_middle_two_screens(void)
{
  /**
   * @brief this test shows the display content, when no set() Method has been called
   * 
   */
  atsLcdScreenG[1].aclLine[0] = "| - enter your info |";
  atsLcdScreenG[1].aclLine[1] = "| - multiple screen |";
  atsLcdScreenG[1].aclLine[2] = "123456789012345678901";

  clCaG.setScreen(&atsLcdScreenG[0], 2);

  uint32_t ulTimeT = 15000;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
  }
}

void test_lcd_header_time_and_date(void)
{
  /**
   * @brief this test shows the display content, when no set() Method has been called
   * 
   */
  atsLcdScreenG[0].aclLine[0] = "- display in header ";
  atsLcdScreenG[0].aclLine[1] = "  time and date";
  atsLcdScreenG[0].aclLine[2] = "  alternatively...";

  clCaG.setScreen(&atsLcdScreenG[0], 1);
  clCaG.updateTime("2024-02-19T22:25:13");

  uint32_t ulTimeT = 10000;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
  }
}

void test_lcd_footer_network_info(void)
{
    /**
   * @brief display network info in the footer
   * 
   */
  atsLcdScreenG[0].aclLine[0] = "- display in footer ";
  atsLcdScreenG[0].aclLine[1] = "  WiFi info";
  atsLcdScreenG[0].aclLine[2] = "  alternatively...";

  clCaG.setScreen(&atsLcdScreenG[0], 1);
  clCaG.updateTime(""); // no time available

  PvzLcd::WifiConfig_ts tsWifiT;
  tsWifiT.clIp = 0x0100A8C0;
  tsWifiT.clSsid = "MyWiFiSSID";  

  clCaG.updateWifiInfo(&tsWifiT);
  clCaG.updateWifiRssi(-60);

  uint32_t ulTimeT = 10000;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
  }
}

void test_lcd_busy_without_footer(void)
{
    /**
   * @brief display network info in the footer
   * 
   */
  atsLcdScreenG[0].aclLine[0] = "- display busy,";
  atsLcdScreenG[0].aclLine[1] = "  this will";
  atsLcdScreenG[0].aclLine[2] = "  not show...";

  clCaG.setScreen(&atsLcdScreenG[0], 1);
  clCaG.updateTime("");     // no time available
  clCaG.updateWifiRssi(0);  // no WiFi Info in footer available
  clCaG.busy("testing busy state:", "I'm busy");

  uint32_t ulTimeT = 5000;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
  }
}

void test_lcd_warning_without_footer(void)
{
    /**
   * @brief display network info in the footer
   * 
   */
  atsLcdScreenG[0].aclLine[0] = "- display busy,";
  atsLcdScreenG[0].aclLine[1] = "  this will";
  atsLcdScreenG[0].aclLine[2] = "  not show...";

  clCaG.setScreen(&atsLcdScreenG[0], 1);
  clCaG.updateTime("");     // no time available
  clCaG.updateWifiRssi(0);  // no WiFi Info in footer available
  clCaG.warning("testing warn state:", "display reason", "what ever...");

  uint32_t ulTimeT = 5000;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
  }
}

void test_lcd_ok_operating(void)
{
    /**
   * @brief display network info in the footer
   * 
   */

  atsLcdScreenG[0].aclLine[0] = "- Final test";
  atsLcdScreenG[0].aclLine[1] = "  hope that all";
  atsLcdScreenG[0].aclLine[2] = "  PASSED :)";
  clCaG.setScreen(&atsLcdScreenG[0], 1);

  clCaG.ok();

  //--------------------------------------------------------------------------------------------------- 
  //
  //
  clCaG.updateTime("2024-02-19T22:49:13");

  //--------------------------------------------------------------------------------------------------- 
  PvzLcd::WifiConfig_ts tsWifiT;
  tsWifiT.clIp = 0x0100A8C0;
  tsWifiT.clSsid = "MyWiFiSSID";  
  clCaG.updateWifiInfo(&tsWifiT);
  clCaG.updateWifiRssi(-80);


  uint32_t ulTimeT = 5000;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
  }
}


void test_lcd_display_default(void)
{
    /**
   * @brief clear display 
   * 
   */
  clCaG.setScreen(NULL);
  clCaG.ok();
  clCaG.updateTime("");
  clCaG.updateWifiRssi(0);


  uint32_t ulTimeT = 500;
  uint64_t uqTickT;
  
  while (ulTimeT > 0)
  {
    if (millis() != uqTickT)
    {
      uqTickT = millis();
      ulTimeT--;
      clCaG.process();
    }
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
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  UNITY_BEGIN(); // IMPORTANT LINE!

  RUN_TEST(test_lcd_init);
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
  RUN_TEST(test_lcd_default);
  RUN_TEST(test_lcd_middle_screen);
  RUN_TEST(test_lcd_middle_two_screens);
  RUN_TEST(test_lcd_header_time_and_date);
  RUN_TEST(test_lcd_footer_network_info);
  RUN_TEST(test_lcd_busy_without_footer);
  RUN_TEST(test_lcd_warning_without_footer);
  RUN_TEST(test_lcd_ok_operating);

  RUN_TEST(test_lcd_display_default);
  
  UNITY_END(); // stop unit testing
  while (1) {}; // only if only one test is available
  // }
}