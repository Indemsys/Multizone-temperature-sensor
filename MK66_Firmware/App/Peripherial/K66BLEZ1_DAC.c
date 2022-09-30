// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.04.29
// 14:38:37
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"




/*-------------------------------------------------------------------------------------------------------------
  Инициализация выбранного DAC и установка в 0
-------------------------------------------------------------------------------------------------------------*/
void Init_DAC_no_DMA(DAC_MemMapPtr DAC)
{
  SIM_MemMapPtr SIM = SIM_BASE_PTR;

  if (DAC == DAC0_BASE_PTR)
  {
    SIM->SCGC2 |= BIT(12);  // DAC0 clock gate control
  }
  else if (DAC == DAC1_BASE_PTR)
  {
    SIM->SCGC2 |= BIT(13);  // DAC1 clock gate control
  }
  else return;

  DAC->C0 = 0
             + LSHIFT(1, 7) // DACEN     | Starts the Programmable Reference Generator operation
             + LSHIFT(1, 6) // DACRFS    | DAC Reference Select. VDDA is connected to the DACREF_2 input.
             + LSHIFT(0, 5) // DACTRGSEL | DAC Trigger Select
             + LSHIFT(0, 4) // DACSWTRG  | DAC Software Trigger
             + LSHIFT(0, 3) // LPEN      | DAC Low Power Control
             + LSHIFT(0, 2) // DACBWIEN  | DAC Buffer Watermark Interrupt Enable
             + LSHIFT(0, 1) // DACBTIEN  | DAC Buffer Read Pointer Top Flag Interrupt Enable
             + LSHIFT(0, 0) // DACBBIEN  | DAC Buffer Read Pointer Bottom Flag Interrupt Enable
  ;
  DAC->C1 = 0
             + LSHIFT(0, 7) // DMAEN     | DMA Enable Select
             + LSHIFT(0, 3) // DACBFWM   | DAC Buffer Watermark Select
             + LSHIFT(0, 1) // DACBFMD   | DAC Buffer Work Mode Select
             + LSHIFT(0, 0) // DACBFEN   | DAC Buffer Enable
  ;
  DAC->C2 = 0
             + LSHIFT(0, 4) // DACBFRP  | DAC Buffer Read Pointer
             + LSHIFT(0, 0) // DACBFUP  | DAC Buffer Upper Limit
  ;

  DAC->DAT[0].DATH=0;
  DAC->DAT[0].DATL=0;
}

/*-----------------------------------------------------------------------------------------------------


  \param DAC
-----------------------------------------------------------------------------------------------------*/
void Init_DAC_with_DMA(DAC_MemMapPtr DAC)
{
  SIM_MemMapPtr SIM = SIM_BASE_PTR;

  if (DAC == DAC0_BASE_PTR)
  {
    SIM->SCGC2 |= BIT(12);  // DAC0 clock gate control
  }
  else if (DAC == DAC1_BASE_PTR)
  {
    SIM->SCGC2 |= BIT(13);  // DAC1 clock gate control
  }
  else return;

  DAC->C0 = 0
             + LSHIFT(1, 7) // DACEN     | Starts the Programmable Reference Generator operation
             + LSHIFT(1, 6) // DACRFS    | DAC Reference Select. VDDA is connected to the DACREF_2 input.
             + LSHIFT(0, 5) // DACTRGSEL | DAC Trigger Select
             + LSHIFT(0, 4) // DACSWTRG  | DAC Software Trigger
             + LSHIFT(0, 3) // LPEN      | DAC Low Power Control
             + LSHIFT(0, 2) // DACBWIEN  | DAC Buffer Watermark Interrupt Enable
             + LSHIFT(0, 1) // DACBTIEN  | DAC Buffer Read Pointer Top Flag Interrupt Enable
             + LSHIFT(0, 0) // DACBBIEN  | DAC Buffer Read Pointer Bottom Flag Interrupt Enable
  ;
  DAC->C1 = 0
             + LSHIFT(1, 7) // DMAEN     | DMA Enable Select
             + LSHIFT(0, 3) // DACBFWM   | DAC Buffer Watermark Select
             + LSHIFT(0, 1) // DACBFMD   | DAC Buffer Work Mode Select
             + LSHIFT(0, 0) // DACBFEN   | DAC Buffer Enable
  ;
  DAC->C2 = 0
             + LSHIFT(0, 4) // DACBFRP  | DAC Buffer Read Pointer
             + LSHIFT(0, 0) // DACBFUP  | DAC Buffer Upper Limit
  ;

  DAC->DAT[0].DATH=0;
  DAC->DAT[0].DATL=0;


}

/*-------------------------------------------------------------------------------------------------------------
  Установка значения в выбранном DAC
-------------------------------------------------------------------------------------------------------------*/
void Set_DAC_val(DAC_MemMapPtr DAC, uint16_t val)
{
  //DAC_MemMapPtr DAC0 = DAC0_BASE_PTR;
  DAC->DAT[0].DATH= val >> 8;
  DAC->DAT[0].DATL= val & 0xFF;
}

/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
uint16_t Get_DAC_val(DAC_MemMapPtr DAC)
{
  uint16_t val;

  val = (DAC->DAT[0].DATH << 8) | (DAC->DAT[0].DATL & 0xFF);
  return val;
}


