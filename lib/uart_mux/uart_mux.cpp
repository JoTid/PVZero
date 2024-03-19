//====================================================================================================================//
// File:          uart_mux.cpp                                                                                        //
// Description:   PhotoVoltaics Zero - UART MUX                                                                       //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include "uart_mux.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
UartMux::UartMux()
{
  tePendingMuxP = eIF_NONE;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
UartMux::~UartMux()
{
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void UartMux::init()
{
  //---------------------------------------------------------------------------------------------------
  // initialise port pins
  //
  pinMode(UART_MUX_PIN_INH, OUTPUT);
  pinMode(UART_MUX_PIN_B, OUTPUT);
  pinMode(UART_MUX_PIN_A, OUTPUT);

  //---------------------------------------------------------------------------------------------------
  // deselect the mux interface
  //
  digitalWrite(UART_MUX_PIN_INH, HIGH);
  digitalWrite(UART_MUX_PIN_B, LOW);
  digitalWrite(UART_MUX_PIN_A, LOW);

  tePendingMuxP = eIF_NONE;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void UartMux::enable(Interface_te teIfV)
{
  //---------------------------------------------------------------------------------------------------
  //
  //
  switch (teIfV)
  {
  case eIF_1:
    digitalWrite(UART_MUX_PIN_INH, LOW);
    digitalWrite(UART_MUX_PIN_B, LOW);
    digitalWrite(UART_MUX_PIN_A, LOW);
    tePendingMuxP = eIF_1;
    break;

  case eIF_2:
    digitalWrite(UART_MUX_PIN_INH, LOW);
    digitalWrite(UART_MUX_PIN_B, LOW);
    digitalWrite(UART_MUX_PIN_A, HIGH);
    tePendingMuxP = eIF_2;
    break;

  case eIF_3:
    digitalWrite(UART_MUX_PIN_INH, LOW);
    digitalWrite(UART_MUX_PIN_B, HIGH);
    digitalWrite(UART_MUX_PIN_A, LOW);
    tePendingMuxP = eIF_3;
    break;

  case eIF_4:
    digitalWrite(UART_MUX_PIN_INH, LOW);
    digitalWrite(UART_MUX_PIN_B, HIGH);
    digitalWrite(UART_MUX_PIN_A, HIGH);
    tePendingMuxP = eIF_4;
    break;

  case eIF_NONE:
  default:
    digitalWrite(UART_MUX_PIN_INH, HIGH);
    digitalWrite(UART_MUX_PIN_B, LOW);
    digitalWrite(UART_MUX_PIN_A, LOW);
    tePendingMuxP = eIF_NONE;
    break;
  }
}