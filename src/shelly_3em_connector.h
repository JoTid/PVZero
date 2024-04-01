/**************************************************************

This file is a part of
https://github.com/JoTid/PVZero

Copyright [2020] Alexander Tiderko

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-3.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

**************************************************************/
#ifndef PVZ_SHELLY_3EM_CONNECTOR_H
#define PVZ_SHELLY_3EM_CONNECTOR_H

#include <WiFiClient.h>
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
#include <HttpClient.h>
#endif
#include <Arduino.h>
#include "ewcConfigInterface.h"
#include "sleeper.h"

namespace PVZ
{
  typedef std::function<void(bool, int32_t)> ShellyStateCallback;

  class Shelly3emConnector : public EWC::ConfigInterface
  {
  public:
    enum State
    {
      UNKNOWN,
      SLEEP,
      ON_CHECK,
      DO_NOT_DISTURB
    };
    // Sleeper& sleeper() { return *_sleeper; }
    bool mailStateChanged; // use for state detection for e-mail send
    Shelly3emConnector(int potPin = A0);
    ~Shelly3emConnector();
    void setup(JsonDocument &config, bool resetConfig = false);
    void fillJson(JsonDocument &config);
    void fromJson(JsonDocument &config);
    void loop();
    void setCallbackState(ShellyStateCallback callback) { _callbackState = callback; }
    String state2string(Shelly3emConnector::State state);
    String info() { return _infoState; }
    String infoSleepUntil() { return _sleepUntil; }
    // ------ protected by mutex -----------
    int32_t getConsumptionPower();
    uint64_t getTimestamp();
    bool isValidConsumptionPower();
    String getUri();
    String getErrorCodes();
    void deleteErrorCodes();
    void forceUpdate() { _sleeper.wakeup(); }

  protected:
    int _potPin;
    TaskHandle_t _httpTaskHandle = NULL;
    unsigned int _reachedUpperLimit;
    ShellyStateCallback _callbackState;
    Sleeper _sleeper;
    String _infoState;
    String _sleepUntil;
    bool _isRequesting;
    long _countRequestsFailed;

    static void httpTask(void *_this);
    WiFiClient wifiClient;
    HTTPClient httpClient;
    // ------ protected by mutex -----------
    void _onTaskResult(bool valid, int32_t consumptionPower, uint64_t timestamp, int httpCode);
    bool _isTaskRunning();
    bool _taskIsRunning;
    String _taskShelly3emUri;
    bool _taskConsumptionPowerValid;
    int32_t _taskConsumptionPower;
    uint64_t _taskTimestamp;
    String _taskErrorCodes;
  };
}; // namespace

#endif
