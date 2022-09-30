// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2015.11.03
// 11:20:39
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

#define CPU_USAGE_FLTR_LEN 128
int32_t cpu_usage_arr[CPU_USAGE_FLTR_LEN];

/*------------------------------------------------------------------------------
 Создаем мбютекс с определенными аттрибутами: MUTEX_PRIO_INHERIT, MUTEX_PRIORITY_QUEUEING

 \param pmutex

 \return _mqx_uint
 ------------------------------------------------------------------------------*/
_mqx_uint Create_mutex_P_inhr_P_queue(MUTEX_STRUCT_PTR  pmutex)
{
  _mqx_uint res;
  MUTEX_ATTR_STRUCT   mutexattr;

  // Конфигурируем аттрибуты мьютекса
  res = _mutatr_init(&mutexattr);
  if (res != MQX_OK) return res;

  _mutatr_set_sched_protocol(&mutexattr, MUTEX_PRIO_INHERIT);
  _mutatr_set_wait_protocol(&mutexattr, MUTEX_PRIORITY_QUEUEING);

  res = _mutex_init(pmutex,&mutexattr);
  return res;
}


/*-----------------------------------------------------------------------------------------------------
  Измеряем длительность интервала времени ti заданного в милисекундах
-----------------------------------------------------------------------------------------------------*/
uint32_t Measure_reference_time_interval(uint32_t ti)
{
  MQX_TICK_STRUCT tickt_prev;
  MQX_TICK_STRUCT tickt;
  bool            overfl;

  _time_get_ticks(&tickt_prev);
  DELAY_ms(ti);
  _time_get_ticks(&tickt);

  return _time_diff_microseconds(&tickt,&tickt_prev,&overfl);
}

/*-------------------------------------------------------------------------------------------------------------
  Фоновая задача.
  Измеряет загруженность процессора

-------------------------------------------------------------------------------------------------------------*/
void Task_background(unsigned int initial_data)
{
  uint32_t t, dt;

  cpu_usage  = 1000;
  g_aver_cpu_usage = 1000;
  ioctl(g_filesystem_handle,IO_IOCTL_FREE_SPACE,&g_fs_free_space);
  g_fs_free_space_val_ready = 1;

  filter_cpu_usage.len = CPU_USAGE_FLTR_LEN;
  filter_cpu_usage.en  = 0;
  filter_cpu_usage.arr = cpu_usage_arr;
  g_aver_cpu_usage = RunAverageFilter_int32_N(cpu_usage,&filter_cpu_usage);

  for (;;)
  {
    t = Measure_reference_time_interval(REF_TIME_INTERVAL);

    if (t < ref_time)
    {
      dt = 0;
    }
    else
    {
      dt = t - ref_time;
    }
    cpu_usage =(1000ul * dt) / ref_time;
    g_aver_cpu_usage = RunAverageFilter_int32_N(cpu_usage,&filter_cpu_usage);
  }

}


/*-----------------------------------------------------------------------------------------------------
  Получаем  оценку калибровочного интервала времени предназначенного для измерения загрузки процессора

  Проводим несколько измерений и выбираем минимальный интервал
-----------------------------------------------------------------------------------------------------*/
void Get_reference_time(void)
{
  uint32_t i;
  uint32_t t;
  uint32_t tt = 0xFFFFFFFF;

  for (i = 0; i < 10; i++)
  {
    t = Measure_reference_time_interval(REF_TIME_INTERVAL);
    if (t < tt) tt = t;
  }
  ref_time = tt;
}


/*-------------------------------------------------------------------------------------------------------------
  Установки и инициализация обычной процедуры прерывания RTOS

  pri - значение приоритета. Чем меньше значение тем выше приоритет.

  Инициализируемые здесь прерывания вызываются через механизм RTOS
  поэтому значение приоритета должно быть строго в рамках от MQX_HARDWARE_INTERRUPT_LEVEL_MAX до 7 включительно.

  Попытка установить приоритету меньшее значение приведет к зависанию RTOS!!!
-------------------------------------------------------------------------------------------------------------*/
void Install_and_enable_isr(int num, int pri, INT_ISR_FPTR isr_ptr)
{
  _int_install_isr(num, isr_ptr, 0);
  _bsp_int_init(num, pri, 0, TRUE);
}

/*-------------------------------------------------------------------------------------------------------------
  Установки и инициализация процедуры прерывания ядра RTOS

  pri - значение приоритета. Чем меньше значение тем выше приоритет.

  Инициализируемые здесь прерывания вызываются  без использования механизмов RTOS
  поэтому значение приоритета от 0 до 7 включительно.

  Установка приоритета равного или больше MQX_HARDWARE_INTERRUPT_LEVEL_MAX приведет к тому чтопрерывание может блокироваться задачами RTOS
-------------------------------------------------------------------------------------------------------------*/
INT_KERNEL_ISR_FPTR Install_and_enable_kernel_isr(int num, int pri, INT_KERNEL_ISR_FPTR isr_ptr)
{
  INT_KERNEL_ISR_FPTR p;
  p = _int_install_kernel_isr(num, isr_ptr);
  _bsp_int_init(num, pri, 0, TRUE);
  return p;
}

/*-----------------------------------------------------------------------------------------------------
  Записать значение защищенной знаковой переменной
-----------------------------------------------------------------------------------------------------*/
void Set_shar_s32val(int32_t *adr, int32_t v)
{
  _int_disable();
  *adr = v;
  _int_enable();
}

/*-----------------------------------------------------------------------------------------------------
  Записать значение защищенной беззнаковой переменной
-----------------------------------------------------------------------------------------------------*/
void Set_shar_u32val(uint32_t *adr, uint32_t v)
{
  _int_disable();
  *adr = v;
  _int_enable();
}

/*-----------------------------------------------------------------------------------------------------
  Записать значение защищенной области данных
-----------------------------------------------------------------------------------------------------*/
void Set_shar_buff(void *dest, void *src, uint32_t sz)
{
  _int_disable();
  memcpy(dest, src, sz);
  _int_enable();
}

/*-----------------------------------------------------------------------------------------------------
  Получить значение защищенной беззнаковой переменной
-----------------------------------------------------------------------------------------------------*/
uint32_t Get_shar_u32val(uint32_t *adr)
{
  uint32_t v;
  _int_disable();
  v =*adr;
  _int_enable();
  return v;
}

/*-----------------------------------------------------------------------------------------------------
  Получить значение защищенной знаковой переменной
-----------------------------------------------------------------------------------------------------*/
int32_t Get_shar_s32val(int32_t *adr)
{
  int32_t v;
  _int_disable();
  v =*adr;
  _int_enable();
  return v;
}

/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
uint32_t Time_elapsed_ms(TIME_STRUCT *tlast_ptr)
{
  TIME_STRUCT tdiff;
  TIME_STRUCT tnow_ptr;
  _time_get(&tnow_ptr);
  _time_diff(tlast_ptr,&tnow_ptr,&tdiff);
  return (tdiff.SECONDS * 1000 + tdiff.MILLISECONDS);
}
/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
uint32_t Time_elapsed_sec(TIME_STRUCT *tlast_ptr)
{
  TIME_STRUCT tdiff;
  TIME_STRUCT tnow;
  _time_get(&tnow);
  _time_diff(tlast_ptr,&tnow,&tdiff);
  return tdiff.SECONDS;
}

/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
uint32_t Time_diff_ms(TIME_STRUCT *tlast_ptr, TIME_STRUCT *tnow_ptr)
{
  TIME_STRUCT tdiff;
  _time_diff(tlast_ptr, tnow_ptr,&tdiff);
  return (tdiff.SECONDS * 1000 + tdiff.MILLISECONDS);
}

/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
uint32_t Time_diff_sec(TIME_STRUCT *tlast_ptr, TIME_STRUCT *current_time)
{
  TIME_STRUCT tdiff;
  _time_diff(tlast_ptr, current_time,&tdiff);
  return tdiff.SECONDS;
}


/*------------------------------------------------------------------------------
 Конвертация времени выраженного в мс в тики RTOS
 Количество тиков не может быть меньше 1 с

 \param ms

 \return uint32_t
 ------------------------------------------------------------------------------*/
uint32_t ms_to_ticks(uint32_t ms)
{
  uint32_t ticks;
  ticks =(_time_get_ticks_per_sec() * ms) / 1000;
  if (ticks == 0)
  {
    ticks = 1;
  }
  return ticks;
}

/*-----------------------------------------------------------------------------------------------------


  \param tms
-----------------------------------------------------------------------------------------------------*/
void Wait_ms(uint32_t tms)
{
  _time_delay(tms);
}


/*------------------------------------------------------------------------------
 Сброс системы
 ------------------------------------------------------------------------------*/
void Reset_system(void)
{
  _int_disable();
  // System Control Block -> Application Interrupt and Reset Control Register
  SCB_AIRCR = 0x05FA0000 // Обязательный шаблон при записи в этот регистр
             | BIT(2);  // Установка бита SYSRESETREQ
  for (;;)
  {
    __no_operation();
  }
}


