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

#ifndef UART_CLASS_H
#define UART_CLASS_H

#include <Arduino.h>
#include "pvz_psu.hpp"
#include "pvz_mppt.hpp"
#include "uart_mux.hpp"

namespace PVZ
{

  //---------------------------------------------------------------------------------------------------------
  typedef enum UartAppSm_e
  {
    eUART_APP_SM_MPPT_e = 0,
    eUART_APP_SM_PSU1_e,
    eUART_APP_SM_PSU2_e,
  } UartAppSm_te;

  class Uart
  {
  public:
    Uart(PvzMppt &mppt, PvzPsu &psu1, PvzPsu &psu2);
    ~Uart();
    void setup();

  protected:
    TaskHandle_t clTaskUartAppP;
    static void taskUartApp(void *pvParameters);

    PvzMppt &clMpptP;
    PvzPsu &aclPsuP1;
    PvzPsu &aclPsuP2;

    UartMux clUartMuxP;
    int32_t slReadBytesT = 0;
    char aszReadDataT[256];
  };

}; // namespace

#endif
