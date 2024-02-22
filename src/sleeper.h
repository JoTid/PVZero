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
#ifndef PVZ_SLEEP_H
#define PVZ_SLEEP_H

#include <Arduino.h>

namespace PVZ
{

  // eine Stunde, da deepSleep max. 71 Minuten kann
  const unsigned long MAX_DEEP_SLEEP_SEC = 3600;

  class Sleeper
  {
  public:
    Sleeper();
    ~Sleeper();
    void setup(bool resetConfig = false);

    /** Returns true if enabled by configuration or wifi is disabled. **/
    bool deepSleepEnabled();
    /** Deep sleep in millseconds. Only if it is enabled by parameter, see deepSleepEnabled().
     * If deep sleep is disabled you can use finished() to check if soft sleep is expired. **/
    void sleep(unsigned long sleepTime);
    /** test if soft sleep finished **/
    bool finished();
    time_t sleepDurationMs();
    void wakeup();

  protected:
#ifdef ESP8266
    uint8_t _sleepTimeUtcAddress;
#endif
    unsigned int _sleepTime;
    unsigned long _msSleepStart;
  };
}; // namespace

#endif
