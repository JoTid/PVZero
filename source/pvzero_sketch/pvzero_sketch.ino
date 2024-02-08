#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


// Replace with your network credentials
const char* ssid = "dummy";
const char* password = "dummy";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

String currentDate;

//--------------------------------------------------------------------------------------------------------------------//
// Perform initialisation of Wifi and connect to defined SIID                                                         //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void initWiFi() {
  //--------------------------------------------------------------------------------------------------- 
  //
  //
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

  //--------------------------------------------------------------------------------------------------- 
  // Initialize a NTPClient to get time
  //
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
}




//--------------------------------------------------------------------------------------------------------------------//
// main setup of application                                                                                          //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void setup() {

  //--------------------------------------------------------------------------------------------------- 
  // initialise LED
  //
  ledInit();
  ledSet(0, true);

  //--------------------------------------------------------------------------------------------------- 
  // Use serial intierface for debug
  //
  Serial.begin(115200); // Setup serial object and define transmission speed
  Serial.println("Starting setup...");
  Serial.printf_P(PSTR("Time: %d\n"), millis()); // using PROGMEM

  //--------------------------------------------------------------------------------------------------- 
  // initialise the LCD
  //
  lcdInit();

  //--------------------------------------------------------------------------------------------------- 
  // Not initialize Wifi and print debug info
  //
  initWiFi();
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());

  //--------------------------------------------------------------------------------------------------- 
  // Perfomr initialisation for Shelly 3EM comunication 
  //
  shelly3emInit();


  Serial.println(F("Setup completed."));
}

//--------------------------------------------------------------------------------------------------------------------//
// main loop of application                                                                                           //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void loop() {

  //--------------------------------------------------------------------------------------------------- 
  // update time and play around with info 
  //
  timeClient.update();

  //--------------------------------------------------------------------------------------------------- 
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedTime();
  Serial.println(formattedDate);

  int splitT = formattedDate.indexOf("T");
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-3);

  //--------------------------------------------------------------------------------------------------- 
  // get the Date 
  //
  time_t epochTime = timeClient.getEpochTime();

  // use time structure for date calculation 
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;

  // complete date
  currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);

  //--------------------------------------------------------------------------------------------------- 
  // control the LED
  //
  if (ledIsOn(0))
  {
    ledSet(0, false);
  }
  else
  {
    ledSet(0, true);
  }

  //--------------------------------------------------------------------------------------------------- 
  // Process info from Shelly 3EM and update LCD Data
  //
  if (shelly3emProcess())
  {
    lcdUpdatePower(shelly3emTotalPower());
  }
  
  delay(1000);
}

