// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2017.05.24
// 15:44:11
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"


/*-----------------------------------------------------------------------------------------------------
  Функция вызывается при перехвате события записи в аттрибуты value_k66wrdata_w_resp или value_k66wrdata в базе данных BLE
 
 \param attr_handle 
 \param value 
 \param len 
 
 \return uint8_t 
-----------------------------------------------------------------------------------------------------*/
uint8_t Write_to_value_k66wrdata(deviceId_t peer_device_id, uint16_t attr_handle, uint8_t *dbuf, uint16_t len)
{
  if (g_communication_mode == COMM_MODE_PERF_TEST)
  {
    return PerfTest_Write_w_resp(attr_handle, dbuf, len);
  }
  else
  {
    if (peer_device_id == g_vuart_peer_dev_id)
    {
      if (Push_message_to_tx_queue(MKW40_CH_VUART, dbuf, len) == RESULT_OK)
      {
        DEBUG_VUARTSERV_PRINT_ARG("Write_to_value_k66wrdata. (len=%d) Res= Ok!\r\n",len);
        return gAttErrCodeNoError_c;
      }
    }
    DEBUG_VUARTSERV_PRINT_ARG("Write_to_value_k66wrdata. (len=%d) Res= Error!\r\n",len);
    return gAttErrCodeUnlikelyError_c;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Функция вызывается при перехвате события записи в аттрибут value_k66rddata в базе данных BLE 
 
 \param peer_device_id 
 \param attr_handle 
 \param dbuf 
 \param len 
 
 \return uint8_t 
-----------------------------------------------------------------------------------------------------*/
uint8_t  Write_to_value_k66rddata(deviceId_t peer_device_id, uint16_t attr_handle, uint8_t *dbuf, uint16_t len)
{
  if (g_communication_mode == COMM_MODE_PERF_TEST)
  {
    return PerfTest_Read_prepare(peer_device_id, attr_handle, dbuf, len); //Инициализация теста чтения
  }

  return 0;
}

/*------------------------------------------------------------------------------
 Функция вызывается при перехвате события чтения из аттрибута value_k66rddata в базе данных BLE 
 
 Подготовка данных теста производительности чтения
 
 \param void
 ------------------------------------------------------------------------------*/
void Read_from_value_k66rddata(void)
{
  if (g_communication_mode == COMM_MODE_PERF_TEST)
  {
    PerfTest_Read();
  }
}


/*-----------------------------------------------------------------------------------------------------
  Вызывается из задачи Master_channel_Task в случае когда от мастер контроллера пришли данные на отправку клиенту виртуального UART
 
 \param attr_handle 
 \param dbuf 
 \param len 
 
 \return uint32_t 
-----------------------------------------------------------------------------------------------------*/
int32_t Notif_ready_k66rddata(uint8_t *dbuf, uint16_t len)
{
  uint32_t    i;
  bleResult_t res;
  bool_t      is_notif_active;

  if (g_communication_mode == COMM_MODE_VIRTUAL_UART)
  {
    // Проверим подписался ли клиент к данному аттрибуту на нотификацию
    if (Gap_CheckNotificationStatus(g_vuart_peer_dev_id, cccd_k66rddata, &is_notif_active) == gBleSuccess_c)
    {
      if (is_notif_active == TRUE)
      {
        // Следим за свободным местом в пуле блоков памяти
        // В случае если пул не имеет достаточно места стек BLE может зависнуть
        // Предпринимаем 10 попыток дождаться освобождения пула
        for (i = 0; i < 10; i++)
        {
          if (Get_boggest_pool_free_block_cnt() > 3)
          {
            res = GattDb_WriteAttribute(value_k66rddata, len, dbuf); // Записываем данные в характеристику, чтобы их мог прочитать клиент
            if (res == gBleSuccess_c)
            {
              res = GattServer_SendNotification(g_vuart_peer_dev_id, value_k66rddata);
              if (res == gBleSuccess_c)
              {
                DEBUG_VUARTSERV_PRINT_ARG("Notif_ready_k66rddata. (len=%d) Res= Ok!\r\n",len);
                return RESULT_OK;
              }
            }
          }
          vTaskDelay(1);
        }
      }
    }
  }
  DEBUG_VUARTSERV_PRINT_ARG("Notif_ready_k66rddata. (len=%d) Res= Error!\r\n",len);
  return RESULT_ERROR;
}


