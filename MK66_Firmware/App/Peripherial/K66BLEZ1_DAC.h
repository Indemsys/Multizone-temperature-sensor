#ifndef __K66BLEZ_DAC_CONTROL
  #define __K66BLEZ_DAC_CONTROL


#define DAC_MID_LEVEL          (4096/2)


void     Init_DAC_no_DMA(DAC_MemMapPtr DAC);
void     Init_DAC_with_DMA(DAC_MemMapPtr DAC);
void     Set_DAC_val(DAC_MemMapPtr DAC, uint16_t val);
uint16_t Get_DAC_val(DAC_MemMapPtr DAC);


#endif



