#include "App.h"


static const T_IO_pins_configuration PWM_outs_pins_conf[] =
{
  { PTE_BASE_PTR, PORTE_BASE_PTR,   6,   IRQ_DIS,   0,   ALT6, DSE_HI, FAST_SLEW, OD_DIS, PFE_DIS, PUPD_DIS, GP_OUT,   0 }, // OUT18_O          9 # PTE6/LLWU_P16 # Default=(DISABLED)  ALT0=()  ALT1=(PTE6/LLWU_P16)  ALT2=(SPI1_PCS3)  ALT3=(UART3_CTS_b)  ALT4=(I2S0_MCLK)  ALT5=()  ALT6=(FTM3_CH1)  ALT7=(USB0_SOF_OUT)  EZPort=()
  { PTD_BASE_PTR, PORTD_BASE_PTR,   3,   IRQ_DIS,   0,   ALT4, DSE_HI, FAST_SLEW, OD_DIS, PFE_DIS, PUPD_DIS, GP_OUT,   0 }, // OUT19_O          130 # PTD3 # Default=(DISABLED)  ALT0=()  ALT1=(PTD3)  ALT2=(SPI0_SIN)  ALT3=(UART2_TX)  ALT4=(FTM3_CH3)  ALT5=(FB_AD3/SDRAM_A11)  ALT6=()  ALT7=(I2C0_SDA)  EZPort=()
};


/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
void Config_pin(const T_IO_pins_configuration pinc)
{
  pinc.port->PCR[pinc.pin_num] = LSHIFT(pinc.irqc, 16) |
                                LSHIFT(pinc.lock, 15) |
                                LSHIFT(pinc.mux, 8) |
                                LSHIFT(pinc.DSE, 6) |
                                LSHIFT(pinc.ODE, 5) |
                                LSHIFT(pinc.PFE, 4) |
                                LSHIFT(pinc.SRE, 2) |
                                LSHIFT(pinc.PUPD, 0);

  if (pinc.init == 0) pinc.gpio->PCOR = LSHIFT(1, pinc.pin_num);
  else pinc.gpio->PSOR = LSHIFT(1, pinc.pin_num);
  pinc.gpio->PDDR =(pinc.gpio->PDDR & ~LSHIFT(1, pinc.pin_num)) | LSHIFT(pinc.dir, pinc.pin_num);
}





/*------------------------------------------------------------------------------



 \return int
 ------------------------------------------------------------------------------*/
int K66BLEZ1_pwm_outs_pins(void)
{
  int i;
  for (i = 0; i < (sizeof(PWM_outs_pins_conf) / sizeof(PWM_outs_pins_conf[0])); i++)
  {
    Config_pin(PWM_outs_pins_conf[i]);
  }
  return 0;
}

/*-------------------------------------------------------------------------------------------------------------
  1 - снимаем сигнал выборки CS
  0 - устанавливаем сигнал выборки CS
-------------------------------------------------------------------------------------------------------------*/
void Set_MKW40_CS_state(int state)
{
  if (state == 0) PTD_BASE_PTR->PCOR = LSHIFT(1, 11); // Устанавливаем бит в Port Clear Output Register
  else PTD_BASE_PTR->PSOR = LSHIFT(1, 11);              // Устанавливаем бит в Port Set Output Register
}


