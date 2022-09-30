#ifndef CAN_COMMUNICATION_H
  #define CAN_COMMUNICATION_H


// Управляющая структура для алгоритма упаковки состояния сигналов в 8-и байтный пакет CAN
typedef struct
{
  uint8_t            itype;    // Тип  сигнала. 0 - простой контакт бистабильный, 1 - контакт в цепи безопасности с 3-я состояниями
  uint8_t            nbyte;    // Номер байта
  uint8_t            nbit;     // Номер бита в байте
  int8_t             *val;     // Указатель на переменную состояния сигнала
  int8_t             *val_prev;// Указатель на переменную состояния сигнала
  int8_t             *flag;    // Указатель на флаг переменной. Флаг не равный нулю указывает на произошедшее изменение состояния переменной
} T_can_inp_pack;


// Структура алгоритма упаковки состояния выходов в 8-и байтный пакет CAN
typedef struct
{
  uint8_t            nbyte;    // Номер байта
  uint8_t            nbit;     // Номер бита в байте
  int8_t             *val;     // Указатель на переменную состояния сигнала
} T_can_out_pack;


_mqx_uint  CAN_send_packed_inputs(uint32_t canid, T_can_inp_pack *parr, uint32_t sz);
int32_t    CAN_unpack_received_inputs(T_can_rx *rx, T_can_inp_pack *parr, uint32_t sz);
_mqx_uint  CAN_send_packed_outputs(uint32_t canid, T_can_out_pack *parr, uint32_t sz);
_mqx_uint  CAN_unpack_received_outputs(T_can_rx *rx, T_can_out_pack *parr, uint32_t sz);


#endif // CAN_COMMUNICATION_H



