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
#include <ewcTickerLED.h>
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
            EWC::I::get().logger() << F("Shelly3emConnector: request consumption power from ") << getUri() << " actually task is " << _taskIsRunning << endl;
            I::get().led().start(1000, 250);
            vTaskResume(_httpTaskHandle);
            std::lock_guard<std::mutex> lck(httpTaskMutex);
            _taskIsRunning = true;
          }
          else
          {
            // our request is active -> task was finished, check the results
            _isRequesting = false;
            I::get().led().stop();
            if (isValidConsumptionPower())
            {
              // successful request
              if (_callbackState != NULL)
              {
                _callbackState(true, getConsumptionPower());
              }
              if (_countRequestsFailed >= 3)
              {
                _infoState = String("Nach ") + _countRequestsFailed + " Versuchen wurde der Wert " + getConsumptionPower() + " W geholt.";
                PZI::get().mail().sendWarning("Shelly 3em wieder erreichbar", _infoState.c_str());
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
              I::get().led().start(3000, 3000);
              _countRequestsFailed += 1;
              if (_callbackState != NULL)
              {
                _callbackState(false, 0);
              }
              if (_countRequestsFailed == 3)
              {
                _infoState = "Fehler beim holen der aktuellen Verbrauchswerte vom Shelly " + getUri();
                PZI::get().mail().sendWarning("Shelly 3em nicht erreichbar", _infoState.c_str());
              }
            }
          }
        }
        else
        {
          // create request task
          httpClient.useHTTP10(true);
          httpClient.setReuse(true);
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

void Shelly3emConnector::_onTaskResult(bool valid, int32_t consumptionPower)
{
  std::lock_guard<std::mutex> lck(httpTaskMutex);
  _taskConsumptionPowerValid = valid;
  _taskConsumptionPower = consumptionPower;
  _taskIsRunning = false;
}

void Shelly3emConnector::httpTask(void *_this)
{
  Shelly3emConnector *sc = static_cast<Shelly3emConnector *>(_this);
  while (true)
  {
    vTaskSuspend(NULL);
    String infoState;
    bool valid = false;
    int32_t consumptionPower = 0;
    String requestUri = sc->getUri();

    //------------------------------------------------------------------------------------------- 
    // Before sending the Request increase the timeout time. This has been done to avoid 
    // HTTPC_ERROR_READ_TIMEOUT errors that has been occurred sometimes.
    //
    sc->httpClient.setTimeout(15000);
    // Send request
    sc->httpClient.begin(sc->wifiClient, requestUri.c_str());
    int httpCode = sc->httpClient.GET();
    if (httpCode != 200)
    {
      EWC::I::get().logger() << F("Shelly3emConnector: ERROR httpCode of GET: ") << httpCode << endl;
    }
    else
    {
      JsonDocument doc;
      String jsonStr = sc->httpClient.getString();
      deserializeJson(doc, jsonStr);
      consumptionPower = (int)doc["total_power"];
      valid = true;
    }
    sc->_onTaskResult(valid, consumptionPower);
  }
}
