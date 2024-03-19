//====================================================================================================================//
// File:          uart_mux.hpp                                                                                        //
// Description:   PhotoVoltaics Zero - UART MUX                                                                       //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

#ifndef UART_MUX_HPP
#define UART_MUX_HPP

/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include <Arduino.h>

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and Enums                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

/**
 * @brief This API is written for *SN74LV4052A* to select different interface via INH, A and B control lines
 *
 * An interface is enabled by using \c #enable() method an providing interface from \c #Interface_e enumeration.
 */

#ifndef UART_MUX_PIN_INH
#define UART_MUX_PIN_INH 5
#endif

#ifndef UART_MUX_PIN_A
#define UART_MUX_PIN_A 19
#endif

#ifndef UART_MUX_PIN_B
#define UART_MUX_PIN_B 18
#endif

/**
 * @brief
 *
 */
class UartMux
{

public:
  //---------------------------------------------------------------------------------------------------
  // enum definition of multiplexed interfaces
  //
  typedef enum Interface_e
  {
    eIF_1 = 0, //!< INH: L, B: L, A: L | 1Y0, 2Y0
    eIF_2,     //!< INH: L, B: L, A: H | 1Y1, 2Y1
    eIF_3,     //!< INH: L, B: H, A: L | 1Y2, 2Y2
    eIF_4,     //!< INH: L, B: H, A: H | 1Y3, 2Y3
    eIF_NONE   //!< INH: H, B: x, A: x |   NONE
  } Interface_te;

  UartMux();
  ~UartMux();

  /**
   * @brief Initialisation the multiplexer
   * This method should be called only once while setup.
   */
  void init();

  /**
   * @brief Enable provided multiplexed interface number
   *
   * @param[in] teIfV     interface number from \c #Interface_e enumeration
   */
  void enable(Interface_te teIfV);

  /**
   * @brief Return interface number that actually is enabled
   *
   * @return pending interface number defined by \c #Interface_te
   */
  Interface_te get();

private:
  /**
   * @brief locally stored interface number that actually is enabled
   */
  Interface_te tePendingMuxP;
};

#endif // UART_MUX_HPP
