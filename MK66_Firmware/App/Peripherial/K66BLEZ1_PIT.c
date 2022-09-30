#include "App.h"

static void PIT0_isr(void *user_isr_ptr);
static void PIT1_isr(void *user_isr_ptr);
static void PIT2_isr(void *user_isr_ptr);
static void PIT3_isr(void *user_isr_ptr);

static INT_ISR_FPTR    PIT_isrs[PIT_CHANNELS_NUM] = {PIT0_isr, PIT1_isr, PIT2_isr, PIT3_isr};
static int32_t         PIT_int_nums[PIT_CHANNELS_NUM] = {INT_PIT0, INT_PIT1, INT_PIT2, INT_PIT3};
static T_PIT_callback  PIT_callbacks[PIT_CHANNELS_NUM] = {0};

/*-------------------------------------------------------------------------------------------------------------
  Обработчик прерывания от модуля SPI с номером modn
-------------------------------------------------------------------------------------------------------------*/
static void PIT_isr(uint8_t chnl)
{
  PIT_MemMapPtr PIT = PIT_BASE_PTR;
 
  PIT->CHANNEL[chnl].TFLG = 1; // Сбрасываем прерывание
  
  if (PIT_callbacks[chnl] != 0) PIT_callbacks[chnl](); // Вызываем callback функцию еслиона зарегистрирована

}
/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
static void PIT0_isr(void *user_isr_ptr)
{
  PIT_isr(0);
}
/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
static void PIT1_isr(void *user_isr_ptr)
{
  PIT_isr(1);
}
/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
static void PIT2_isr(void *user_isr_ptr)
{
  PIT_isr(2);
}
/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
static void PIT3_isr(void *user_isr_ptr)
{
  PIT_isr(3);
}



/*-------------------------------------------------------------------------------------------------------------
  Инициализация модуля Periodic Interrupt Timer (PIT)
  Один из каналов PIT используется как источник сигналов триггера для запука ADC

  Тактируется модуль от Bus Clock -> CPU_BUS_CLK_HZ = 60 МГц
-------------------------------------------------------------------------------------------------------------*/
void PIT_init_module(void)
{
  SIM_MemMapPtr  SIM  = SIM_BASE_PTR;
  PIT_MemMapPtr  PIT = PIT_BASE_PTR;


  SIM->SCGC6 |= BIT(23); // PIT | PIT clock gate control | 1 Clock is enabled.
  PIT->MCR = 0
            + LSHIFT(0, 1) // MDIS | Module Disable  | 1 Clock for PIT Timers is disabled.
            + LSHIFT(1, 0) // FRZ  | Freeze          | 1 Timers are stopped in debug mode.
  ;
}

/*-------------------------------------------------------------------------------------------------------------
   Инициализация таймера 0 модуля PIT
   Используется как триггер ADC

   period  - период подачи сигнала в мкс
   scan_sz - Количество сканируемых ADC каналов  

   Тактируется модуль от Bus Clock -> CPU_BUS_CLK_HZ = 60 МГц
 -------------------------------------------------------------------------------------------------------------*/
void PIT_init_for_ADC_trig(uint8_t chnl, uint32_t period, uint32_t scan_sz)
{
  PIT_MemMapPtr PIT = PIT_BASE_PTR;
  if (chnl >= PIT_CHANNELS_NUM) return;

  // Timer Load Value Register
  PIT->CHANNEL[chnl].LDVAL =((CPU_BUS_CLK_HZ / 1000000ul) * period) / scan_sz; // Загружаем период таймера


//  PIT_callbacks[chnl] = 0;
//  Install_and_enable_isr(PIT_int_nums[chnl], 1, PIT_isrs[chnl]);

  PIT->CHANNEL[chnl].TFLG = 1; // Сбрасываем прерывание
  PIT->CHANNEL[chnl].TCTRL = 0
                         + LSHIFT(1, 1) // TIE | Timer Interrupt Enable Bit. | 0 Interrupt requests from Timer n are disabled.
                         + LSHIFT(1, 0) // TEN | Timer Enable Bit.           | 1 Timer n is active.
  ;
}




/*-----------------------------------------------------------------------------------------------------
  Инициализация канала PIT с формированием прерываний и вызовом callback функции  
  
  \param channel  
  \param period  
  \param callback_func  
-----------------------------------------------------------------------------------------------------*/
void PIT_init_interrupt(uint8_t chnl, uint32_t period, uint8_t prio, T_PIT_callback callback_func)
{
  PIT_MemMapPtr PIT = PIT_BASE_PTR;
  if (chnl >= PIT_CHANNELS_NUM) return;

  // Timer Load Value Register
  PIT->CHANNEL[chnl].LDVAL =((CPU_BUS_CLK_HZ / 1000000ul) * period); // Загружаем период таймера

  PIT_callbacks[chnl] = callback_func;
  Install_and_enable_isr(PIT_int_nums[chnl], prio, PIT_isrs[chnl]);

  PIT->CHANNEL[chnl].TCTRL = 0
                            + LSHIFT(1, 1) // TIE | Timer Interrupt Enable Bit. | 0 Interrupt requests from Timer n are disabled.
                            + LSHIFT(1, 0) // TEN | Timer Enable Bit.           | 1 Timer n is active.
  ;
}

/*-----------------------------------------------------------------------------------------------------
  
  
  \param chnl  
-----------------------------------------------------------------------------------------------------*/
void PIT_disable(uint8_t chnl)
{
  PIT_MemMapPtr PIT = PIT_BASE_PTR;
  if (chnl >= PIT_CHANNELS_NUM) return;
  PIT->CHANNEL[chnl].TCTRL = 0
                            + LSHIFT(0, 1) // TIE | Timer Interrupt Enable Bit. | 0 Interrupt requests from Timer n are disabled.
                            + LSHIFT(0, 0) // TEN | Timer Enable Bit.           | 1 Timer n is active.
  ;
  PIT_callbacks[chnl] = 0;
}

/*-------------------------------------------------------------------------------------------------------------
   Получить текущее значение таймера. Таймер ведет счет до 0 и обносляет свое занчение величиной из PIT->CHANNEL[0].LDVAL
 -------------------------------------------------------------------------------------------------------------*/
uint32_t PIT_get_curr_val(uint8_t chnl)
{
  PIT_MemMapPtr PIT = PIT_BASE_PTR;
  if (chnl >= PIT_CHANNELS_NUM) return 0;
  return PIT->CHANNEL[chnl].CVAL;
}

/*-------------------------------------------------------------------------------------------------------------
   Получить загрузочное  значение таймера.
 -------------------------------------------------------------------------------------------------------------*/
uint32_t PIT_get_load_val(uint8_t chnl)
{
  PIT_MemMapPtr PIT = PIT_BASE_PTR;
  if (chnl >= PIT_CHANNELS_NUM) return 0;
  return PIT->CHANNEL[chnl].LDVAL;
}


