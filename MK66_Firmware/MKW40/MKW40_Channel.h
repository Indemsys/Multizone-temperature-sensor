#ifndef MKW40_CHANNEL_H
  #define MKW40_CHANNEL_H

#include "MKW40_comm_channel_IDs.h"

typedef void (*T_MKW40_receiver)(uint8_t *data, uint32_t sz, void *ptr);

typedef struct
{
  T_MKW40_receiver   receiv_func;
  void               *pcbl; // Указатель на вспомогательную управляющую структуру для функции приемника
} T_MKW40_recv_subsc;  // Запись подуписки на прием из канала MKW40


void        Task_MKW40(uint32_t parameter);
_mqx_uint   MKW40_subscibe(uint8_t llid, T_MKW40_receiver receiv_func, void *pcbl);
_mqx_uint   MKW40_send_buf(uint8_t llid, uint8_t *data, uint32_t sz);
uint8_t     Is_BLE_activated(void);


#endif // MKW40_CHANNEL_H



