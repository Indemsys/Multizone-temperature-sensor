// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.09.11
// 13:38:51
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "BSP.h"


uint32_t SystemCoreClock;


#if (defined(__ICCARM__))
    #pragma section = ".data"
    #pragma section = ".data_init"
    #pragma section = ".bss"
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : init_data_bss
 * Description   : Make necessary initializations for RAM.
 * - Copy initialized data from ROM to RAM.
 * - Clear the zero-initialized data section.
 * - Copy the vector table from ROM to RAM. This could be an option.  
 *
 * Tool Chians:
 *   __GNUC__   : GCC
 *   __CC_ARM   : KEIL
 *   __ICCARM__ : IAR
 *
 *END**************************************************************************/
void init_data_bss(void)
{
    uint32_t n; 
    
    /* Addresses for VECTOR_TABLE and VECTOR_RAM come from the linker file */
#if defined(__CC_ARM)
    extern uint32_t Image$$VECTOR_ROM$$Base[];
    extern uint32_t Image$$VECTOR_RAM$$Base[];
    extern uint32_t Image$$RW_m_data$$Base[];

    #define __VECTOR_TABLE Image$$VECTOR_ROM$$Base  
    #define __VECTOR_RAM Image$$VECTOR_RAM$$Base  
    #define __RAM_VECTOR_TABLE_SIZE (((uint32_t)Image$$RW_m_data$$Base - (uint32_t)Image$$VECTOR_RAM$$Base))
#elif defined(__ICCARM__)
    extern uint32_t __RAM_VECTOR_TABLE_SIZE[];
    extern uint32_t __VECTOR_TABLE[];  
    extern uint32_t __VECTOR_RAM[];  
#elif defined(__GNUC__)
    extern uint32_t __VECTOR_TABLE[];
    extern uint32_t __VECTOR_RAM[];
    extern uint32_t __RAM_VECTOR_TABLE_SIZE_BYTES[];
    uint32_t __RAM_VECTOR_TABLE_SIZE = (uint32_t)(__RAM_VECTOR_TABLE_SIZE_BYTES);
#endif

    if (__VECTOR_RAM != __VECTOR_TABLE)
    {   
        /* Copy the vector table from ROM to RAM */
        for (n = 0; n < ((uint32_t)__RAM_VECTOR_TABLE_SIZE)/sizeof(uint32_t); n++)
        {
            __VECTOR_RAM[n] = __VECTOR_TABLE[n];
        }
        /* Point the VTOR to the position of vector table */
        SCB->VTOR = (uint32_t)__VECTOR_RAM;
    }
    else
    {
        /* Point the VTOR to the position of vector table */
        SCB->VTOR = (uint32_t)__VECTOR_TABLE;
    }

#if !defined(__CC_ARM) && !defined(__ICCARM__)
    
    /* Declare pointers for various data sections. These pointers
     * are initialized using values pulled in from the linker file */
    uint8_t * data_ram, * data_rom, * data_rom_end;
    uint8_t * bss_start, * bss_end;

    /* Get the addresses for the .data section (initialized data section) */
#if defined(__GNUC__)
    extern uint32_t __DATA_ROM[];
    extern uint32_t __DATA_RAM[];
    extern char __DATA_END[];
    data_ram = (uint8_t *)__DATA_RAM;
    data_rom = (uint8_t *)__DATA_ROM;
    data_rom_end  = (uint8_t *)__DATA_END;
    n = data_rom_end - data_rom;
#endif

    /* Copy initialized data from ROM to RAM */
    while (n--)
    {
        *data_ram++ = *data_rom++;
    }   
    
    /* Get the addresses for the .bss section (zero-initialized data) */
#if defined(__GNUC__)
    extern char __START_BSS[];
    extern char __END_BSS[];
    bss_start = (uint8_t *)__START_BSS;
    bss_end = (uint8_t *)__END_BSS;
#endif
    
    /* Clear the zero-initialized data section */
    n = bss_end - bss_start;
    while(n--)
    {
        *bss_start++ = 0;
    }
#endif /* !__CC_ARM && !__ICCARM__*/
}

/*-----------------------------------------------------------------------------------------------------
  Вызывается в модуле startup до того как будут инициализированы области переменных
  поэтому здесь нельзя назначать значения рабочим переменным
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
void SystemInit (void)
{
  SIM->COPC = (uint32_t)0x00u; // Выключаем watchdog

  if ((PMC->REGSC & PMC_REGSC_ACKISO_MASK) != 0x00U)
  {
    PMC->REGSC |= PMC_REGSC_ACKISO_MASK; /* Release hold with ACKISO:  Only has an effect if recovering from VLLSx.*/
    SMC->STOPCTRL = 0;
    SMC->PMCTRL = 0;
  }
  /* enable clock for PORTs */
  SIM->SCGC5 |= BIT(9);  // Port A Clock Gate Control
  SIM->SCGC5 |= BIT(10); // Port B Clock Gate Control
  SIM->SCGC5 |= BIT(11); // Port C Clock Gate Control

  SMC->PMPROT = 0
                + LSHIFT(1, 5) // AVLP  | Allow Very-Low-Power Modes
                + LSHIFT(1, 3) // ALLS  | Allow Low-Leakage Stop Mode
                + LSHIFT(1, 1) // AVLLS | Allow Very-Low-Leakage Stop Mode
  ;

  BSP_Init_Pins();

  Clock_init(); // Init board clock

  SIM->SOPT2 = (SIM->SOPT2 & ~LSHIFT(0x7, 5)) | LSHIFT(2, 5); // На выходе CLKOUT - частота шины (16 МГц)
  // 000 OSCERCLK DIV2
  // 001 OSCERCLK DIV4
  // 010 Bus clock
  // 011 LPO clock 1 kHz
  // 100 MCGIRCLK
  // 101 OSCERCLK DIV8
  // 110 OSCERCLK
  // 111 Reserved


}
