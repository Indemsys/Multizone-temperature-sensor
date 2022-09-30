#ifndef BLE_HOST_CHANNEL_H
  #define BLE_HOST_CHANNEL_H

#define MAX_BLE_STR_SZ 20 // Максимальное количество символов в строках применяемых в  BLE стеке 


#define MKW_DATA_SZ  MAX_BLE_STR_SZ   // Количество полезных данных в пакете
#define MKW_BUF_SZ   (MKW_DATA_SZ+2)  // Вместимость всего пакета, один байт идентификатор пакета и один байт длины


#define MKW_QUEUE_SZ 4  // Количество запросов на отправку в очереди


typedef struct
{
  uint32_t err1;  // Количество ошибок при записи в аттрибут командного сервиса
  uint32_t err2;
  uint32_t err3;
  uint32_t err4;
}
T_master_channel_statictic;


#define COMM_MODE_PERF_TEST      BIT(0) 
#define COMM_MODE_VIRTUAL_UART   BIT(1) 

#ifdef _MASTER_CHANNEL_GLOBALS_
T_master_channel_statictic chan_stat;

uint32_t  g_communication_mode;
uint32_t  g_vuart_peer_dev_id;         // Идентификатор устройства начавшего сессию виртуального UART-а 

char      advertised_device_name[MAX_BLE_STR_SZ+1];
uint32_t  pin_code = 999999;
char      software_revision[MAX_BLE_STR_SZ+1];

#else
extern T_master_channel_statictic chan_stat;

extern uint32_t  g_communication_mode;
extern uint32_t  g_vuart_peer_dev_id;

extern char      advertised_device_name[MAX_BLE_STR_SZ+1];
extern uint32_t  pin_code;
extern char      software_revision[MAX_BLE_STR_SZ+1];

#endif

void    Init_master_channel_task(void);
int32_t Push_message_to_tx_queue(uint8_t chanid, uint8_t *data, uint32_t sz);

#endif // BLE_HOST_CHANNEL_H



