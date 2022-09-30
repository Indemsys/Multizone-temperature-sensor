// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.09.25
// 20:49:46
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

// Коды ответов
#define  REPLY_DEVICE_READY_FOR_CMD   0x0000AA00  // Устройство готово к обмену

// Коды команд
#define  CMD_VIRTUAL_UART_MODE        0x00000001  // Команда перехода в режим виртуального UART-а
#define  CMD_PERF_TEST_MODE           0x00000002  // Команда перехода в режим теста производительности


static deviceId_t        g_peer_device_id;
/*------------------------------------------------------------------------------
  Перехватили запись в характеристику сервиса команд
  Выполняется в контексте задачи Main из процедуры BleApp_GattServerCallback

 \param peer_device_id
 \param handle
 \param value
 \param len

 \return uint8_t
 ------------------------------------------------------------------------------*/
uint8_t  CommandService_write(deviceId_t peer_device_id, uint16_t handle, uint8_t *value, uint16_t len)
{
  uint32_t val;


  // Проверяем получили ли мы команду или имя файла
  if (len == 4)
  {
    memcpy(&val, value, 4);
  }
  else return gAttErrCodeUnlikelyError_c;

  DEBUG_CMDSERV_PRINT_ARG("Write attr. 'value_k66_cmd_write'. Val.=%08X\r\n", val);

  switch (val)
  {
  case CMD_VIRTUAL_UART_MODE:
    // Переключаемся на режим виртуального UART-а
    g_communication_mode = COMM_MODE_VIRTUAL_UART;
    g_vuart_peer_dev_id = peer_device_id;
    break;
  case CMD_PERF_TEST_MODE:
    // Переключаемся на режим теста производительности канала связи BLE 
    g_communication_mode = COMM_MODE_PERF_TEST;
    break;
  default:
    // Запоминаем устройство от которого перехватили запрос на запись
    g_peer_device_id = peer_device_id;
    if (Push_message_to_tx_queue(MKW40_CH_CMDMAN, (uint8_t *)&val, sizeof(val)) == RESULT_ERROR) chan_stat.err4++;
    break;
  }


  return gAttErrCodeNoError_c;
}

/*-----------------------------------------------------------------------------------------------------
 
 \param attr_handl       - хэндлер атрибута
 \param cccd_attr_handl  - хэндлер cccd атрибута
 
 \return uint32_t 
-----------------------------------------------------------------------------------------------------*/
static uint32_t CommandService_send_notification(uint16_t attr_handl, uint16_t cccd_attr_handl)
{
  bleResult_t res;

  bool_t      is_notif_active;
  if (Gap_CheckNotificationStatus(g_peer_device_id, cccd_attr_handl, &is_notif_active) == gBleSuccess_c)
  {
    if (is_notif_active == TRUE)
    {
      res = GattServer_SendNotification(g_peer_device_id, attr_handl);
      if (res != gBleSuccess_c)
      {
        return RESULT_ERROR;
      }
      return RESULT_OK;
    }
  }
  return RESULT_ERROR;
}


/*------------------------------------------------------------------------------
   Запись и нотификация о записи в аттрибут данных принятых из канала связи с мастер контроллером
   Выполняется в контексте задачи Master_channel_Task


 \param data
 \param sz
 ------------------------------------------------------------------------------*/
uint32_t CommandService_write_attribute(uint8_t *data, uint32_t sz)
{
  uint32_t res;
  // Записываем ответ от хост контроллера в характеристику
  if (GattDb_WriteAttribute(value_k66_cmd_read, sz, data) == gBleSuccess_c)
  {
    res = CommandService_send_notification(value_k66_cmd_read, cccd_k66_cmd_read);
    if (res == RESULT_ERROR) chan_stat.err2++;
    return res;
  }
  else
  {
    chan_stat.err1++;
    return RESULT_ERROR;
  }
}
/*------------------------------------------------------------------------------
  Перехватили чтение характеристики севиса команд
  Выполняется в контексте задачи Main из процедуры BleApp_GattServerCallback

 \param void
 ------------------------------------------------------------------------------*/
void CommandService_read(void)
{
  uint32_t val = REPLY_DEVICE_READY_FOR_CMD;

  val = val | g_communication_mode;
  // Записываем данные в характеристику
  if (GattDb_WriteAttribute(value_k66_cmd_read, sizeof(val), (uint8_t *)&val) != gBleSuccess_c) chan_stat.err3++;

  DEBUG_CMDSERV_PRINT_ARG("Read attr. 'value_k66_cmd_read'. Val.=%08X \r\n", val);
}
