//====================================================================================================================//
// File:          mov_av_filter.hpp                                                                                   //
// Description:   Moving Average Filter                                                                               //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//


#ifndef MOV_AV_FILTER_HPP
#define MOV_AV_FILTER_HPP


/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/ 
#include <Arduino.h>


/*--------------------------------------------------------------------------------------------------------------------*\
** Declaration                                                                                                        **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
class MovAvFilter
{
private:
  float ftConstantP;
  float ftPreviousP;
  bool  btEnabledP;
 
public:
  MovAvFilter(void) {};
  ~MovAvFilter(void) {};

  /**
   * @brief Initialisation of Moving Average filter
   * This method should be called only once while setup.
   */
  void init(uint8_t ubConstantV);

  /**
   * @brief     Process the filter.
   * @param[in] ftInputV value that should be filtered
   * This method can be called each time a new value is pending. 
   * The frequency of calls influences the filter behavior.
   */
  float process(float ftInputV);

  //--------------------------------------------------------------------------------------------------- 
  // getter methods
  //
  uint8_t constant(void) {
    return (uint8_t)ftConstantP;
  }
};

#endif // MOV_AV_FILTER_HPP
