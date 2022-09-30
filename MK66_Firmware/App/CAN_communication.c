// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.07.08
// 08:52:18
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"


/*------------------------------------------------------------------------------
  Запаковываем значения состояний входов в пакет CAN


 \param canid      - идентификатор пакета
 \param parr       - массив с описанием упаковки
 \param sz         - размер массива
 ------------------------------------------------------------------------------*/
_mqx_uint  CAN_send_packed_inputs(uint32_t canid, T_can_inp_pack *parr, uint32_t sz)
{
  uint32_t i;
  uint8_t  data[8];
  uint8_t  len = 0;

  memset(data, 0, 8);

  for (i = 0; i < sz; i++)
  {
    if (parr[i].itype == GEN_SW)
    {
      data[parr[i].nbyte] = data[parr[i].nbyte] | ((*parr[i].val & 1) << parr[i].nbit);
    }
    else
    {
      data[parr[i].nbyte] = data[parr[i].nbyte] | ((*parr[i].val & 3) << parr[i].nbit);
    }
    if ((parr[i].nbyte + 1) > len)
    {
      len = parr[i].nbyte + 1;
    }
  }

  return CAN_post_packet_to_send(canid, data, len);
}

/*------------------------------------------------------------------------------
 Распаковываем значения состояний входов из пакета CAN


 \param rx       - принятый пакет
 \param parr     - массив с описанием упаковки
 \param sz       - размер массива

 \return _mqx_uint возвращает 0 есди не было изменений, 1 - если были изменения
 ------------------------------------------------------------------------------*/
int32_t CAN_unpack_received_inputs(T_can_rx *rx, T_can_inp_pack *parr, uint32_t sz)
{
  uint32_t i;
  int8_t   v;
  int32_t  f = 0;

  for (i = 0; i < sz; i++)
  {
    if (parr[i].itype == GEN_SW)
    {
      v = (rx->data[parr[i].nbyte] >> parr[i].nbit) & 1;
    }
    else
    {
       v = (rx->data[parr[i].nbyte] >> parr[i].nbit) & 3;
       if (v == 3) v = -1;
    }
    if (*parr[i].val != v)
    {
      *parr[i].val_prev = *parr[i].val;
      *parr[i].val = v;
      *parr[i].flag = 1;
      f = 1;
    }
  }
  return f;
}

/*------------------------------------------------------------------------------
  Запаковываем значения состояний входов в пакет CAN


 \param canid      - идентификатор пакета
 \param parr       - массив с описанием упаковки
 \param sz         - размер массива
 ------------------------------------------------------------------------------*/
_mqx_uint  CAN_send_packed_outputs(uint32_t canid, T_can_out_pack *parr, uint32_t sz)
{
  uint32_t i;
  uint8_t  data[8];
  uint8_t  len = 0;

  memset(data, 0, 8);

  for (i = 0; i < sz; i++)
  {
    data[parr[i].nbyte] = data[parr[i].nbyte] | ((*parr[i].val & 1) << parr[i].nbit);
    if ((parr[i].nbyte + 1) > len)
    {
      len = parr[i].nbyte + 1;
    }
  }
  return CAN_post_packet_to_send(canid, data, len);
}

/*------------------------------------------------------------------------------
 Распаковываем значения состояний выходов из пакета CAN


 \param rx       - принятый пакет
 \param parr     - массив с описанием упаковки
 \param sz       - размер массива

 \return _mqx_uint
 ------------------------------------------------------------------------------*/
_mqx_uint CAN_unpack_received_outputs(T_can_rx *rx, T_can_out_pack *parr, uint32_t sz)
{
  uint32_t i;
  int8_t   v;
  for (i = 0; i < sz; i++)
  {
    v = (rx->data[parr[i].nbyte] >> parr[i].nbit) & 1;
    if (*parr[i].val != v)
    {
      *parr[i].val = v;
    }
  }
  return MQX_OK;
}


