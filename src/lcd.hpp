
/*--------------------------------------------------------------------------------------------------------------------*\
** Definitions and Enums                                                                                              **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
#ifndef PVZ_LCD_H
#define PVZ_LCD_H

/*--------------------------------------------------------------------------------------------------------------------*\
** Declaration                                                                                                        **
**                                                                                                                    **
\*--------------------------------------------------------------------------------------------------------------------*/
namespace PVZERO
{
  class LCD
  {
  private:
    /* data */
    typedef enum lcd_e
    {
      eLCD_AppInit = 0,
      eLCD_AppRunOK,
      eLCD_AppRunFailure,
    } lcd_te;

    void progressBar3(void);

    /**
     * @brief Refresh time of LCD given in milliseconds
     */
    #define LCD_REFRESH_TIME 20

  public:
    LCD(void);
    ~LCD(void);
    void init(void);
    void process(void);
    void busy(void);
    void warning(void);
  };
};     // namespace

#endif // PVZ_LCD_H
