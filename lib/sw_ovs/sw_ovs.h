//====================================================================================================================//
// File:          sw_ovs.h                                                                                            //
// Description:   Oversampling Unit                                                                                   //
// Author:        Tiderko                                                                                             //
//--------------------------------------------------------------------------------------------------------------------//
//                                                                                                                    //
//====================================================================================================================//


#ifndef  MC_OVS_H_
#define  MC_OVS_H_


/*--------------------------------------------------------------------------------------------------------------------*\
** Include files                                                                                                      **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#include <Arduino.h>

//------------------------------------------------------------------------------------------------------
/*!
** \file    mc_ovs.h
**
** This is a software based oversampler unit.
** 
** The oversampling algorithm works by repeatedly adding new samples to a running total. 
** The number of samples that are added depends on the number of bits that the user wants 
** to extend the input value by. 
** For example, if the user wants to extend the input value by 4 bits, 
** then the algorithm will add 256 samples.
**
** Here is a list for required samples depending on bit number:
**  0 => 1 
**  1 => 4
**  2 => 16
**  3 => 64
**  4 => 256
**  n => 1<<(2*n)
**
** Example for usage:
** <code>
** // declaration of oversampling unit in application code
** static McOvs_ts  tsOvsInputS;
** ...
** // Initialisation of oversampling unit with number of desired bits, here one bit 
** McOvsInit(&tsOvsInputS, 1); 
** ...
** // Pass new samples to the oversampling unit
** if (McOvsSample(&tsOvsInputS, ulAdcValueT) == true)
** {
**    // get and handle oversampled value
**    ulNewValueT = McOvsGetResult(&tsOvsInputS);
** }
** </code>
*/

//-------------------------------------------------------------------//
// take precautions if compiled with C++ compiler                    //
#ifdef __cplusplus                                                   //
extern "C" {                                                         //
#endif                                                               //
//-------------------------------------------------------------------//


/*--------------------------------------------------------------------------------------------------------------------*\
** Defines / Enumeration                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/


//---------------------------------------------------------------------------------------------------------
/*!
** \def     MC_OVS_VERSION
**
** Version number of oversampling software module.
*/
#define  MC_OVS_VERSION     1.00


/*--------------------------------------------------------------------------------------------------------------------*\
** Structures                                                                                                         **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------------------------------
/*!
** \struct  McOvs_s
** This structure summarises initial configuration parameters for oversampling unit.
** To set or get the parameters of this structure the user should use one of provided function.
*/
typedef struct McOvs_s {
   /*!
    * Number of additional bits
    */
   uint8_t  ubBitNumber;
   /*!
    * Summed values
    */
   uint32_t ulInputSum;
   /*!
    * This value counts from oversampling ration to 0 to calculate the sum of input values.
    * The oversampling ration depends from required number of bits that are provided by the #ubBitNumberV at
    * initialisation. 
    * The formula for oversampling ratio start value is: ulRatioCount = 2^(2*ubBitNumber)
    */
   uint32_t ulRatioCount;
   /*!
    * Number of summations, which is required
    */
   uint32_t ulRatioCountMax;
   /*! 
    * The result of summed and shifted value
    */
   uint32_t ulResult;

} McOvs_ts;



/*--------------------------------------------------------------------------------------------------------------------*\
** Function prototypes                                                                                                **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/

/*!
** \brief      Sample a value
** \param[in]  ptsOvsV pointer to the oversampler unit, defined by \c #McOvs_ts
** \param[in]  ubBitNumberV provides number of bits the input value should be extended to
** \return     returns 0 in case, when all parameters a valid or negative value in case of error.
** 
** The ubBitNumberV should provide value between 0 and 16. Depending on that the number of iterations that 
** are required for a result increases.
** In case the \c #ubBitNumberV value is higher 16 this function returns -2.
** In case the \c #ptsOvsV value is equal 0 this function returns -1.
*/
int32_t McOvsInit(McOvs_ts *ptsOvsV, uint8_t ubBitNumberV);

/*!
** \brief      Sample a value
** \param[in]  ptsOvsV pointer to the oversampler unit, defined by \c #McOvs_ts
** \param[in]  ulInputV is a value that should be considerd by the oversampler unit
** \return     returns true when a new oversampled value has been generated
**
** This function samples a value, so an oversampled value can be generated.
** The number of calls before a new ovesampled value is generated and true is returned depends on 
** parameters that has been provided by call \c #McOvsInit() .
*/
bool McOvsSample(McOvs_ts *ptsOvsV, uint32_t ulInputV);

/*!
** \brief      Get last result from oversampler unit
** \param[in]  ptsOvsV pointer to the oversampler unit, defined by \c #McOvs_ts
** \return     Oversampled value 
** 
** This function returns the last oversampled value. The default value after initialisation is 0.
*/
uint32_t McOvsGetResult(McOvs_ts *ptsOvsV);

//-------------------------------------------------------------------//
#ifdef __cplusplus                                                   //
}                                                                    //
#endif                                                               //
// end of C++ compiler wrapper                                       //
//-------------------------------------------------------------------//

#endif // MC_OVS_H_
