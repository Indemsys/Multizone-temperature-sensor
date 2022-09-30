#include "App.h"

HWTIMER                    hwtimer1;
unsigned int               tmodulo;
unsigned int               tperiod;
#define HWTIMER1_FREQUENCY 1

uint8_t  g_RTC_state; // Состояние часов реального времени в момент старта. 1 - означает, что часы включены. 0 - означает, что часы остановлены

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void Init_RTC(void)
{
  uint32_t        rtc_time;
  TIME_STRUCT     mqx_time;
  RTC_MemMapPtr   rtc    = RTC_BASE_PTR;

  g_RTC_state = 1;

  SIM_SCGC6 |= SIM_SCGC6_RTC_MASK; // Включаем тактирование для доступа к регистрам RTC

  _int_disable();

  if (rtc->SR & RTC_SR_TIF_MASK)
  {
    // Здесь если время в RTC было потеряно
    // Флаг RTC_SR_TIF_MASK сбрасывается записью в регистр TSR какого либо значения
    g_RTC_state = 0;


    rtc->SR &= ~RTC_SR_TCE_MASK;    // Сбрасываем флаг TCE (Time Counter Enable) чтобы иметь возможность записать в регистр секунд и предделителя
    rtc->TAR = 0xFFFFFFFF;          // e2574: RTC: Writing RTC_TAR[TAR] = 0 does not disable RTC alarm

    rtc->TSR = 1;                   // Записываем регистр секунд. Заодно сбрасывается бит TIF в регистре SR
    rtc->SR |= RTC_SR_TCE_MASK;     // Разрешаем работу RTC

  }
  _int_enable();


  if (!(rtc->SR & RTC_SR_TCE_MASK) || !(rtc->CR & RTC_CR_OSCE_MASK))
  {
    // Здесь если счет RTC не был разрешен или был отключен осциллятор
    g_RTC_state = 0;
    rtc->CR |= RTC_CR_OSCE_MASK;
    _time_delay(125);             // Даем паузу для старта осциллятора
    rtc->SR |= RTC_SR_TCE_MASK;
  }


  _rtc_get_time(&rtc_time);
  mqx_time.SECONDS = rtc_time;
  mqx_time.MILLISECONDS = 0;
  _time_set(&mqx_time); // Установка времени RTOS

}


/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
_mqx_int TimeManInit(void)
{
  if (MQX_OK != hwtimer_init(&hwtimer1,&BSP_HWTIMER1_DEV, BSP_HWTIMER1_ID,(BSP_DEFAULT_MQX_HARDWARE_INTERRUPT_LEVEL_MAX + 1)))
  {
    return MQX_ERROR;
  }
  hwtimer_set_freq(&hwtimer1, BSP_HWTIMER1_SOURCE_CLK, HWTIMER1_FREQUENCY);

  tmodulo = hwtimer_get_modulo(&hwtimer1);
  tperiod = hwtimer_get_period(&hwtimer1);

  hwtimer_start(&hwtimer1);

  return MQX_OK;
}

/*-------------------------------------------------------------------------------------------------------------
  Получить значение времени
-------------------------------------------------------------------------------------------------------------*/
void Get_time_counters(HWTIMER_TIME_STRUCT *t)
{
  hwtimer_get_time(&hwtimer1, t);
}

/*-------------------------------------------------------------------------------------------------------------
  Вычисление разницы во времени в мкс
-------------------------------------------------------------------------------------------------------------*/
uint32_t Eval_meas_time(HWTIMER_TIME_STRUCT t1, HWTIMER_TIME_STRUCT t2)
{
  unsigned int t;
  unsigned long long  tt;

  t2.TICKS = t2.TICKS - t1.TICKS;
  if (t2.SUBTICKS < t1.SUBTICKS)
  {
    t2.TICKS = t2.TICKS - 1;
    t2.SUBTICKS = t1.SUBTICKS - t2.SUBTICKS;
  }
  else
  {
    t2.SUBTICKS = t2.SUBTICKS - t1.SUBTICKS;
  }
  t = t2.TICKS * tperiod;
  tt = (unsigned long long)t2.SUBTICKS * (unsigned long long)tperiod;
  t = t + tt / tmodulo;
  return t;
}

/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
uint32_t  Get_usage_time(void)
{
  HWTIMER_TIME_STRUCT t1,t2;

  Get_time_counters(&t1);
  DELAY_ms(100);
  //us_Delay(2079999u);
  Get_time_counters(&t2);

  return Eval_meas_time(t1,t2);
}


