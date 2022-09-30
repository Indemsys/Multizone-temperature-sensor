// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.09.19
// 15:58:24
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define _MASTER_CHANNEL_GLOBALS_
#include   "App.h"



// Принимаем и отправляем данные внешнему контроллеру в режиме SPI мастера
// Посылаем пакеты по SPI размером MKW_BUF_SZ каждые 2 мс
// Структура пакета:
// Номер байта   Назначение
// в пакете
// 0     [llid]  - идентификатор логического канала
// 1     [LEN]   - колическтво передаваемых данных
// 2     [data0] - первый байт данных
// ...
// LEN+2 [dataN] - последний байт данных


// Очередь буферов на отправку
volatile uint32_t          master_queue_head;
volatile uint32_t          master_queue_tail;
uint8_t                    master_queue[MKW_QUEUE_SZ][MKW_BUF_SZ];

task_handler_t             spi_chan_task_id;

volatile uint8_t           unhandled_ch;

extern event_t            app_event_struct;
int32_t                   App_accept_parameter(uint8_t *dbuf);
void                      App_reset(void);

OSA_TASK_DEFINE(HOSTCHAN, SPI_CHAN_TASK_STACK_SZ);

static void Parse_received_data(void);
static void Move_msg_from_queue_to_trasmitter(void);
static void Master_channel_Task(task_param_t argument);
/*------------------------------------------------------------------------------
  Инициализация задачи коммуникационного канала с хост микроконтроллером

 \param void
 ------------------------------------------------------------------------------*/
void Init_master_channel_task(void)
{
  OSA_TaskCreate(Master_channel_Task, "SPI_Chan", SPI_CHAN_TASK_STACK_SZ, HOSTCHAN_stack, SPI_CHAN_TASK_PRIO, (task_param_t)NULL, FALSE, &spi_chan_task_id);
}

/*------------------------------------------------------------------------------
 Задача организует непрерывную отсылку-прием пакетов по 22 байта каждые 2 мс (1 тик операционной системы)


 \param argument
 ------------------------------------------------------------------------------*/
static void Master_channel_Task(task_param_t argument)
{
  g_communication_mode = COMM_MODE_PERF_TEST;

  while (1)
  {
    Move_msg_from_queue_to_trasmitter();  // Переносим данные из очереди в передатчик SPI
    BSP_SPI_DMA_read_write();             // Отправляем и читаем данные по SPI

    Parse_received_data();                // Анализируем принятые данные и распределяем их по назначению

    if (g_communication_mode == COMM_MODE_PERF_TEST) PerfomanceTest_read_notification();   // Процедура отправки пакетов во время тестирования промизводительности чтения

    vTaskDelay(1);
    Set_PTC6_state(Get_PTC6_state() ^ 1);
  };
}


/*------------------------------------------------------------------------------



 \param void
 ------------------------------------------------------------------------------*/
static void Parse_received_data(void)
{
  volatile uint8_t dummy;
  uint8_t          *data;
  data = BSP_SPI_DMA_Get_rx_buf();


  switch (data[0])
  {
  case MKW40_CH_CMDMAN:
    CommandService_write_attribute(&data[2], data[1]);
    break;
  case MKW40_CH_VUART:
    Notif_ready_k66rddata(&data[2], data[1]);
    break;
  case MKW40_CH_SETT_DONE:
    OSA_EventSet(&app_event_struct, EVENT_SETTINGS_DONE);
    break;
  case MKW40_CH_SETT_WRITE:
    App_accept_parameter(&data[1]);
    break;
  case MKW40_CH_RESET:
    App_reset();
    break;

  default:
    unhandled_ch = data[0];
    break;
  }
}

/*------------------------------------------------------------------------------
 Переносим данные из очереди в передатчик SPI


 \param void
 ------------------------------------------------------------------------------*/
static void Move_msg_from_queue_to_trasmitter(void)
{
  uint32_t ptail;
  uint32_t tail;
  uint8_t  *data;
  uint8_t  sz;

  data = BSP_SPI_DMA_Get_tx_buf(); // Получим указатель на буфер отправки канала SPI
  memset(data, 0, MKW_BUF_SZ);     // Предварительно буфер отправки заполняем нулями

  ptail = master_queue_tail;
  if (ptail == master_queue_head) return; // Выходим данных нет
  tail = ptail + 1;
  if (tail >= MKW_QUEUE_SZ) tail = 0;

  sz = master_queue[ptail][1] + 2;       // Находим размер данных на основе информации о структуре пакета
  memcpy(data, master_queue[ptail], sz); // Заполняем буфер отправки из буфера очереди

  master_queue_tail = tail; // Смещаем указатель хвоста
}

/*------------------------------------------------------------------------------
  Помещаем данные в очередь для передачи мастер контроллеру


 \param data
 \param sz
 ------------------------------------------------------------------------------*/
int32_t Push_message_to_tx_queue(uint8_t chanid, uint8_t *data, uint32_t sz)
{
  uint32_t phead;
  uint32_t head;

  // Помещаем данные в кольцевой буффер
  if (sz > MKW_DATA_SZ) return RESULT_ERROR; // Выходим, объем данных слишком большой

  phead = master_queue_head;
  head = phead + 1;
  if (head >= MKW_QUEUE_SZ) head = 0;
  if (head == master_queue_tail) return RESULT_ERROR; //Выходим, в очереди нету места

  // Переносим данные в буфер очереди
  master_queue[phead][0] = chanid;
  if ((sz!=0) && (data!=0))
  {
    master_queue[phead][1] = sz;
    memcpy(&master_queue[phead][2], data, sz);
  }
  else
  {
    master_queue[phead][1] = 0;
  }

  // Смещаем указатель головы очереди
  master_queue_head = head;
  return RESULT_OK;
}

