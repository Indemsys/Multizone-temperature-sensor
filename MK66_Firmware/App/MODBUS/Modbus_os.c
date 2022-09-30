#include "app.h"


#if (MODBUS_CFG_MASTER_EN == 1)
static  LWEVENT_STRUCT       MB_OS_rx_signal_semafores[MODBUS_CFG_MAX_CH]; // Массив семафоров для каналов мастера через которые сигнализируется о приеме данных
#endif

#if (MODBUS_CFG_SLAVE_EN  == 1)
static uint32_t              MB_OS_rx_queue[sizeof(LWMSGQ_STRUCT)/sizeof(uint32_t)+ MODBUS_CFG_MAX_CH * sizeof(void*)]; // Очередь через которую слэйву сообщается о приеме данных
static _task_id              rx_task;
#endif

#if (MODBUS_CFG_MASTER_EN == 1)
static  void                 MB_OS_init_master(void);
static  void                 MB_OS_deinit_master(void);
#endif

#if (MODBUS_CFG_SLAVE_EN == 1)
static  void                 MB_OS_init_slave(void);
static  void                 MB_OS_deinit_slave(void);
static  void                 MB_OS_slave_rx_task(uint32_t p_arg);
#endif


#if (MODBUS_CFG_MASTER_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Инициализация сервисов RTOS используемых в работе мастера

  \param
-----------------------------------------------------------------------------------------------------*/
static  void  MB_OS_init_master(void)
{
  uint8_t   i;

  for (i = 0; i < MODBUS_CFG_MAX_CH; i++)                             /* Create a semaphore for each channel   */
  {
    _lwevent_create(&MB_OS_rx_signal_semafores[i], LWEVENT_AUTO_CLEAR);
  }
}
#endif



#if (MODBUS_CFG_SLAVE_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Инициализация сервисов RTOS используемых в работе слэйва

  \param
-----------------------------------------------------------------------------------------------------*/
static  void  MB_OS_init_slave(void)
{
  _mqx_uint res;

  res = _lwmsgq_init((void *)MB_OS_rx_queue, MODBUS_CFG_MAX_CH, sizeof(void *));

  if (res == MQX_OK)
  {

    TASK_TEMPLATE_STRUCT    task_template = {0};
    task_template.TASK_NAME          = "Modbus Rx";
    task_template.TASK_PRIORITY      = MODBUS_SLAVE_RX_PRIO;
    task_template.TASK_STACKSIZE     = MODBUS_SLAVE_RX_STACK;
    task_template.TASK_ADDRESS       = MB_OS_slave_rx_task;
    task_template.TASK_ATTRIBUTES    = MQX_FLOATING_POINT_TASK;
    task_template.CREATION_PARAMETER = 0;
    rx_task =  _task_create(0, 0, (uint32_t)&task_template);
  }

}

#endif

/*-----------------------------------------------------------------------------------------------------
  Инициализация сервисов RTOS используемых в работе модуля MODBUS


  \param void
-----------------------------------------------------------------------------------------------------*/
void  MB_OS_Init(void)
{
#if (MODBUS_CFG_MASTER_EN == 1)
  MB_OS_init_master();
#endif

#if (MODBUS_CFG_SLAVE_EN == 1)
  MB_OS_init_slave();
#endif
}




#if (MODBUS_CFG_MASTER_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Завершение работы сервисов RTOS задействованных в работе мастера

  \param
-----------------------------------------------------------------------------------------------------*/
static  void  MB_OS_deinit_master(void)
{
  uint8_t  i;

  for (i = 0; i < MODBUS_CFG_MAX_CH; i++)
  {
    _lwevent_destroy(&MB_OS_rx_signal_semafores[i]);
  }
}

#endif

#if (MODBUS_CFG_SLAVE_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Завершение работы сервисов RTOS задействованных в работе слэйва

  \param void
-----------------------------------------------------------------------------------------------------*/
void  MB_OS_deinit_slave(void)
{
  _task_destroy(rx_task);                        /* Delete Modbus Rx Task                 */
  _lwmsgq_deinit(MB_OS_rx_queue);
}
#endif

/*-----------------------------------------------------------------------------------------------------
  Завершение работы сервисов RTOS задействованных в модуле MODBUS

  \param void
-----------------------------------------------------------------------------------------------------*/
void  MB_OS_deinit(void)
{
#if (MODBUS_CFG_MASTER_EN == 1)
  MB_OS_deinit_master();
#endif

#if (MODBUS_CFG_SLAVE_EN == 1)
  MB_OS_deinit_slave();
#endif
}

/*-----------------------------------------------------------------------------------------------------
  Функция сигналящая об окончании приема пакета
  Вызывается из обработчика прерывания таймера в режиме RTU
  или из обработчика прерываний  UART в режие ASCII

  \param pch
-----------------------------------------------------------------------------------------------------*/
void  MB_OS_packet_end_signal(T_MODBUS_ch *pch)
{
  if (pch != (T_MODBUS_ch *)0)
  {
    switch (pch->master_slave_flag)
    {
#if (MODBUS_CFG_MASTER_EN == 1)

    case MODBUS_MASTER:
      _lwevent_set(&MB_OS_rx_signal_semafores[pch->modbus_channel], 1);
      //ITM_EVENT8(1,0);
      break;

#endif

#if (MODBUS_CFG_SLAVE_EN == 1)

    case MODBUS_SLAVE:
    default:
      _lwmsgq_send(MB_OS_rx_queue, (void *)pch, LWMSGQ_SEND_BLOCK_ON_FULL); // Посылаем в очередь указатель на управляющую структуру канала принявшего пакет

      break;
#endif
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Ожидаем получения пакета данных

  \param pch
  \param perr
-----------------------------------------------------------------------------------------------------*/
void  MB_OS_wait_rx_packet_end(T_MODBUS_ch   *pch, uint16_t  *perr)
{
#if (MODBUS_CFG_MASTER_EN == 1)

  _mqx_uint  res;


  if (pch != (T_MODBUS_ch *)0)
  {
    if (pch->master_slave_flag == MODBUS_MASTER)
    {
      res = _lwevent_wait_ticks(&MB_OS_rx_signal_semafores[pch->modbus_channel], 1, TRUE,  ms_to_ticks(pch->rx_timeout));

      switch (res)
      {
      case LWEVENT_WAIT_TIMEOUT:
        if (pch->rx_packet_size < MODBUS_RTU_MIN_MSG_SIZE)
        {
          *perr = MODBUS_ERR_TIMED_OUT;
        }
        else
        {
          *perr = MODBUS_ERR_NONE;
        }
        break;

      case MQX_OK:
        //ITM_EVENT8(1,1);
        *perr = MODBUS_ERR_NONE;
        break;
      default:
        *perr = MODBUS_ERR_INVALID;
        break;
      }
    }
    else
    {
      *perr = MODBUS_ERR_NOT_MASTER;
    }
  }
  else
  {
    *perr = MODBUS_ERR_NULLPTR;
  }
#else
  *perr = MODBUS_ERR_INVALID;
#endif
}

#if (MODBUS_CFG_SLAVE_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Задача обработки запросов слэйвом

  \param p_arg
-----------------------------------------------------------------------------------------------------*/
static  void  MB_OS_slave_rx_task(uint32_t p_arg)
{
  T_MODBUS_ch  *pch;

  (void)p_arg;

  while (1)
  {
    _lwmsgq_receive(MB_OS_rx_queue, (uint32_t *)pch, LWMSGQ_RECEIVE_BLOCK_ON_EMPTY , 0, 0);
    MB_slave_rx_task(pch);
  }
}

#endif


