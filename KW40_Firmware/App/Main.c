// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2017.05.12
// 16:11:37
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"


uint8_t  gUseRtos_c         = 1;   // Переменная требуемая функцией l2ca_main в библиотеке ble_host_lib.a. Не переименовывать!
uint32_t gNvmStartAddress_c;       // Переменная требуемая модулем device_db в библиотеке ble_host_lib.a. Не переименовывать!
extern uint32_t      NV_STORAGE_END_ADDRESS[]; // Задается в файле линкера
extern uint8_t       gBDAddress_c[6];          // переменная требуемая модулем platform_board_init.o в библиотеке ble_controller_lib.a. Не переименовывать!




static void        BLE_SignalFromISRCallback(void);
static osaStatus_t AppIdle_TaskInit(void);
static void        App_Idle_Task(task_param_t argument);
static void        Main_task(uint32_t param);
void               Get_MAC_address();

event_t            app_event_struct;
anchor_t           main_task_queue;


/*-----------------------------------------------------------------------------------------------------
 
 \param void 
 
 \return int 
-----------------------------------------------------------------------------------------------------*/
int main(void)
{
  task_handler_t handler;

  // Эти константы должны задаваться до инициализации RTOS поскольку они там используются
  SystemCoreClock = CORE_CLOCK_FREQ;
  g_xtal0ClkFreq  = 32000000U;

  OSA_Init();

  OSA_TaskCreate((task_t)Main_task,
                 "main",
                 MAIN_TASK_STACK_SZ,
                 NULL,
                 MAIN_TASK_PRIO,
                 0,
                 FALSE,
                 &handler);

  OSA_Start();

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
 
 \return int32_t 
-----------------------------------------------------------------------------------------------------*/
static int32_t Send_ACK(uint8_t *dbuf, uint32_t sz)
{
  return Push_message_to_tx_queue(MKW40_CH_ACK, dbuf, sz);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
 
 \return int32_t 
-----------------------------------------------------------------------------------------------------*/
static int32_t Send_NACK(void)
{
  return Push_message_to_tx_queue(MKW40_CH_NACK, 0, 0);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param dbuf 
 
 \return int32_t 
-----------------------------------------------------------------------------------------------------*/
int32_t App_accept_parameter(uint8_t *dbuf)
{
  uint32_t par_num;
  uint32_t par_sz;

  par_num = (*dbuf >> BLE_PARAM_POS) & BLE_PARAM_MASK;
  par_sz  = *dbuf & BLE_PARAM_SZ_MASK;

  dbuf++;
  switch (par_num)
  {
  case PAR_PIN_CODE:
    if (par_sz == 4 )
    {
      memcpy(&pin_code, dbuf, 4 ); 
      Send_ACK(dbuf,par_sz);
    }
    break;
  case PAR_ADV_DEV_NAME :
    if (par_sz <= MAX_BLE_STR_SZ )
    {
      memcpy(advertised_device_name, dbuf, par_sz ); 
      advertised_device_name[par_sz] = 0;
      Send_ACK(dbuf,par_sz);
    }
    break;
  case PAR_SOFT_REV :
    if (par_sz <= MAX_BLE_STR_SZ )
    {
      memcpy(software_revision, dbuf, par_sz ); 
      software_revision[par_sz] = 0;
      Send_ACK(dbuf,par_sz);
    }
    break;
  default:
    Send_NACK();
    break;    
  }

  return 0;
}

/*-----------------------------------------------------------------------------------------------------
 
 \param  
-----------------------------------------------------------------------------------------------------*/
void Get_MAC_address()
{
  // Инициализируем публичный идентификатор BLE
  gBDAddress_c[0] = (SIM->UIDMH  >> 8) & 0xFF;     // ADDR_B1;
  gBDAddress_c[1] = (SIM->UIDMH  >> 0) & 0xFF;     // ADDR_B2;
  gBDAddress_c[2] = (SIM->UIDML  >> 24) & 0xFF;    // ADDR_B3;
  gBDAddress_c[3] = (SIM->UIDML  >> 16) & 0xFF;    // ADDR_B4;
  gBDAddress_c[4] = (SIM->UIDML  >> 8) & 0xFF;     // ADDR_B5;
  gBDAddress_c[5] = (SIM->UIDML  >> 0) & 0xFF;     // ADDR_B6;
  DEBUG_PRINT_ARG("Address = %02X%02X%02X%02X%02X%02X\r\n", gBDAddress_c[0], gBDAddress_c[1], gBDAddress_c[2], gBDAddress_c[3], gBDAddress_c[4], gBDAddress_c[5]);

}

/*-----------------------------------------------------------------------------------------------------
 
 \param param 
-----------------------------------------------------------------------------------------------------*/
void Main_task(uint32_t param)
{
  event_flags_t event;

  osa_status_t  status;
  uint8_t       pseudoRNGSeed[20] = { 0 };



  NV_ReadHWParameters(&gHardwareParameters);

  SEGGER_RTT_Init();
  DEBUG_PRINT("\r\nMain task stared. Compiled:"__TIMESTAMP__"\r\n");

  /* Framework init */
  MEM_Init();
  TMR_Init();        // Инициализация таймера TPM0. Таймер после предделителя тактируется частотой 250000 Гц. Прерывания таймера устанавливаются по срабатыванию компаратора в канале 0
  SecLib_Init();

  BSP_DMA_init();
  BSP_Init_PIT();    // PIT используем для измерения интервалов времени
  BSP_Init_SPI1();   // Иниициализация SPI в режиме мастера для связи в внешним микроконтроллером


  RNG_Init();
  RNG_GetRandomNo((uint32_t *)(&(pseudoRNGSeed[0])));
  RNG_GetRandomNo((uint32_t *)(&(pseudoRNGSeed[4])));
  RNG_GetRandomNo((uint32_t *)(&(pseudoRNGSeed[8])));
  RNG_GetRandomNo((uint32_t *)(&(pseudoRNGSeed[12])));
  RNG_GetRandomNo((uint32_t *)(&(pseudoRNGSeed[16])));
  RNG_SetPseudoRandomNoSeed(pseudoRNGSeed);

  NV_Init();  // Initialize NV module
  gNvmStartAddress_c = (uint32_t)((uint8_t *)NV_STORAGE_END_ADDRESS);     // NV_STORAGE_END_ADDRESS from linker file is used as NV Start Address


  pfBLE_SignalFromISR = BLE_SignalFromISRCallback;

  #if USE_POWER_DOWN_MODE == TRUE
  AppIdle_TaskInit();
  PWR_Init();
  PWR_DisallowDeviceToSleep();
  DEBUG_PRINT("Used Power Down Mode.\r\n");
  #else
  DEBUG_PRINT("Do not used Power Down Mode.\r\n");
  #endif


  Init_master_channel_task();                          // Стартуем задачу канала связи с мастер контроллером

  OSA_EventCreate(&app_event_struct, kEventAutoClear); // Создаем объект для передачи событий из задач BLE стека в задачу main
  MSG_InitQueue(&main_task_queue);                     // Создаем объект для передачи сообщений из задач BLE стека в задачу main


  // Здесь ожидаем инициализации со стороны мастер контроллера

  strcpy(advertised_device_name,"MZTS"); // Длина ограничена 8-ю символами для этого поля  
  pin_code = 000000;
  strcpy(software_revision,"2022.09.29.1");

  OSA_EventWait(&app_event_struct, EVENT_SETTINGS_DONE, FALSE, OSA_WAIT_FOREVER, &event);

  BleApp_setup_names();


  Get_MAC_address();  // Задать уникальный MAC адрес устройству
  XcvrInit(BLE);      // BLE Radio Init



  if (Ble_Initialize(App_GenericCallback) != gBleSuccess_c)  // Создаем и запускаем задачи BLE хоста, BLE контроллера
  {
    Panic(0, 0, 0, 0);
    return;
  }



  DEBUG_PRINT("Start advertising\r\n");

  BleApp_Restart_Advertising();


  // Бесконечный цикл обработки сообщений
  while (1)
  {
    OSA_EventWait(&app_event_struct, 0x00FFFFFF, FALSE, OSA_WAIT_FOREVER, &event);
    if (event & EVENT_FROM_HOST_STACK)
    {
      appMsgFromHost_t *msg = NULL;

      while (MSG_Pending(&main_task_queue))
      {
        msg = MSG_DeQueue(&main_task_queue);

        if (msg)
        {
          DEBUG_PRINT("->>>> App message handle begin.\r\n");
          App_HandleHostMessageInput(msg);
          DEBUG_PRINT(".....\r\n");
          MSG_Free(msg);
          msg = NULL;
          DEBUG_PRINT("##### App message handle end.\r\n");
          //OSA_TimeDelay(2); // Задержка чтобы канал RTT успевал передавать сообщения
        }
      }
    }
    else
    {
      DEBUG_PRINT("Unrecognized main task message!\r\n");
    }
  }
}

/*------------------------------------------------------------------------------

 Called by BLE when a connect is received

------------------------------------------------------------------------------*/
static void BLE_SignalFromISRCallback(void)
{
  #if (USE_POWER_DOWN_MODE)
  PWR_DisallowDeviceToSleep();
  #endif
}



#if (USE_POWER_DOWN_MODE)
/*------------------------------------------------------------------------------



 \return osaStatus_t
 ------------------------------------------------------------------------------*/
static osaStatus_t AppIdle_TaskInit(void)
{
  osa_status_t status;

  if (gAppIdleTaskId)
  {
    return osaStatus_Error;
  }

  /* Task creation */
  status = OSA_TaskCreate(App_Idle_Task, "IDLE_Task", IDLE_TASK_STACK_SZ, IDLE_stack, IDLE_TASK_PRIO, (task_param_t)NULL, FALSE, &gAppIdleTaskId);

  if (kStatus_OSA_Success != status)
  {
    Panic(0, 0, 0, 0);
    return osaStatus_Error;
  }

  return osaStatus_Success;
}

/*------------------------------------------------------------------------------



 \param argument
 ------------------------------------------------------------------------------*/
static void App_Idle_Task(task_param_t argument)
{
  PWRLib_WakeupReason_t wakeupReason;
  volatile uint32_t     regPCRB2;

  while (1)
  {
    if (PWR_CheckIfDeviceCanGoToSleep())
    {
      /* Enter Low Power */
      wakeupReason = PWR_EnterLowPower();

      #if gFSCI_IncludeLpmCommands_c
      /* Send Wake Up indication to FSCI */
      FSCI_SendWakeUpIndication();
      #endif

//      Вызывается если переход в сон был вызван кнопкой
//      KBD_SwitchPressedOnWakeUp();
//      PWR_DisallowDeviceToSleep();
    }

  }
}
#endif


/*------------------------------------------------------------------------------

 Вызывается при установлении связывания

 \param nvmStartAddress
 \param nvmSize
 ------------------------------------------------------------------------------*/
void App_NvmErase(uint32_t nvmStartAddress, uint32_t nvmSize)
{
  (void)nvmSize;
  NV_FlashEraseSector(&gFlashConfig, nvmStartAddress, gcReservedFlashSizeForBondedDevicesData_c, gFlashLaunchCommand);
}

/*------------------------------------------------------------------------------

 Вызывается при установлении связывания

 \param nvmDestinationAddress
 \param pvRamSource
 \param cDataSize
 ------------------------------------------------------------------------------*/
void App_NvmWrite(uint32_t nvmDestinationAddress, void *pvRamSource, uint32_t cDataSize)
{
  NV_FlashProgramUnaligned(&gFlashConfig, nvmDestinationAddress, cDataSize, pvRamSource, gFlashLaunchCommand);
}

/*------------------------------------------------------------------------------

  Функция вызывается при старте 
  Читает с адреса 0x00026C00

 \param nvmSourceAddress
 \param pvRamDestination
 \param cDataSize
 ------------------------------------------------------------------------------*/
void App_NvmRead(uint32_t nvmSourceAddress, void *pvRamDestination, uint32_t cDataSize)
{
  NV_FlashRead(nvmSourceAddress, pvRamDestination, cDataSize);
}

/*-----------------------------------------------------------------------------------------------------
 Функция используется в модуле gap_api2msg.0 в библиотеке ble_host_lib.a 
 
 \param eventId 
 \param flagsToSet 
 
 \return osaStatus_t 
-----------------------------------------------------------------------------------------------------*/
osaStatus_t OSA_EXT_EventSet(osaEventId_t eventId, osaEventFlags_t flagsToSet)
{
  osa_status_t osa_status;
  T_event_obj  *p_event_obj;
  p_event_obj = (T_event_obj *)eventId;
  osa_status = OSA_EventSet(&p_event_obj->os_event, (event_flags_t)flagsToSet);
  return (osaStatus_t)osa_status;
}

/*-----------------------------------------------------------------------------------------------------
 Функция используется в модуле l2ca_main.0 в библиотеке ble_host_lib.a 

 Description   : This function checks the event's status, if it meets the wait
 condition, return osaStatus_Success, otherwise, timeout will be used for
 wait. The parameter timeout indicates how long should wait in milliseconds.
 Pass osaWaitForever_c to wait indefinitely, pass 0 will return the value
 osaStatus_Timeout immediately if wait condition is not met. The event flags
 will be cleared if the event is auto clear mode. Flags that wakeup waiting
 task could be obtained from the parameter setFlags.
 This function returns osaStatus_Success if wait condition is met, returns
 osaStatus_Timeout if wait condition is not met within the specified
 'timeout', returns osaStatus_Error if any errors occur during waiting.
 
 \param eventId 
 \param flagsToWait 
 \param waitAll 
 \param millisec 
 \param pSetFlags 
 
 \return osaStatus_t 
-----------------------------------------------------------------------------------------------------*/
osaStatus_t OSA_EXT_EventWait(osaEventId_t eventId, osaEventFlags_t flagsToWait, bool_t waitAll, uint32_t millisec, osaEventFlags_t *pSetFlags)
{
  osa_status_t osa_status;
  T_event_obj  *p_event_obj;

  /* Clean FreeRTOS cotrol flags */
  flagsToWait = flagsToWait & 0x00FFFFFF;

  p_event_obj = (T_event_obj *)eventId;
  osa_status = OSA_EventWait(&p_event_obj->os_event, (event_flags_t)flagsToWait, waitAll, millisec, (event_flags_t *)pSetFlags);
  return (osaStatus_t)osa_status;
}


/*-----------------------------------------------------------------------------------------------------
 Функция используется в модуле hci_commands.0 в библиотеке ble_host_lib.a 
 
 Description   : This function checks the semaphore's counting value, if it is
 positive, decreases it and returns osaStatus_Success, otherwise, timeout
 will be used for wait. The parameter timeout indicates how long should wait
 in milliseconds. Pass osaWaitForever_c to wait indefinitely, pass 0 will
 return osaStatus_Timeout immediately if semaphore is not positive.
 This function returns osaStatus_Success if the semaphore is received, returns
 osaStatus_Timeout if the semaphore is not received within the specified
 'timeout', returns osaStatus_Error if any errors occur during waiting.
 
 \param semId 
 \param millisec 
 
 \return osaStatus_t 
-----------------------------------------------------------------------------------------------------*/
osaStatus_t OSA_EXT_SemaphoreWait(osaSemaphoreId_t semId, uint32_t millisec)
{
  osa_status_t osa_status;
  semaphore_t  sem        = (semaphore_t)semId;

  osa_status = OSA_SemaWait(&sem, millisec);
  if (osa_status != kStatus_OSA_Success)
  {
    __no_operation(); // Отладочная вставка
  }
  return  (osaStatus_t)osa_status;
}



/*-----------------------------------------------------------------------------------------------------
 Функция используется в модуле hci_commands.0 в библиотеке ble_host_lib.a 
 
 Description   : This function is used to wake up one task that wating on the
 semaphore. If no task is waiting, increase the semaphore. The function returns
 osaStatus_Success if the semaphre is post successfully, otherwise returns
 osaStatus_Error.
 
 
 \param semId 
 
 \return osaStatus_t 
-----------------------------------------------------------------------------------------------------*/
osaStatus_t OSA_EXT_SemaphorePost(osaSemaphoreId_t semId)
{
  osa_status_t osa_status;
  semaphore_t  sem        = (semaphore_t)semId;

  osa_status = OSA_SemaPost(&sem);
  if (osa_status != kStatus_OSA_Success)
  {
    __no_operation(); // Отладочная вставка
  }

  return (osaStatus_t)osa_status;
}

/*----------------------------------------------------------------------------------------------------- 
  Сброс чипа 
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
void App_reset(void)
{
  __disable_interrupt();
  // System Control Block -> Application Interrupt and Reset Control Register
  SCB->AIRCR = 0x05FA0000  // Обязательный шаблон при записи в этот регистр
    | BIT(2);              // Установка бита SYSRESETREQ
  for (;;)
  {
    __no_operation();
  }
}
