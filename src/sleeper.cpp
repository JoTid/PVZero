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
#include "sleeper.h"
#ifdef ESP8266
#include "ewcRTC.h"
#endif
#include "pvz_config.h"
#include "ewcLogger.h"
#include "ewcInterface.h"

using namespace EWC;
using namespace PVZ;

Sleeper::Sleeper()
{
#ifdef ESP8266
  // initialize utc addresses in setup method
  _sleepTimeUtcAddress = 0;
#endif
  _sleepTime = 0;
  _msSleepStart = 0;
}

Sleeper::~Sleeper()
{
}

void Sleeper::setup(bool resetConfig)
{
#ifdef ESP8266
  _sleepTimeUtcAddress = EWC::I::get().rtc().get();
#endif
  if (deepSleepEnabled())
  {
#ifdef ESP8266
    _sleepTime = EWC::I::get().rtc().read(_sleepTimeUtcAddress);
#endif
    if (_sleepTime > 86400)
    {
      // One day in seconds, if more then reset
      _sleepTime = 0;
#ifdef ESP8266
      EWC::I::get().rtc().write(_sleepTimeUtcAddress, _sleepTime);
#endif
    }
    if (_sleepTime > 0)
    {
      sleep(_sleepTime);
    }
  }
}

bool Sleeper::deepSleepEnabled()
{
  return false;
  // TODO: add support for deep sleep
  // return BI::get().config().deep_sleep.get() || BI::get().config().wifiDisabled();
}

void Sleeper::sleep(unsigned long sleepTime)
{
  unsigned int secSleepTime = sleepTime / 1000;
  EWC::I::get().logger() << F("-> Naechster check in ") << sleepTime / MINUTEms << F(" Minuten") << " und " << secSleepTime - (sleepTime / MINUTEms) << " Sekunden" << endl;
  if (deepSleepEnabled())
  {
    unsigned int deepSleepTime = secSleepTime;
#ifdef ESP8266
    unsigned int sleepTime_rest = 0;
#else
    unsigned int sleepTime_rest = 0;
#endif
    if (secSleepTime > MAX_DEEP_SLEEP_SEC)
    {
      sleepTime_rest = secSleepTime - MAX_DEEP_SLEEP_SEC;
      deepSleepTime = MAX_DEEP_SLEEP_SEC; // eine Stunde, da deepSleep max. 71 Minuten kann
    }
#ifdef ESP8266
    EWC::I::get().rtc().write(_sleepTimeUtcAddress, sleepTime_rest);
#endif
    EWC::I::get().logger() << F("deep sleep for: ") << deepSleepTime << F(" seconds") << endl;
    ESP.deepSleep(deepSleepTime * 1000000); // convert to microseconds
  }
  else
  {
    _msSleepStart = millis();
    _sleepTime = secSleepTime;
    EWC::I::get().logger() << F("soft sleep for: ") << secSleepTime << F(" seconds") << endl;
  }
}

bool Sleeper::finished()
{
  if (deepSleepEnabled())
  {
    return true;
  }
  else
  {
    unsigned long msSleeped = millis() - _msSleepStart;
    return msSleeped / 1000 > _sleepTime;
  }
}

time_t Sleeper::sleepDurationMs()
{
  if (deepSleepEnabled())
  {
    return 0;
  }
  else
  {
    unsigned long msSleeped = millis() - _msSleepStart;
    if (msSleeped / 1000 < _sleepTime)
    {
      return msSleeped;
    }
  }
  return 0;
}

void Sleeper::wakeup()
{
  EWC::I::get().logger() << F("[SLEEPER] wakeup") << endl;
  _sleepTime = 0;
  _msSleepStart = 0;
}
