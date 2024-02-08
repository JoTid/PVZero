#include <HTTPClient.h>
#include <ArduinoJson.h>

String sTotalPower = "total_power";
String searchKey;

StaticJsonDocument<1224> doc;
DeserializationError clJsonErrorS;

//Your Domain name with URL path or IP address with path
const char* serverName = "http://192.168.2.47/status";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 500;


double totalPower;


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
int httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  int slReturnT = 0;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    
    Serial.println(payload);

    //------------------------------------------------------------------------------------------- 
    // https://arduinojson.org/v6/example/parser/
    // Deserialize the JSON document
    clJsonErrorS = deserializeJson(doc, payload);
    // Test if parsing succeeds.
    if (clJsonErrorS) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(clJsonErrorS.f_str());
      slReturnT = 1;
    }
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    slReturnT = -2;
  }
  // Free resources
  http.end();

  return slReturnT;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
void shelly3emInit()
{
  Serial.printf("Initialise Shelly 3EM Module...\n");
}


//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
bool shelly3emProcess()
{
  bool btReturnT = false;
  //
  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
              
      if (httpGETRequest(serverName) == 0)
      {
         const char* time = doc["time"];
         totalPower = doc["total_power"];

          Serial.println(time);
          Serial.println(totalPower, 2);

          btReturnT = true;
      }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  return btReturnT;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------// 
double shelly3emTotalPower()
{
  return totalPower;
}

