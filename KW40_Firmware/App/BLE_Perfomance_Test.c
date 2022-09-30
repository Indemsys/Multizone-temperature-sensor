// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.09.21
// 17:42:46
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

#define TEST_DATA_BUF_SZ 20
static uint64_t         g_rcv_time;
static uint64_t         g_start_rcv_time;
static T_perf_test_data g_ptd;
static uint32_t         g_transf_data_cnt  = 0;
static uint32_t         g_transf_data_sz;
static uint32_t         g_notification; // Флаг отправки данных теста чтения способом нотификации

static uint8_t          g_test_data_buf[TEST_DATA_BUF_SZ];
static uint8_t          g_read_test_active;
static deviceId_t       g_peer_device_id;
static uint16_t         g_perf_rw_attr_handle;
/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
void Restart_perfomance_test()
{
  DEBUG_PERF_TEST_PRINT_ARG("Perfomance test restart\r\n", 0);

  g_ptd.test_data_crc =  0;
  g_transf_data_cnt = 0;

}

/*-----------------------------------------------------------------------------------------------------
 Функция вызывается при перехвате события записи в аттрибуты value_k66wrdata_w_resp или value_k66wrdata
 в базе данных BLE в режиме тестирования производительности канала
 
 Функция обратно в этот же перехватываемый аттрибут записывает структуру с контрольной суммой полученных данных
 и значением времени прошедшего от приема первого пакета до приема текущего пакета 
 
 При завершении теста тестирующая сторона может прочитать аттрибут и проверить правильность контрольной суммы и узнать общее время приема всех пакетов 
 
 \param handle 
 \param value 
 \param len 
 
 \return uint8_t 
-----------------------------------------------------------------------------------------------------*/
uint8_t PerfTest_Write_w_resp(uint16_t handle, uint8_t *value, uint16_t len)
{
  bleResult_t res;
  g_rcv_time = BSP_Read_PIT();
  g_ptd.test_data_crc = Finish_CRC32(g_ptd.test_data_crc);
  g_ptd.test_data_crc = Update_CRC32(g_ptd.test_data_crc, value, len);
  g_ptd.test_data_crc = Finish_CRC32(g_ptd.test_data_crc);

  if (g_transf_data_cnt == 0)
  {
    g_start_rcv_time = g_rcv_time; // Запоминаем стартовое время 
  }
  g_ptd.delta_t = (uint32_t)(g_rcv_time - g_start_rcv_time);
  res = GattDb_WriteAttribute(handle, sizeof(g_ptd), (uint8_t *)&g_ptd); // Записываем контрольную сумму обратно в прочитанный аттрибут чтобы ее мог прочитатть сервер
  if (res != gBleSuccess_c)
  {
    DEBUG_PERF_TEST_PRINT_ARG("GattDb_WriteAttribute Error = %d\r\n", res);
  }

  g_transf_data_cnt += len;
  DEBUG_PERF_TEST_PRINT_ARG("Received:%06d Time stump=%08X%08X\r\n", g_transf_data_cnt, (uint32_t)(g_rcv_time >> 32), (uint32_t)(g_rcv_time & 0xFFFFFFFF));


  //Debug_dump_8bit_mem(len, value);
  return gAttErrCodeNoError_c;

}

/*-----------------------------------------------------------------------------------------------------
 
 \param peer_device_id 
 \param handle 
 \param dbuf   - буфер с настроечной информацией: количество читаемых данных и необходимость нотификации 
 \param len 
 
 \return uint8_t 
-----------------------------------------------------------------------------------------------------*/
uint8_t  PerfTest_Read_prepare(deviceId_t peer_device_id, uint16_t handle, uint8_t *dbuf, uint16_t len)
{
  Restart_perfomance_test();

  // Инициализируем переменную с установкой количества данных
  if (len >= sizeof(g_transf_data_sz))
  {
    memcpy(&g_transf_data_sz, dbuf, sizeof(g_transf_data_sz)); // Сохраняем количество читаемых байт данных
  }
  else return gAttErrCodeRequestNotSupported_c;

  // Инициализируем переменную с установкой необходимости нотификации
  if (len >= (sizeof(g_transf_data_sz) +  sizeof(g_notification)))
  {
    memcpy(&g_notification, dbuf + sizeof(g_transf_data_sz), sizeof(g_notification)); // Сохраняем флаг нотификации
    if (g_notification > 1) g_notification = 0;
  }
  else g_notification = 0;

  DEBUG_PERF_TEST_PRINT_ARG("Read perfomance test params: size=%d, notif=%d \r\n", g_transf_data_sz, g_notification);

  g_peer_device_id = peer_device_id;
  g_perf_rw_attr_handle = handle;
  g_read_test_active = 1;
  return gAttErrCodeNoError_c;
}

/*-----------------------------------------------------------------------------------------------------
  Функция вызывается при перехвате события чтения из аттрибута value_k66rddata в базе данных BLE в режиме тестирования производительности 
 
  В ответ функция в аттрибут g_perf_rw_attr_handle записывает блок тестовых данных
 
  При завершении теста функция записывает в аттрибут g_perf_rw_attr_handle контрольную сумму и общее время отсылки всех пакетов 
  Эти данные читает тестирующая сторона чтобы проверить корректность чтения и общее время 
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
void PerfTest_Read(void)
{
  bleResult_t res;
  uint32_t    len;
  uint32_t    i;
  // Фиксируем момент времени чтения данных
  g_rcv_time = BSP_Read_PIT();
  if (g_transf_data_cnt == 0)
  {
    g_start_rcv_time = g_rcv_time; // Запоминаем стартовое время 
  }
  g_ptd.delta_t = (uint32_t)(g_rcv_time - g_start_rcv_time);

  // Генерируем случайные числа и записываем их в характеристику теста
  len = g_transf_data_sz - g_transf_data_cnt;
  if (len != 0)
  {
    if (len > TEST_DATA_BUF_SZ) len = TEST_DATA_BUF_SZ;

    for (i = 0; i < len; i++)
    {
      g_test_data_buf[i] = rand();
    }
    g_transf_data_cnt += len;

    // Подсчитываем контрольную сумму
    g_ptd.test_data_crc = Finish_CRC32(g_ptd.test_data_crc);
    g_ptd.test_data_crc = Update_CRC32(g_ptd.test_data_crc, g_test_data_buf, len);
    g_ptd.test_data_crc = Finish_CRC32(g_ptd.test_data_crc);

    DEBUG_PERF_TEST_PRINT_ARG("Read test block. CNT=%06d Time stump=%08X%08X\r\n", g_transf_data_cnt, (uint32_t)(g_rcv_time >> 32), (uint32_t)(g_rcv_time & 0xFFFFFFFF));
    res = GattDb_WriteAttribute(g_perf_rw_attr_handle, len, g_test_data_buf); // Записываем данные в характеристику, чтобы их мог прочитать клиент
    if (res != gBleSuccess_c)
    {
      DEBUG_PERF_TEST_PRINT_ARG("GattDb_WriteAttribute Error = %d\r\n", res);
    }
  }
  else
  {
    if (g_transf_data_cnt != 0)
    {
      // len = 0, Значит переданы все данные. В характеристику сохраняем crc и время
      DEBUG_PERF_TEST_PRINT_ARG("Read test   END. CRC=%08X Delta T.=%d\r\n", g_ptd.test_data_crc, g_ptd.delta_t);
      // Записываем контрольную сумму обратно в прочитанный аттрибут чтобы ее мог прочитатть сервер
      res = GattDb_WriteAttribute(g_perf_rw_attr_handle, sizeof(g_ptd), (uint8_t *)&g_ptd);
      if (res != gBleSuccess_c)
      {
        DEBUG_PERF_TEST_PRINT_ARG("GattDb_WriteAttribute Error = %d\r\n", res);
      }
      g_read_test_active = 0;
      Restart_perfomance_test(); // Подготавливаемся сразу же к другому тесту
    }
  }
}


/*------------------------------------------------------------------------------
  Периодически вызывается из сторонней задачи для отправки нотификаций о новом значении в  тестовой характеристике


 \param void
 ------------------------------------------------------------------------------*/
uint32_t PerfomanceTest_read_notification(void)
{
  bleResult_t res;

  if ((g_notification != 0) && (g_read_test_active != 0))
  {
    bool_t is_notif_active;
    if (Gap_CheckNotificationStatus(g_peer_device_id, cccd_k66rddata, &is_notif_active) == gBleSuccess_c)
    {
      if (is_notif_active == TRUE)
      {
        // Следим за свободным местом в пуле блоков памяти
        // В случае если пул не имеет достаточно места стек BLE может зависнуть
        if (Get_boggest_pool_free_block_cnt() > 3) 
        {
          Read_from_value_k66rddata();
          DEBUG_PERF_TEST_PRINT_ARG("Send notification.\r\n", 0);
          res = GattServer_SendNotification(g_peer_device_id, g_perf_rw_attr_handle);
          if (res != gBleSuccess_c)
          {
            DEBUG_PERF_TEST_PRINT_ARG("GattServer_SendNotification Error = %d\r\n", res);
            return RESULT_ERROR;
          }
          return RESULT_OK;
        }
        else return RESULT_ERROR;
      }
    }
  }
  return RESULT_ERROR;
}
/*------------------------------------------------------------------------------



 \param cValueLength
 \param aValue
 ------------------------------------------------------------------------------*/
void Debug_dump_8bit_mem(uint16_t len, uint8_t *mem)
{
  uint32_t i;

  DEBUG_PERF_TEST_PRINT_ARG("\r\n", 0);
  while (len > 0)
  {
    for (i = 0; i < 16; i++)
    {
      DEBUG_PERF_TEST_PRINT_ARG(" %02X", *mem);
      mem++;
      len--;
      if (len == 0) break;
    }
    DEBUG_PERF_TEST_PRINT_ARG("\r\n", 0);
  }
}

/*------------------------------------------------------------------------------



 \param cValueLength
 \param aValue
 ------------------------------------------------------------------------------*/
void Debug_dump_32bit_mem(uint16_t len, uint32_t *mem)
{
  uint32_t i;

  DEBUG_PERF_TEST_PRINT_ARG("\r\n", 0);
  while (len > 0)
  {
    for (i = 0; i < 8; i++)
    {
      DEBUG_PERF_TEST_PRINT_ARG(" %08X", *mem);
      mem++;
      len--;
      if (len == 0) break;
    }
    DEBUG_PERF_TEST_PRINT_ARG("\r\n", 0);
  }
}
