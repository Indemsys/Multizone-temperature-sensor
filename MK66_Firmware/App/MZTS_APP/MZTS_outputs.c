// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.07.08
// 14:10:37
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"


#define __ON   1
#define _OFF   0
//  Шаблон состоит из массива груп слов.
//  Первое слово в группе - значение напряжения
//  Второе слово в группе - длительность интервала времени в мс или специальный маркер остановки(0xFFFFFFFF) или цикла(0x00000000)

const int32_t   OUT_BLINK[] =
{
  __ON, 100,
  _OFF, 200,
  0, 0
};

const int32_t   OUT_ON[] =
{
  __ON, 10,
  __ON, 0xFFFFFFFF
};

const int32_t   OUT_OFF[] =
{
  _OFF, 10,
  _OFF, 0xFFFFFFFF
};

const int32_t   OUT_UNDEF_BLINK[] =
{
  __ON, 30,
  _OFF, 900,
  0, 0
};

const int32_t   OUT_3_BLINK[] =
{
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 250,
  0, 0
};

const int32_t   OUT_CAN_ACTIVE_BLINK[] =
{
  __ON, 10,
  _OFF, 40,
  __ON, 10,
  _OFF, 40,
  __ON, 10,
  _OFF, 40,
  __ON, 10,
  _OFF, 40,
  __ON, 10,
  _OFF, 40,
  0, 0xFFFFFFFF
};


const int32_t   OUT_10_OFF_BLINK[] =
{
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 50,
  __ON, 50,
  _OFF, 0xFFFFFFFF
};

T_otputs_cfg PLATF_outputs[OUTPUTS_NUM] =
{
  { (uint32_t *)&(PTE_BASE_PTR->PSOR), (uint32_t *)&(PTE_BASE_PTR->PCOR), BIT(26)}, // OUT_1WLED        Сигнал светодиода считывателя 1WIRE. Зажигается 1
};

static uint32_t outp_sm_period = 1; // Период в мс вызова автомата состояний выходов
static T_outs_ptrn outs_cbl[OUTPUTS_NUM];


void    Set_LCD_RST            (int32_t v) { if (v) (PTC_BASE_PTR->PSOR = BIT(12));  else (PTC_BASE_PTR->PCOR = BIT(12));}
void    Set_LCD_DC             (int32_t v) { if (v) (PTC_BASE_PTR->PSOR = BIT(13));  else (PTC_BASE_PTR->PCOR = BIT(13));}
void    Set_LCD_BLK            (int32_t v) { if (v) (PTC_BASE_PTR->PSOR = BIT( 6));  else (PTC_BASE_PTR->PCOR = BIT( 6));}
void    Set_LCD_CS             (int32_t v) { if (v) (PTC_BASE_PTR->PSOR = BIT( 4));  else (PTC_BASE_PTR->PCOR = BIT( 4));}
void    Set_IMU_EN             (int32_t v) { if (v) (PTC_BASE_PTR->PSOR = BIT(18));  else (PTC_BASE_PTR->PCOR = BIT(18));}
void    Set_LSM_CS             (int32_t v) { if (v) (PTC_BASE_PTR->PSOR = BIT( 3));  else (PTC_BASE_PTR->PCOR = BIT( 3));}

/*-----------------------------------------------------------------------------------------------------


  \param out_num
-----------------------------------------------------------------------------------------------------*/
void Set_output_blink(uint32_t out_num)
{
  Outputs_set_pattern(OUT_BLINK, out_num);
}

void Set_output_on(uint32_t out_num)
{
  Outputs_set_pattern(OUT_ON, out_num);
}

void Set_output_off(uint32_t out_num)
{
  Outputs_set_pattern(OUT_OFF, out_num);
}

void Set_output_blink_undef(uint32_t out_num)
{
  Outputs_set_pattern(OUT_UNDEF_BLINK, out_num);
}

void Set_output_blink_3(uint32_t out_num)
{
  Outputs_set_pattern(OUT_3_BLINK, out_num);
}

void Set_output_intf_active_blink(uint32_t out_num)
{
  Outputs_set_pattern(OUT_CAN_ACTIVE_BLINK, out_num);
}

void Set_output_off_blink_10(uint32_t out_num)
{
  Outputs_set_pattern(OUT_10_OFF_BLINK, out_num);
}

/*------------------------------------------------------------------------------



 \param num - порядковый номер выхода (1..19)
 \param val - лог. уровень выхода
 ------------------------------------------------------------------------------*/
void Set_output_state(uint8_t num, uint8_t val)
{
  if (num >= OUTPUTS_NUM) return;

  if (val != 0)
  {
    *PLATF_outputs[num].reg_for_1 = PLATF_outputs[num].bit;
  }
  else
  {
    *PLATF_outputs[num].reg_for_0 = PLATF_outputs[num].bit;
  }
}


/*-----------------------------------------------------------------------------------------------------
  Инициализация шаблона для машины состояний выходного сигнала

  Шаблон состоит из массива груп слов.
  Первое слово в группе - значение сигнала
  Второе слово в группе - длительность интервала времени в  мс
    интервал равный 0x00000000 - означает возврат в начало шаблона
    интервал равный 0xFFFFFFFF - означает застывание состояния


  \param pttn    - указатель на запись шаблоне
  \param n       - номер сигнала
  \param period  - периодичность вызова машины состояний
-----------------------------------------------------------------------------------------------------*/
void Outputs_set_pattern(const int32_t *pttn, uint32_t n)
{

  if (n >= OUTPUTS_NUM) return;

  if ((pttn != 0) && (outs_cbl[n].pattern_start_ptr != (int32_t *)pttn))
  {
    outs_cbl[n].pattern_start_ptr = (int32_t *)pttn;
    outs_cbl[n].pttn_ptr = (int32_t *)pttn;
    Set_output_state(n,*outs_cbl[n].pttn_ptr);
    outs_cbl[n].pttn_ptr++;
    outs_cbl[n].counter =(*outs_cbl[n].pttn_ptr) / outp_sm_period;
    outs_cbl[n].pttn_ptr++;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Установить период в мс вызова автомата состояний выходов

  \param val
-----------------------------------------------------------------------------------------------------*/
void Outputs_set_period_ms(uint32_t val)
{
  outp_sm_period =val;
}



/*-----------------------------------------------------------------------------------------------------


  \param n
-----------------------------------------------------------------------------------------------------*/
void Outputs_clear_pattern(uint32_t n)
{
  if (n >= OUTPUTS_NUM) return;
  outs_cbl[n].pattern_start_ptr = 0;
  outs_cbl[n].pttn_ptr = 0;
}

/*-----------------------------------------------------------------------------------------------------
   Автомат состояний выходных сигналов


  \param period  - период вызова этой функции в мс
-----------------------------------------------------------------------------------------------------*/
void Outputs_state_automat(void)
{
  uint32_t        duration;
  uint32_t        output_state;
  uint32_t        n;


  for (n = 0; n < OUTPUTS_NUM; n++)
  {
    // Управление состоянием выходного сигнала
    if (outs_cbl[n].counter) // Отрабатываем шаблон только если счетчик не нулевой
    {
      outs_cbl[n].counter--;
      if (outs_cbl[n].counter == 0)  // Меняем состояние сигнала при обнулении счетчика
      {
        if (outs_cbl[n].pattern_start_ptr != 0)  // Проверяем есть ли назначенный шаблон
        {
          output_state =*outs_cbl[n].pttn_ptr;   // Выборка значения состояния выхода
          outs_cbl[n].pttn_ptr++;
          duration =*outs_cbl[n].pttn_ptr;       // Выборка длительности состояния
          outs_cbl[n].pttn_ptr++;                // Переход на следующий элемент шаблона
          if (duration != 0xFFFFFFFF)
          {
            if (duration == 0)  // Длительность равная 0 означает возврат указателя элемента на начало шаблона и повторную выборку
            {
              outs_cbl[n].pttn_ptr = outs_cbl[n].pattern_start_ptr;
              output_state =*outs_cbl[n].pttn_ptr;
              outs_cbl[n].pttn_ptr++;
              outs_cbl[n].counter =(*outs_cbl[n].pttn_ptr) / outp_sm_period;
              outs_cbl[n].pttn_ptr++;
              Set_output_state(n , output_state);
            }
            else
            {
              outs_cbl[n].counter = duration / outp_sm_period;
              Set_output_state(n ,output_state);
            }
          }
          else
          {
            // Обнуляем счетчик и таким образом выключаем обработку паттерна
            Set_output_state(n , output_state);
            outs_cbl[n].counter = 0;
            outs_cbl[n].pattern_start_ptr = 0;
          }
        }
        else
        {
          // Если нет шаблона обнуляем состояние выходного сигнала
          Set_output_state(n, 0);
        }
      }
    }

  }
}



