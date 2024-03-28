
#include <mutex>
#include <ewcInterface.h>
#include "pvzero_interface.h"
#include "uart.h"

using namespace EWC;
using namespace PVZ;

Uart::Uart(PvzMppt &mppt, PvzPsu &psu1, PvzPsu &psu2) : clMpptP(mppt), aclPsuP1(psu1), aclPsuP2(psu2)
{
}

Uart::~Uart()
{
}

void Uart::setup()
{
  //---------------------------------------------------------------------------------------------------
  // Configure a task for UART application that handles Serial2 communication with both PSUs and the
  // Victron SmartSolar charger.
  //
  xTaskCreatePinnedToCore(
      this->taskUartApp, /* Function to implement the task */
      "UartApp",         /* Name of the task */
      2048,              /* Stack size in words */
      this,              /* Task input parameter */
      0,                 /* Priority of the task */
      &clTaskUartAppP,   /* Task handle. */
      0);                /* Core where the task should run */
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void Uart::taskUartApp(void *_this)
{
  Uart *uart = static_cast<Uart *>(_this);
  static uint32_t uqTimeStampT = 0;
  static UartAppSm_te teUartAppStateG = eUART_APP_SM_MPPT_e;
  static int32_t aslPsuStateT[] = {0, 0};
  static int32_t aslPsuStateBeginT[] = {0, 0};
  static bool abtPsuInitSuccessT[] = {false, false};
  static int32_t slPsuReturnT;

  //---------------------------------------------------------------------------------------------------
  // intialise the multiplexer and enable IF 1 for victron
  //
  uart->clUartMuxP.init();
  uart->clUartMuxP.enable(UartMux::eIF_1);

  //---------------------------------------------------------------------------------------------------
  // perform configuration of serial 2
  //
  Serial2.begin(19200);

  while (true)
  {
    //-------------------------------------------------------------------------------------------
    // handle state and the corresponding transitions of the Serial2 state machine
    //
    switch (teUartAppStateG)
    {
    case eUART_APP_SM_MPPT_e:

      //-----------------------------------------------------------------------------------
      // store all incoming chars, this requires about 95 ms
      //
      while (Serial2.available())
      {
        // copy and increase char counter avoid invalid memory access
        if (uart->slReadBytesT < 256)
        {
          uart->aszReadDataT[uart->slReadBytesT] = Serial2.read();
        }
        else
        {
          uart->aszReadDataT[254] = Serial2.read();
        }

        uart->slReadBytesT++;

        //---------------------------------------------------------------------------
        // store time when read the last char for later evaluation
        //
        uqTimeStampT = millis();
      }

      //-----------------------------------------------------------------------------------
      // assume that after 10 ms of silence all data has been received from victron
      //
      if ((millis() > (uqTimeStampT + 10)) && (uart->slReadBytesT > 0))
      {
        //---------------------------------------------------------------------------
        // append the a null to terminate the string
        //
        uart->aszReadDataT[uart->slReadBytesT] = '\0'; // Append a null

        //---------------------------------------------------------------------------
        // provide the received frame from victron to the mppt class for processing
        //
        uart->clMpptP.updateFrame(uart->aszReadDataT, uart->slReadBytesT);

        //---------------------------------------------------------------------------
        // pre pare next reception data
        //
        uart->slReadBytesT = 0;

        ///---------------------------------------------------------------------------
        ///---------------------------------------------------------------------------
        // before next send of frame from victron we have at least 900ms time
        // poll an set both PSUs
        //

        //---------------------------------------------------------------------------
        // perform transition to the next state,
        // make sure the serial buffers are reset
        //
        Serial2.flush();
        uart->clUartMuxP.enable(UartMux::eIF_2);
        delay(10);
        Serial2.flush();
        teUartAppStateG = eUART_APP_SM_PSU1_e;
      }
      break;

    case eUART_APP_SM_PSU1_e:
      //-----------------------------------------------------------------------------------
      // make sure we leave this state 300ms after final char of Victron Text frame
      // => 105 ms for reception and storage of MPPT Text Frame, than make uqTimeStampT
      //    + 400 ms for PSU0
      //
      if (millis() > (uqTimeStampT + 400))
      {
        //---------------------------------------------------------------------------
        // define where should we at new cycle
        //
        aslPsuStateT[0] = aslPsuStateBeginT[0];

        //---------------------------------------------------------------------------
        // perform transition to the next state,
        // make sure the serial buffers are reset
        //
        Serial2.flush();
        uart->clUartMuxP.enable(UartMux::eIF_3);
        delay(10);
        Serial2.flush();
        teUartAppStateG = eUART_APP_SM_PSU2_e;
      }

      //-----------------------------------------------------------------------------------
      // stay in this state and treat the PSU at MUX IF 2
      //
      else
      {
        switch (aslPsuStateT[0])
        {
          // init state of PSU
          // - in case of error this state needs about 105 ms
          // - in case success this state needs about 35 ms
        case 0:
          slPsuReturnT = uart->aclPsuP1.init(Serial2);

          if (slPsuReturnT == 1)
          {
            uart->aclPsuP1.enable(true);
            aslPsuStateBeginT[0] = 1;
            aslPsuStateT[0] = 1;
          }
          else
          {
            aslPsuStateT[0] = 10; // fail to init, may be not connected
          }
          break;

          // Write and Read
          // - in case of error this state needs about 105 ms
          // - in case of success this state needs about 55 ms
        case 1:
          slPsuReturnT = uart->aclPsuP1.write();
          if (slPsuReturnT < 0)
          {
            aslPsuStateT[0] = 11; // fail to init, may be not connected
          }
          else
          {
            slPsuReturnT = uart->aclPsuP1.read();
            if (slPsuReturnT < 0)
            {
              aslPsuStateT[0] = 12; // fail to init, may be not connected
            }
            else
            {
              aslPsuStateT[0] = 5;
            }
          }
          break;

        case 5: // Success, we are finish in this cycle, stay in here
          break;

        case 10: // Error State: Fail to init the PSU
          EWC::I::get().logger() << "PSU0 Error at init: " << slPsuReturnT << endl;
          aslPsuStateT[0] = 100;
          break;

        case 11: // Error State: while writing to PSU
          EWC::I::get().logger() << "PSU0 Error at write: " << slPsuReturnT << endl;
          aslPsuStateT[0] = 100;
          break;

        case 12: // Error State: while reading from PSU
          EWC::I::get().logger() << "PSU0 Error at read: " << slPsuReturnT << endl;
          aslPsuStateT[0] = 100;
          break;

        case 100: // Error State: prepare re-initialisation
          aslPsuStateBeginT[0] = 0;
          Serial2.flush();
        default:
          break;
        }
      }

      break;
    case eUART_APP_SM_PSU2_e:

      //-----------------------------------------------------------------------------------
      // make sure we leave this state 800ms after final char of Victron Text frame
      // => 105 ms for reception and storage of MPPT Text Frame, than make uqTimeStampT
      //    + 400 ms for PSU0
      //    + 400 ms for PSU1
      //
      if (millis() > (uqTimeStampT + 800))
      {

        //---------------------------------------------------------------------------
        // define where should we at new cycle
        //
        aslPsuStateT[1] = aslPsuStateBeginT[1];

        //---------------------------------------------------------------------------
        // perform transition to the next state,
        // make sure the serial buffers are reset
        //
        Serial2.flush();
        uart->clUartMuxP.enable(UartMux::eIF_1);
        delay(10);
        Serial2.flush();
        teUartAppStateG = eUART_APP_SM_MPPT_e;
      }

      //-----------------------------------------------------------------------------------
      // stay in this state and treat the PSU at MUX IF 3
      //
      else
      {
        switch (aslPsuStateT[1])
        {
          // init state of PSU
          // - in case of error this state needs about 105 ms
          // - in case success this state needs about 35 ms
        case 0:
          slPsuReturnT = uart->aclPsuP2.init(Serial2);

          if (slPsuReturnT == 1)
          {
            uart->aclPsuP2.enable(true);
            aslPsuStateBeginT[1] = 1;
            aslPsuStateT[1] = 1;
          }
          else
          {
            aslPsuStateT[1] = 10; // fail to init, may be not connected
          }
          break;

          // Write and Read
          // - in case of error this state needs about 105 ms
          // - in case of success this state needs about 55 ms
        case 1:
          slPsuReturnT = uart->aclPsuP2.write();
          if (slPsuReturnT < 0)
          {
            aslPsuStateT[1] = 11; // fail to init, may be not connected
          }
          else
          {
            slPsuReturnT = uart->aclPsuP2.read();
            if (slPsuReturnT < 0)
            {
              aslPsuStateT[1] = 12; // fail to init, may be not connected
            }
            else
            {
              aslPsuStateT[1] = 5;
            }
          }

          break;

        case 5: // Success, we are finish in this cycle, stay in here
          break;

        case 10: // Error State: Fail to init the PSU
          EWC::I::get().logger() << "PSU1 Error at init: " << slPsuReturnT << endl;
          aslPsuStateT[1] = 100;
          break;

        case 11: // Error State: while writing to PSU
          EWC::I::get().logger() << "PSU1 Error at write: " << slPsuReturnT << endl;
          aslPsuStateT[1] = 100;
          break;

        case 12: // Error State: while reading from PSU
          EWC::I::get().logger() << "PSU1 Error at read: " << slPsuReturnT << endl;
          aslPsuStateT[1] = 100;
          break;

        case 100: // Error State: prepare re-initialisation
          aslPsuStateBeginT[1] = 0;
          Serial2.flush();
        default:
          break;
        }
      }
      break;

    default:
      break;
    }
  }
}