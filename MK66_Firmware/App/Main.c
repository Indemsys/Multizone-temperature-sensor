#define _MAIN_GLOBALS_
#include "App.h"


WVAR_TYPE                  wvar;
uint32_t                   ref_time;             // Калибровочная константа предназначенная для измерения нагрузки микропроцессора
volatile uint32_t          cpu_usage;            // Процент загрузки процессора
T_run_average_int32_N      filter_cpu_usage;
volatile uint32_t          g_aver_cpu_usage;
uint32_t                   g_fmstr_rate_src;     // Переменная управляющая источником сэмплов для рекордера FreeMaster
float                      g_fmstr_smpls_period; // Период сэмплирования рекордера FreeMaster в секундах
uint8_t                    g_sd_card_status;
uint64_t                   g_fs_free_space;
uint8_t                    g_fs_free_space_val_ready;

void Main_task(unsigned int initial_data);



const TASK_TEMPLATE_STRUCT  MQX_template_list[] =
{
  /* Task Index,     Function,                Stack,  Priority,            Name,        Attributes,                                                          Param, Time Slice */
  { MAIN_TASK_IDX,      Main_task,            2048,   MAIN_TASK_PRIO,     "Main",       MQX_FLOATING_POINT_TASK + MQX_AUTO_START_TASK + MQX_TIME_SLICE_TASK, 0,     2 },
  { VT100_UART_IDX,     Task_VT100,           1500,   VT100_UART_ID_PRIO, "VT100_UART", MQX_FLOATING_POINT_TASK + MQX_TIME_SLICE_TASK,                       0,     2 },
  { VT100_USB_IDX,      Task_VT100,           1500,   VT100_USB_ID_PRIO,  "VT100_USB",  MQX_FLOATING_POINT_TASK + MQX_TIME_SLICE_TASK,                       0,     2 },
  { VT100_BLE_IDX,      Task_VT100,           1500,   VT100_BLE_ID_PRIO,  "VT100_BLE",  MQX_FLOATING_POINT_TASK + MQX_TIME_SLICE_TASK,                       0,     2 },
  { CAN_TX_IDX,         Task_CAN_Tx,          500,    CAN_TX_ID_PRIO,     "CAN_TX",     MQX_FLOATING_POINT_TASK + 0,                                         0,     0 },
  { CAN_RX_IDX,         Task_CAN_Rx,          2048,   CAN_RX_ID_PRIO,     "CAN_RX",     MQX_FLOATING_POINT_TASK + 0,                                         0,     0 },
  { MKW40_IDX,          Task_MKW40,           2048,   MKW_ID_PRIO,        "MKW40",      MQX_FLOATING_POINT_TASK + MQX_TIME_SLICE_TASK,                       0,     2 },
  { FILELOG_IDX,        Task_file_log,        1500,   FILELOG_ID_PRIO,    "FileLog",    MQX_FLOATING_POINT_TASK + MQX_TIME_SLICE_TASK,                       0,     2 },
  { FREEMASTER_IDX,     Task_FreeMaster,      750,    FREEMASTER_ID_PRIO, "FreeMaster", MQX_FLOATING_POINT_TASK + MQX_TIME_SLICE_TASK,                       0,     2 },
  { SUPERVISOR_IDX,     Task_supervisor,      500,    SUPRVIS_ID_PRIO,    "SUPRVIS",    MQX_FLOATING_POINT_TASK + MQX_TIME_SLICE_TASK,                       0,     2 },
  { BACKGR_IDX,         Task_background,      1000,   BACKGR_ID_PRIO,     "BACKGR",     MQX_FLOATING_POINT_TASK,                                             0,     0 },
  { 0 }
};

_task_id terminal_task_id;

volatile uint32_t tdelay, tmin, tmax;

/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
void Main_task(unsigned int initial_data)
{
  // Устанавливаем время операционной системы
  Init_RTC();


  Set_LED_pattern(LED_BLINK, 0);

  AppLogg_init();
  Write_start_log_rec();
  Get_reference_time();

  Inputs_create_sync_obj(); // Создаем объект синхронизации задачи обслуживания входов до того как запустили измерения
  PIT_init_module();
  ADCs_subsystem_init();    // Инициализируем работу многоканального АЦП




#ifdef MQX_MFS
  if (Init_mfs() == MQX_OK)
  {
    _task_create(0, FILELOG_IDX, 0);
  }
#endif

  Restore_parameters_and_settings();


  _task_create(0, BACKGR_IDX, 0);     // Фоновая задача. Измеряет загрузку процессора
  _task_create(0, SUPERVISOR_IDX, 0); // Задача сброса watchdog и наблюдения за работоспособностью системы
  WatchDog_init(WATCHDOG_TIMEOUT, WATCHDOG_WIN);   // Инициализируем Watchdog в оконном режиме

  _task_create(0, INPUTS_IDX, 0);     //

  CAN_init(CAN0_BASE_PTR, CAN_SPEED);

  _task_create(0, CAN_RX_IDX, (uint32_t)CAN_app_task_rx);
  _task_create(0, CAN_TX_IDX, (uint32_t)CAN_app_task_tx);


   _task_create(0, MKW40_IDX, 0);

#ifdef USB_2VCOM
  Composite_USB_device_init();
#elif USB_1VCOM
  Init_USB();
#endif

#ifdef USB_VT100
  terminal_task_id = _task_create(0, VT100_USB_IDX, (uint32_t)Mnudrv_get_USB_vcom_driver1());       // Создаем задачу VT100_task с дайвером  интерфейса и делаем ее активной
#else
  terminal_task_id = _task_create(0, VT100_UART_IDX, (uint32_t)Mnsdrv_get_ser_std_driver());        // Создаем задачу VT100_task с дайвером  интерфейса и делаем ее активной
#endif

  _task_create(0, FREEMASTER_IDX, (uint32_t)Mnudrv_get_USB_vcom_driver2());                         // Создаем задачу FreeMaster с дайвером  интерфейса и делаем ее активной

  MZTS_task();

}



