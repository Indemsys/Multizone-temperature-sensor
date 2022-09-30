#ifndef __MZTS_ANALOG_INS_H
  #define __MZTS_ANALOG_INS_H


#define ADC_SCAN_SZ       4

#define ADC_PERIOD        500  // Период выборки цифро-анлогового преобразователя (ADC) в мкс
#define ADC_TASK_FREQ     (1000000.0/ADC_PERIOD) // Частоты вызовов задачи измерений
#define INPUTS_TASK_PERIOD_DIV  2
#define INPUTS_TASK_PERIOD      (INPUTS_TASK_PERIOD_DIV*ADC_PERIOD)

#define DEST_MODULO       4    // Количество младших бит адреса разрешенных к изменению в адресе приемника результатов ADC. 6 бит соответствует адресации  2-х буферов по 16 16-и битных слов - 64 байта
#define ADC0_CHAN_SEL     1
#define ADC1_CHAN_SEL     1

typedef struct
{
  uint16_t smpl_TSensor0    ; // TSensor0    - Внутреннй температурный сенсор
  uint16_t smpl_TSensor1    ; // TSensor1    - Внутреннй температурный сенсор
  uint16_t smpl_VREFH1      ; // VREFH1      - Внутренний верхний опорный сигнал
  uint16_t smpl_VREFL1      ; // VREFL1      - Внутренний нижний  опорный сигнал

  uint16_t smpl_TSensor2    ; // TSensor2    - Внутреннй температурный сенсор
  uint16_t smpl_TSensor3    ; // TSensor3    - Внутреннй температурный сенсор
  uint16_t smpl_VREFH2      ; // VREFH2      - Внутренний верхний опорный сигнал
  uint16_t smpl_VREFL2      ; // VREFL2      - Внутренний нижний  опорный сигнал
}
T_ADC_res;

int32_t Get_TSensor1     (void);
int32_t Get_VREFH1       (void);
int32_t Get_VREFL1       (void);
                               ;

float   Value_TSensor1    (int32_t v);
float   Value_VREFH1      (int32_t v);
float   Value_VREFL1      (int32_t v);


#ifdef ADC_GLOBAL

T_ADC_res   adcs;

#else

extern T_ADC_res   adcs;

#endif


#endif
