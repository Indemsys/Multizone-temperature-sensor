// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2020.07.14
// 16:50:15
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"


/*-----------------------------------------------------------------------------------------------------
  
  
  \param TPM  
-----------------------------------------------------------------------------------------------------*/
static void TPM_switch_on_clk(TPM_MemMapPtr TPM)
{
  if (TPM == TPM1_BASE_PTR)
  {
    // Разрешаем тактирование FTM0
    SIM_SCGC2 |= BIT(9);
  }
  else if (TPM == TPM2_BASE_PTR)
  {
    // Разрешаем тактирование FTM1
    SIM_SCGC2 |= BIT(10);
  }
  else return;
}

/*-----------------------------------------------------------------------------------------------------
  Инициализация таймера TPM для организации воспроизведения аудиосигнала через DAC с помощью DMA
 
  Канал 0 выключен
  Канал 1 выключен
  По переполнению таймера вызывается DMA
  Таймер в режиме непрерывного счета 
 
  Тактирование таймеров производится от системной частоты = 180 МГц
  
  
  \param TPM  принимает значение TPM1_BASE_PTR или TPM2_BASE_PTR
  \param mod  коэффициент деления тактовой частоты 180 МГц
-----------------------------------------------------------------------------------------------------*/
void TPM_Init_for_audio(TPM_MemMapPtr TPM, uint16_t mod)
{


  TPM_switch_on_clk(TPM);

  TPM->CONTROLS[0].CnSC = 0;
  TPM->CONTROLS[1].CnSC = 0;

  TPM->SC = 0
            + LSHIFT(0, 8) // DMA   | DMA Enable
            + LSHIFT(0, 7) // TOF   | Timer Overflow Flag
            + LSHIFT(0, 6) // TOIE  | Timer Overflow Interrupt Enable
            + LSHIFT(0, 5) // CPWMS | Center-Aligned PWM Select
            + LSHIFT(0, 3) // CMOD  | Clock Mode Selection
            + LSHIFT(0, 0) // PS    | Prescale Factor Selection
  ;

  TPM->CNT = 0;
  TPM->MOD = mod;

  TPM->STATUS = 0
                + LSHIFT(1, 8) // TOF  (w1c)| Timer Overflow Flag
                + LSHIFT(1, 1) // CH1F (w1c)| Channel 1 Flag
                + LSHIFT(1, 0) // CH0F (w1c)| Channel 0 Flag
  ;
  TPM->COMBINE = 0
                 + LSHIFT(0, 1) // COMSWAP0 | Combine Channel 0 and 1 Swap
                 + LSHIFT(0, 0) // COMBINE0 | Combine Channels 0 and 1
  ;
  TPM->POL = 0
             + LSHIFT(0, 1) // POL1 | Channel 1 Polarity
             + LSHIFT(0, 0) // POL0 | Channel 0 Polarity
  ;
  TPM->FILTER = 0
                + LSHIFT(0x0F, 4) // CH1FVAL | Channel 1 Filter Value
                + LSHIFT(0x0F, 0) // CH0FVAL | Channel 0 Filter Value
  ;
  TPM->QDCTRL = 0
                + LSHIFT(0, 3) // QUADMODE | Quadrature Decoder Mode
                + LSHIFT(0, 2) // QUADIR   | Counter Direction in Quadrature Decode Mode
                + LSHIFT(0, 1) // TOFDIR   | Indicates if the TOF bit was set on the top or the bottom of counting
                + LSHIFT(0, 0) // QUADEN   | Enables the quadrature decoder mode.
  ;
  TPM->CONF = 0
              + LSHIFT(0, 24) // TRGSEL  | Trigger Select
              + LSHIFT(0, 23) // TRGSRC  | Trigger Source
              + LSHIFT(0, 22) // TRGPOL  | Trigger Polarity
              + LSHIFT(0, 19) // CPOT    | Counter Pause On Trigger
              + LSHIFT(0, 18) // CROT    | Counter Reload On Trigger
              + LSHIFT(0, 17) // CSOO    | Counter Stop On Overflow
              + LSHIFT(0, 16) // CSOT    | Counter Start on Trigger
              + LSHIFT(0, 9)  // GTBEEN  | Global time base enable
              + LSHIFT(0, 8)  // GTBSYNC | Global Time Base Synchronization
              + LSHIFT(0, 6)  // DBGMODE | Debug Mode
              + LSHIFT(0, 5)  // DOZEEN  | Doze Enable
  ;

  // Канал 0 выключен
  TPM->CONTROLS[0].CnSC = 0
                          + LSHIFT(1, 7) // CHF  (w1c)| Channel Flag
                          + LSHIFT(0, 6) // CHIE      | Channel Interrupt Enable
                          + LSHIFT(0, 5) // MSB       | Channel Mode Select
                          + LSHIFT(0, 4) // MSA       | Channel Mode Select
                          + LSHIFT(0, 3) // ELSB      | Edge or Level Select
                          + LSHIFT(1, 2) // ELSA      | Edge or Level Select
                          + LSHIFT(0, 0) // DMA       | DMA Enable
  ;
  // Канал 1 выключен
  TPM->CONTROLS[1].CnSC = 0
                          + LSHIFT(1, 7) // CHF  (w1c)| Channel Flag
                          + LSHIFT(0, 6) // CHIE      | Channel Interrupt Enable
                          + LSHIFT(0, 5) // MSB       | Channel Mode Select
                          + LSHIFT(0, 4) // MSA       | Channel Mode Select
                          + LSHIFT(0, 3) // ELSB      | Edge or Level Select
                          + LSHIFT(1, 2) // ELSA      | Edge or Level Select
                          + LSHIFT(0, 0) // DMA       | DMA Enable
  ;

  TPM->SC = 0
            + LSHIFT(1, 8) // DMA   | DMA Enable
            + LSHIFT(1, 7) // TOF   | Timer Overflow Flag
            + LSHIFT(0, 6) // TOIE  | Timer Overflow Interrupt Enable
            + LSHIFT(0, 5) // CPWMS | Center-Aligned PWM Select
            + LSHIFT(1, 3) // CMOD  | Clock Mode Selection      | 01 TPM counter increments on every TPM counter clock
            + LSHIFT(0, 0) // PS    | Prescale Factor Selection | 000 Divide by 1
  ;
}

