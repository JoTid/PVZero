
#include <mutex>
#include <ewcInterface.h>
#include "pvzero_interface.h"
#include "uart.h"

using namespace EWC;
using namespace PVZ;

#ifdef UART_APP_TIME_MEASUREMENT
static uint64_t uqTimeMeas_MpptStartP;
static uint64_t uqTimeMeas_MpptStopP;
static uint64_t uqTimeMeas_MpptDeltaP;

static uint64_t uqTimeMeas_Psu1StartP;
static uint64_t uqTimeMeas_Psu1StopP;
static uint64_t uqTimeMeas_Psu1DeltaP;

static uint64_t uqTimeMeas_Psu2StartP;
static uint64_t uqTimeMeas_Psu2StopP;
static uint64_t uqTimeMeas_Psu2DeltaP;

static uint64_t uqTimeMeas_TotalStartP;
static uint64_t uqTimeMeas_TotalStopP;
static uint64_t uqTimeMeas_TotalDeltaP;
#endif

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
  int32_t slReadCharT;
  Uart *uart = static_cast<Uart *>(_this);
  static uint32_t uqTimeStampT = 0;
  static UartAppSm_te teUartAppStateG = eUART_APP_SM_MPPT_e;
  static int32_t aslPsuStateT[] = {0, 0};
  static int32_t aslPsuStateBeginT[] = {0, 0};
  // static bool abtPsuInitSuccessT[] = {false, false};
  static int32_t slPsuReturnT;

  Serial.print("enter taskUartApp() running on core ");
  Serial.println(xPortGetCoreID());

  //---------------------------------------------------------------------------------------------------
  // perform configuration of serial 2
  //
  Serial2.begin(19200);
  Serial2.flush();
  delay(10);
  do
  {
    slReadCharT = Serial2.read();
  } while (slReadCharT >= 0);

  //---------------------------------------------------------------------------------------------------
  // intialise the multiplexer and enable IF 1 for victron
  //
  uart->clUartMuxP.init();
  uart->clUartMuxP.enable(UartMux::eIF_1);

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
#ifdef UART_APP_TIME_MEASUREMENT
        if (uart->slReadBytesT == 0)
        {
          uqTimeMeas_MpptStartP = millis();
        }
#endif
        // copy and increase char counter avoid invalid memory access
        if (uart->slReadBytesT < 254)
        {
          uart->aszReadDataT[uart->slReadBytesT] = Serial2.read();
        }
        else
        {
          uart->aszReadDataT[254] = Serial2.read();
          uart->slReadBytesT = 254;
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
      if ((millis() > (uqTimeStampT + 20)) && (uart->slReadBytesT > 0))
      {

#ifdef UART_APP_TIME_MEASUREMENT
        uqTimeMeas_MpptStopP = millis();
        uqTimeMeas_MpptDeltaP = uqTimeMeas_MpptStopP - uqTimeMeas_MpptStartP;
        Serial.print("MT: ");
        Serial.print(uqTimeMeas_MpptDeltaP);
        Serial.print(" = ");
        Serial.print(uqTimeMeas_MpptStopP);
        Serial.print(" - ");
        Serial.println(uqTimeMeas_MpptStartP);
#endif

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
        Serial2.flush();
        delay(10);
        do
        {
          slReadCharT = Serial2.read();
        } while (slReadCharT >= 0);

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
        Serial2.flush();
        delay(10);
        do
        {
          slReadCharT = Serial2.read();
        } while (slReadCharT >= 0);

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
            uart->aclPsuP1.write();
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

#ifdef UART_APP_TIME_MEASUREMENT
          uqTimeMeas_Psu1StartP = millis();
#endif

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
#ifdef UART_APP_TIME_MEASUREMENT
              uqTimeMeas_Psu1StopP = millis();
              uqTimeMeas_Psu1DeltaP = uqTimeMeas_Psu1StopP - uqTimeMeas_Psu1StartP;
              Serial.print("P1: ");
              Serial.print(uqTimeMeas_Psu1DeltaP);
              Serial.print(" = ");
              Serial.print(uqTimeMeas_Psu1StopP);
              Serial.print(" - ");
              Serial.println(uqTimeMeas_Psu1StartP);
#endif
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
          aslPsuStateT[0] = 5; // Errors that occur during operation are only reported
          break;

        case 12: // Error State: while reading from PSU
          EWC::I::get().logger() << "PSU0 Error at read: " << slPsuReturnT << endl;
          aslPsuStateT[0] = 5; // Errors that occur during operation are only reported
          break;

        case 100: // Error State: prepare re-initialisation
          aslPsuStateBeginT[0] = 0;
          Serial2.flush();
          delay(10);
          do
          {
            slReadCharT = Serial2.read();
          } while (slReadCharT >= 0);
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
        Serial2.flush();
        delay(10);
        do
        {
          slReadCharT = Serial2.read();
        } while (slReadCharT >= 0);

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
            uart->aclPsuP2.write();
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

#ifdef UART_APP_TIME_MEASUREMENT
          uqTimeMeas_Psu2StartP = millis();
#endif

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
#ifdef UART_APP_TIME_MEASUREMENT
              uqTimeMeas_Psu2StopP = millis();
              uqTimeMeas_Psu2DeltaP = uqTimeMeas_Psu2StopP - uqTimeMeas_Psu2StartP;
              Serial.print("P2: ");
              Serial.print(uqTimeMeas_Psu2DeltaP);
              Serial.print(" = ");
              Serial.print(uqTimeMeas_Psu2StopP);
              Serial.print(" - ");
              Serial.println(uqTimeMeas_Psu2StartP);
#endif
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
          aslPsuStateT[1] = 5; // Errors that occur during operation are only reported
          break;

        case 12: // Error State: while reading from PSU
          EWC::I::get().logger() << "PSU1 Error at read: " << slPsuReturnT << endl;
          aslPsuStateT[1] = 5; // Errors that occur during operation are only reported
          break;

        case 100: // Error State: prepare re-initialisation
          aslPsuStateBeginT[1] = 0;
          Serial2.flush();
          delay(10);
          do
          {
            slReadCharT = Serial2.read();
          } while (slReadCharT >= 0);

        default:
          break;
        }
      }
      break;

    default:
      break;
    }

    delay(10);
  }
}