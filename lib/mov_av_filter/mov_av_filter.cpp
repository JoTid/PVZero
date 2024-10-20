//====================================================================================================================//
// File:          mov_av_filter.cpp                                                                                   //
// Description:   Moving Average Filter                                                                               //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//

/*--------------------------------------------------------------------------------------------------------------------*\
** Include Files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include "mov_av_filter.hpp"

/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and variables of module                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
void MovAvFilter::init(uint8_t ubConstantV)
{
  //---------------------------------------------------------------------------------------------------
  // set filter constant
  //
  if (ubConstantV > 0)
  {
    ftConstantP = (float)ubConstantV;
  }
  else
  {
    ftConstantP = 1.0;
  }

  //---------------------------------------------------------------------------------------------------
  // filter will be enabled at first input has been provided via process()
  //
  btEnabledP = false;
}

//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//                                                                                                                    //
//--------------------------------------------------------------------------------------------------------------------//
float MovAvFilter::process(float ftInputV)
{
  float ftDeltaValueT;

  //---------------------------------------------------------------------------------------------------
  // filter value
  //
  if (btEnabledP == true)
  {
    //-------------------------------------------------------------------------------------------
    // proceed filter formula: Output = Previous + ((Input - Previous) / Constant)
    //
    ftDeltaValueT = (ftInputV - ftPreviousP);
    ftInputV = ftDeltaValueT / ftConstantP;
    ftInputV += ftPreviousP;

    //-------------------------------------------------------------------------------------------
    // If the absolute delta value is less than the filter constant,
    // then an approximation of the filtered value to the actual value have to be done
    //
    if (ftConstantP > abs(ftDeltaValueT))
    {
      if (ftDeltaValueT < 0.1)
      {
        ftPreviousP -= (1.0 / ftConstantP);
      }
      else if (ftDeltaValueT > 0.1)
      {
        ftPreviousP += (1.0 / ftConstantP);
      }
    }
    else
    {
      ftPreviousP = ftInputV;
    }
  }

  //---------------------------------------------------------------------------------------------------
  // initial run of filter
  //
  else
  {
    ftPreviousP = ftInputV;
    btEnabledP = true;
  }

  return ftInputV;
}
