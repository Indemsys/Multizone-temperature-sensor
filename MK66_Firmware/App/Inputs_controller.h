#ifndef INPUTS_CONTROLLER_H
  #define INPUTS_CONTROLLER_H

// Структура для обработчика подавления дребезга
typedef struct
{
  int8_t    val;
  int8_t    curr;
  int8_t    prev;
  uint32_t  cnt;
  uint8_t   init;

} T_bncf;

#define GEN_SW     0
#define ESC_SW     1

// Структура алгоритма определения состояния сигналов
typedef struct
{
  uint8_t            itype;    // Тип  сигнала. 0 - простой контакт бистабильный, 1 - контакт в цепи безопасности с 3-я состояниями
  uint16_t           *p_smpl1; // Указатель на результат сэмплирования напряжения на контакте с более высоким напряжением
  uint16_t           *p_smpl2; // Указатель на результат сэмплирования напряжения на контакте с более низким напряжением
  uint16_t           lbound;   // Граница между логическим 0 и 1 на входе
  uint32_t           l0_time;  // Время устоявшегося состояния для фиксации низкого уровня сигнала
  uint32_t           l1_time;  // Время устоявшегося состояния для фиксации высокого уровня сигнала
  uint32_t           lu_time;  // Время устоявшегося состояния для фиксации неопределенного уровня сигнала
  int8_t             *val;     // Указатель на переменную для сохранения вычисленного состояния входа
  int8_t             *val_prev;// Указатель на переменную для сохранения предыдущего состояния входа
  int8_t             *flag;    // Указатель на флаг переменной. Флаг не равный нулю указывает на произошедшее изменение состояния переменной
  T_bncf             pbncf;    // Структура для алгоритма фильтрации дребезга
} T_input_cbl;



void      Inputs_create_sync_obj(void);
void      Inputs_set_ADC_evt(void);
void      Inputs_set_changed_evt(void);
_mqx_uint Inputs_wait_for_ADC_event(void);
_mqx_uint Inputs_wait_for_changed_event(uint32_t ticks);
void      Inputs_processing_task(void);

uint32_t  Inputs_do_processing(T_input_cbl *scbl);

#endif // INPUTS_PROCESSING_H



