#include <U8g2lib.h>
#include <Wire.h>

void progressBar2(void);
void progressBar3(void);


U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// const char COPYRIGHT_SYMBOL[] = { 0xa9, '\0' };
void u8g2_prepare(void);
void u8g2_prepare() {
    u8g2.setFont(u8g2_font_6x10_tf); 
    u8g2.setFontRefHeightExtendedText(); 
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);
}
// void u8g2_box_frame() {
//     u8g2.drawStr(0, 0, "drawBox");
//     u8g2.drawBox(5, 10, 20, 10);
//     u8g2.drawStr(60, 0, "drawFrame");
//     u8g2.drawFrame(65, 10, 20, 10);
// }
// void u8g2_r_frame_box() {
//     u8g2.drawStr(0, 0, "drawRFrame");
//     u8g2.drawRFrame(5, 10, 40, 15, 3);
//     u8g2.drawStr(70, 0, "drawRBox");
//     u8g2.drawRBox(70, 10, 25, 15, 3);
// }
// void u8g2_disc_circle() {
//     u8g2.drawStr(0, 0, "drawDisc");
//     u8g2.drawDisc(10, 18, 9);
//     u8g2.drawDisc(30, 16, 7);
//     u8g2.drawStr(60, 0, "drawCircle");
//     u8g2.drawCircle(70, 18, 9);
//     u8g2.drawCircle(90, 16, 7);
// }


// void u8g2_string_orientation() {
//     u8g2.setFontDirection(0);
//     u8g2.drawStr(5, 15, "0");
//     u8g2.setFontDirection(3);
//     u8g2.drawStr(40, 25, "90");
//     u8g2.setFontDirection(2);
//     u8g2.drawStr(75, 15, "180");
//     u8g2.setFontDirection(1);
//     u8g2.drawStr(100, 10, "270");
// }
// void u8g2_line() {
//     u8g2.drawStr( 0, 0, "drawLine");
//     u8g2.drawLine(7, 10, 40, 32);
//     u8g2.drawLine(14, 10, 60, 32);
//     u8g2.drawLine(28, 10, 80, 32);
//     u8g2.drawLine(35, 10, 100, 32);
// }
// void u8g2_triangle() {
//     u8g2.drawStr( 0, 0, "drawTriangle");
//     u8g2.drawTriangle(14, 7, 45, 30, 10, 32);
// }
// void u8g2_unicode() {
//     u8g2.drawStr(0, 0, "Unicode");
//     u8g2.setFont(u8g2_font_unifont_t_symbols);
//     u8g2.setFontPosTop();
//     u8g2.setFontDirection(0);
//     u8g2.drawUTF8(10, 15, "☀");
//     u8g2.drawUTF8(30, 15, "☁");
//     u8g2.drawUTF8(50, 15, "☂");
//     u8g2.drawUTF8(70, 15, "☔");
//     u8g2.drawUTF8(95, 15, COPYRIGHT_SYMBOL);  //COPYRIGHT SIMBOL
//     u8g2.drawUTF8(115, 15, "\xb0");  // DEGREE SYMBOL
// }


void lcdInit()
{
  Serial.printf("Initialise LCD...\n");

  u8g2.begin();
  u8g2_prepare();


  // u8g2.clearBuffer();
  // u8g2_prepare();
  // u8g2_box_frame();
  // u8g2.sendBuffer();
  // delay(1500); 
  // u8g2.clearBuffer();
  // u8g2_disc_circle();
  // u8g2.sendBuffer();
  // delay(1500); 
  // u8g2.clearBuffer();
  // u8g2_r_frame_box();
  // u8g2.sendBuffer();
  // delay(1500); 
  //  u8g2.clearBuffer();
  //  u8g2_prepare();
  //  u8g2_string_orientation();
  //  u8g2.sendBuffer();
  //  delay(1500); 
  // u8g2.clearBuffer();
  // u8g2_line();
  // u8g2.sendBuffer();
  // delay(1500); 
  // u8g2.clearBuffer();
  // u8g2_triangle();
  // u8g2.sendBuffer();
  // delay(1500);
  // u8g2.clearBuffer();
  // u8g2_prepare();
  // u8g2_unicode();
  // u8g2.sendBuffer();
  // delay(1500);
}

// void lcdUpdatePower(int slValueS) 
// {
//   String clLineT;
  

//   char battVoltageStr[20]; //Assignment of char
//   sprintf(battVoltageStr,"%d",slValueS); // Copying integer to char as string


//   u8g2.clearBuffer();
//   u8g2_prepare();
//   u8g2.setFontDirection(0);
//   clLineT = "10:10"; // + " " + currentDate; //"23:16 07.02.2024";
//   // const char * myV = String("23:16 07.02.2024").c_str();
//   u8g2.drawStr(128-u8g2.getStrWidth(clLineT.c_str()), 0, clLineT.c_str()); 
//   u8g2.drawStr(0, 0, "PVZero");
//   // u8g2.drawStr(40, 0, "Mama");
//   u8g2.drawLine(0, 12, 128, 12);
//   clLineT = "Total Power: ";
//   clLineT += String(battVoltageStr).c_str();
//   clLineT += " Wh";
//   u8g2.drawStr(0, 15, clLineT.c_str());

//   u8g2.drawLine(0, 52, 128, 52);
//   // clLineT = String();
//   u8g2.setCursor(0, 54);
//   u8g2.print("192.168.0.1");//WiFi.localIP()); 
//   // clLineT = "RSSI: ";
//   clLineT = "tiki";//WiFi.RSSI();
//   u8g2.drawStr(128-u8g2.getStrWidth(clLineT.c_str()), 54, clLineT.c_str()); 
//   // u8g2.drawStr(0, 52, clLineT.c_str());


//   u8g2.sendBuffer();
// }


// void progressBar()
// {
//   int32_t ulSpaceT = 32;
//   // uint32_t ulSpaceS = 64;
//   static int32_t ulMoveS;
//   int32_t aslStartT[3];
//   int32_t aslStopT[3];
//   int32_t slWidthT = 64;
//   // static uint32_t ulSpaceStopPosS = ulSpaceStopPosS + ulSpaceStartPosS;
//   ulMoveS += 4;
//   if (ulMoveS > (128))
//   {
//     ulMoveS = 0;//-slWidthT;
//   }

//   if ((ulMoveS - slWidthT) < 0)
//   {
    
//     aslStartT[2] = ulMoveS;
//     aslStopT[2] = 128-(ulMoveS - slWidthT);
//     u8g2.drawLine(aslStartT[2], 52, (ulMoveS + slWidthT), 52);
//     // u8g2.drawLine(0, 52, aslStopT[2] , 52);
//   } 

//   else if ((ulMoveS + slWidthT) > 128)
//   {
//     aslStartT[0] = ulMoveS;
//     aslStopT[0] = (ulMoveS + slWidthT)-128;
//     u8g2.drawLine(aslStartT[0], 52, 128, 52);
//     u8g2.drawLine(0, 52, aslStopT[0], 52);
    
//   } 
//   else 
//   {
//     aslStartT[1] = ulMoveS;
//     aslStopT[1] = ulMoveS+slWidthT;
//     u8g2.drawLine(aslStartT[1], 52, aslStopT[1], 52);
//   }
  
  
//   // u8g2.drawLine(ulMoveS+1, 52, ulMoveS+1, 52);
//   // u8g2.drawLine(ulMoveS+2, 52, ulMoveS+2, 52);
//   // u8g2.drawLine(ulMoveS+3, 52, ulMoveS+3, 52);
//   // u8g2.drawLine(ulMoveS+4, 52, ulMoveS+4, 52);
//   // u8g2.drawLine(ulMoveS+5, 52, ulMoveS+5, 52);
//   // u8g2.drawLine(ulMoveS+ulSpaceT, 52, 128, 52);
//   // ulSpaceStartPosS += 32;
//   // if (ulSpaceStartPosS > 128)
//   // {
//   //   ulSpaceStartPosS = 0;
//   // }
// }

void progressBar2()
{
  static int32_t ulMoveS = 0;
  static bool btOperatorT = true;

  if (btOperatorT)
  {
    ulMoveS += 2;
    if (ulMoveS >= 64)
    { 
      btOperatorT = false;
    }
  }
  else
  {
    ulMoveS -= 2;
    if (ulMoveS <= 0)
    { 
      btOperatorT = true;
    }    
  }
  u8g2.drawLine(64-ulMoveS, 52, 64+ulMoveS, 52);
}

void progressBar3()
{
  static bool btOperatorT = true;

  if (btOperatorT)
  {
    btOperatorT = false;
    u8g2.drawLine(0, 52, 41, 52);
    u8g2.drawLine(84, 52, 127, 52);
  }
  else
  {
    btOperatorT = true;
    u8g2.drawLine(42, 52, 83, 52);
  }
}




//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                     //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void lcdProcess(int slValueS) 
{
  String clLineT;
  char battVoltageStr[20]; // array is used for spirntf operations
  static int slOldTimeS = millis(); 
  

  //--------------------------------------------------------------------------------------------------- 
  // collect data
  //

  //--------------------------------------------------------------------------------------------------- 
  // preopare the LCD
  //


  

  if (millis() > (slOldTimeS + 20))
  {
    u8g2.clearBuffer();
    u8g2_prepare();
    u8g2.setFontDirection(0);

    u8g2.drawStr(0, 0, "PVZero");
    u8g2.drawLine(0, 12, 128, 12);

    progressBar2();
    // u8g2.drawLine(0, 52, 128, 52);
    slOldTimeS = millis();


  //--------------------------------------------------------------------------------------------------- 
  // show informataion of step 1: initialisation and connection to the provided SSID 
  //
  if (slValueS == 0)
  {
    u8g2.drawStr(128-u8g2.getStrWidth(String("v1.00").c_str()), 0, String("v1.00").c_str());    
  }


//   //--------------------------------------------------------------------------------------------------- 
//   // show informataion of step 1: initialisation and connection to the provided SSID 
//   //
//   else if (slValueS == 1)
//   {
//     u8g2.drawStr(128-u8g2.getStrWidth(String("v1.00").c_str()), 0, String("v1.00").c_str());    

    
//     u8g2.drawStr(0, 15, String("Connecting to SSID:").c_str());
//     u8g2.setFont(u8g2_font_10x20_tf); 
//     u8g2.drawStr((128-u8g2.getStrWidth(String(ssid).c_str()))/2, 30, String(ssid).c_str());
//     u8g2.setFont(u8g2_font_6x10_tf);
//   }

//   //--------------------------------------------------------------------------------------------------- 
//   // show informataion of step 2: connection fail 
//   //
//   else if (slValueS == 2)
//   {

//   }

//   //--------------------------------------------------------------------------------------------------- 
//   // show informataion of step 3: connection valid
//   //
//   else if (slValueS > 3)
//   {
//     u8g2.drawStr(128-u8g2.getStrWidth(timeStamp.c_str()), 0, timeStamp.c_str());     
//   }

//   // //--------------------------------------------------------------------------------------------------- 
//   // // show informataion of step 2: show info while normal opertion
//   // //
//   // else if (slValueS == 2)
//   // {

//   // // }


  
  // sprintf(battVoltageStr,"%d",slValueS); // Copying integer to char as string


  // clLineT = timeStamp; // + " " + currentDate; //"23:16 07.02.2024";
  // // const char * myV = String("23:16 07.02.2024").c_str();
  
  // // u8g2.drawStr(40, 0, "Mama");

  // clLineT = "Total Power: ";
  // clLineT += String(battVoltageStr).c_str();
  // clLineT += " Wh";
  // u8g2.drawStr(0, 15, clLineT.c_str());

  
  // // clLineT = String();
  // u8g2.setCursor(0, 54);
  // u8g2.print(WiFi.localIP()); 
  // // clLineT = "RSSI: ";
  // clLineT = WiFi.RSSI();
  // u8g2.drawStr(128-u8g2.getStrWidth(clLineT.c_str()), 54, clLineT.c_str()); 
  // // u8g2.drawStr(0, 52, clLineT.c_str());


  u8g2.sendBuffer();

  }
  // slOldTimeS = slNewTimeS;
}