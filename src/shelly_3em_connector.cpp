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
#include <mutex>
#include <extensions/ewcTime.h>
#include <extensions/ewcMail.h>
#include <ewcLed.h>
#include <ewcInterface.h>
#include "shelly_3em_connector.h"
#include "ewcLogger.h"
#include "pvz_config.h"
#include "pvzero_interface.h"

using namespace EWC;
using namespace PVZ;

std::mutex httpTaskMutex;

Shelly3emConnector::Shelly3emConnector(int potPin) : EWC::ConfigInterface("Shelly3em")
{
  mailStateChanged = false;
  _potPin = potPin;
  _callbackState = NULL;
  _taskConsumptionPowerValid = false;
  _taskConsumptionPower = 0;
  _isRequesting = false;
  _taskIsRunning = false;
  _countRequestsFailed = 0;
  _taskShelly3emUri = "";
}

Shelly3emConnector::~Shelly3emConnector()
{
  // delete _sleeper;
}

void Shelly3emConnector::setup(JsonDocument &config, bool resetConfig)
{
  fromJson(config);
  if (_reachedUpperLimit != 1)
  {
    _reachedUpperLimit = 0;
  }
  // pinMode(_potPin, OUTPUT);
  // digitalWrite(_potPin, LOW);
  // _sleeper->setup(resetConfig);
}

void Shelly3emConnector::fillJson(JsonDocument &config)
{
  // we use this method to be informed about save configuration
  String uri = PZI::get().config().getShelly3emAddr();
  if (uri.length() > 0 && !uri.startsWith("http"))
  {
    uri = String("http://") + uri;
  }
  if (uri.length() > 0)
  {
    std::lock_guard<std::mutex> lck(httpTaskMutex);
    EWC::I::get().logger() << F("Shelly3emConnector: set uri ") << uri << endl;
    _taskShelly3emUri = uri + "/status";
  }
}
void Shelly3emConnector::fromJson(JsonDocument &config)
{
  String uri = PZI::get().config().getShelly3emAddr();
  if (uri.length() > 0 && !uri.startsWith("http"))
  {
    uri = String("http://") + uri;
  }
  if (uri.length() > 0)
  {
    std::lock_guard<std::mutex> lck(httpTaskMutex);
    EWC::I::get().logger() << F("Shelly3emConnector: set uri ") << uri << endl;
    _taskShelly3emUri = uri + "/status";
  }
}

void Shelly3emConnector::loop()
{
  _infoState = String("");
  if (I::get().time().isDisturb())
  {
    _infoState += "bitte nicht stoeren phase";
    return;
  }
  if (_sleeper.finished())
  {
    if (getUri().length() > 0)
    {
      // we are in the requesting loop
      if (!_isTaskRunning())
      {
        // http request is not running, should we resume the task
        if (_httpTaskHandle != NULL)
        {
          if (!_isRequesting)
          {
            _isRequesting = true;
            EWC::I::get().logger() << F("Shelly3emConnector: request consumption power from ") << getUri() << ", rssi: " << WiFi.RSSI() << endl;
            I::get().led().start(LED_GREEN, 250, 150);
            std::lock_guard<std::mutex> lck(httpTaskMutex);
            _taskIsRunning = true;
            vTaskResume(_httpTaskHandle);
          }
          else
          {
            // our request is active -> task was finished, check the results
            I::get().led().start(LED_GREEN, 2000, 2000, 1);
            _isRequesting = false;
            if (isValidConsumptionPower())
            {
              // update time
              if (!I::get().time().isNtpEnabled())
              {
                I::get().time().setLocalTime(getTimestamp());
              }
              // successful request
              if (_callbackState != NULL)
              {
                _callbackState(true, getConsumptionPower());
              }
              if (_countRequestsFailed >= 3)
              {
                _infoState = String("Nach ") + _countRequestsFailed + " Versuchen wurde der Wert " + getConsumptionPower() + " W geholt." + "\nHttpCodes" + getErrorCodes();
                PZI::get().mail().sendWarning("Shelly 3em wieder erreichbar", _infoState.c_str());
                deleteErrorCodes();
              }
              _countRequestsFailed = 0;
              EWC::I::get().logger() << F("Shelly3emConnector: request finished... sleep ") << endl;
              _sleepUntil = I::get().time().str(PZI::get().config().getCheckInterval());
              _infoState = "Nächster check um " + _sleepUntil;
              _sleeper.sleep(PZI::get().config().getCheckInterval() * 1000);
            }
            else
            {
              // failed request
              I::get().led().start(LED_RED, 2000, 2000, 1);
              _countRequestsFailed += 1;
              if (_callbackState != NULL)
              {
                _callbackState(false, 0);
              }
              if (_countRequestsFailed == 3)
              {
                _infoState = "Fehler beim holen der aktuellen Verbrauchswerte vom Shelly " + getUri() + "\nHttpCode" + getErrorCodes();
                PZI::get().mail().sendWarning("Shelly 3em nicht erreichbar", _infoState.c_str());
              }
              _sleeper.sleep(PZI::get().config().getCheckInterval() * 1000);
            }
          }
        }
        else
        {
          // create request task
          // httpClient.useHTTP10(true);
          // httpClient.setReuse(true);
          EWC::I::get().logger() << F("Shelly3emConnector: create request task") << endl;
          xTaskCreate(
              this->httpTask,                // Function that should be called
              "GET current by http request", // Name of the task (for debugging)
              4096,                          // Stack size (bytes)
              this,                          // Parameter to pass
              5,                             // Task priority
              &_httpTaskHandle               // Task handle
          );
        }
      }
    }
    else
    {
      _sleeper.sleep(PZI::get().config().getCheckInterval() * 1000);
    }
  }
};

String Shelly3emConnector::state2string(Shelly3emConnector::State state)
{
  switch (state)
  {
  case ON_CHECK:
    return "ON_CHECK";
  case DO_NOT_DISTURB:
    return "DO_NOT_DISTURB";
  case UNKNOWN:
    return "UNKNOWN";
  case SLEEP:
    return "SLEEP";
  }
  return "Unknown";
}

int32_t Shelly3emConnector::getConsumptionPower()
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  return _taskConsumptionPower;
}
bool Shelly3emConnector::isValidConsumptionPower()
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  return _taskConsumptionPowerValid;
}

String Shelly3emConnector::getUri()
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  return _taskShelly3emUri;
}

bool Shelly3emConnector::_isTaskRunning()
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  return _taskIsRunning;
}

uint64_t Shelly3emConnector::getTimestamp()
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  return _taskTimestamp;
}

String Shelly3emConnector::getErrorCodes()
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  return _taskErrorCodes;
}

void Shelly3emConnector::deleteErrorCodes()
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  _taskErrorCodes = "";
}

void Shelly3emConnector::_onTaskResult(bool valid, int32_t consumptionPower, uint64_t timestamp, int httpCode)
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  _taskConsumptionPowerValid = valid;
  _taskConsumptionPower = consumptionPower;
  _taskIsRunning = false;
  _taskTimestamp = timestamp;
  if (!valid)
  {
    _taskErrorCodes += ": " + String(httpCode) + ", ";
  }
}

void Shelly3emConnector::httpTask(void *_this)
{
  Shelly3emConnector *sc = static_cast<Shelly3emConnector *>(_this);
  while (true)
  {
    vTaskSuspend(NULL);
    bool valid = false;
    int32_t consumptionPower = 0;
    uint64_t timestamp = 0;
    String infoState;
    String requestUri = sc->getUri();
    HTTPClient httpClient;
    httpClient.begin(sc->wifiClient, requestUri.c_str());

    //-------------------------------------------------------------------------------------------
    // Send request like: <http://192.168.2.18/status>
    // and get response like:
    // {"wifi_sta": {"connected":true,"ssid":"TiHo","ip":"192.168.2.18","rssi":-69},
    //  "cloud":{"enabled":true,"connected":true},
    //  "mqtt":{"connected":false},
    //  "time":"19:24","unixtime":1729272273,"serial":21275,"has_update":false,"mac":"8CAAB561F822","
    //   cfg_changed_cnt":0,"actions_stats":{"skipped":0},
    //   "relays":[{"ison":false,"has_timer":false,"timer_started":0,"timer_duration":0,"timer_remaining":0,"overpower":false,"is_valid":true,"source":"input"}],
    //   "emeters":[
    //      {"power":3883.29,"pf":0.86,"current":19.56,"voltage":231.24,"is_valid":true,"total":7047963.3,"total_returned":96056.5},
    //      {"power":6887.17,"pf":1.00,"current":29.91,"voltage":230.25,"is_valid":true,"total":10596202.7,"total_returned":268858.0},
    //      {"power":3393.38,"pf":0.88,"current":16.89,"voltage":231.74,"is_valid":true,"total":6386482.5,"total_returned":547784.1}
    //      ],
    //      "total_power":14163.84,"emeter_n":{"current":0.00,"ixsum":2.55,"mismatch":false,"is_valid":false},
    //      "fs_mounted":true,"v_data":1,"ct_calst":0,
    //      "update":{"status":"idle","has_update":false,"new_version":"20230913-114244/v1.14.0-gcb84623","old_version":"20230913-114244/v1.14.0-gcb84623","beta_version":"20231107-165007/v1.14.1-rc1-g0617c15"},
    //      "ram_total":49920,"ram_free":30108,"fs_size":233681,"fs_free":152859,"uptime":16932401}
    //
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK)
    {
      JsonDocument doc;
      // JsonDocument doc1;
      // JsonDocument doc2;
      // String jsonDebugStr;
      String jsonStr = httpClient.getString();
      deserializeJson(doc, jsonStr);
      consumptionPower = (int)doc["total_power"];
      timestamp = (uint64_t)doc["unixtime"];

      // deserializeJson(doc1, doc["emeters"]);

      // serializeJson(doc, jsonDebugStr);
      // EWC::I::get().logger() << "doc[emeters]: " << jsonDebugStr << endl;

      // deserializeJson(doc2, doc1[0]);
      // EWC::I::get().logger() << "doc2[emeters[0].power]: " << (float)doc["emeters"][0]["total"] << endl;
      // deserializeJson(doc2, doc1[1]);
      // EWC::I::get().logger() << "doc2[emeters[1].power]: " << (float)doc["emeters"][1]["total"] << endl;
      // deserializeJson(doc2, doc1[2]);
      // EWC::I::get().logger() << "doc2[emeters[2].power]: " << (float)doc["emeters"][2]["total"] << endl;

      // float totalConsumption = (float)doc["emeters"][0]["total"];
      // totalConsumption += (float)doc["emeters"][1]["total"];
      // totalConsumption += (float)doc["emeters"][2]["total"];
      // EWC::I::get().logger() << " total consumption: " << String(totalConsumption, 0) << endl;

      valid = true;
    }
    else
    {
      EWC::I::get().logger() << F("Shelly3emConnector: ERROR httpCode of GET: ") << httpCode << endl;
    }
    httpClient.end();
    sc->_onTaskResult(valid, consumptionPower, timestamp, httpCode);
  }
}
