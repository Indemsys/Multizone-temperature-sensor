#define  ADC_GLOBAL
#include "App.h"

#define ADC_PREC      4095.0f
#define VREF          3.3f

// Массив байт для конфигурации регистра ADC->SC1[0]
#pragma data_alignment= 64 // Выравнивание необязательно
const uint8_t  adc0_ch_cfg[ADC_SCAN_SZ] =
{
        // Сигнал        Канал АЦП             Аналоговые функции
        //                                      вывода
  26,   // Temperature Sensor (S.E)
  29,   // VREFH (S.E)
  30,   // VREFL

  // В структуре T_ADC_res этот сигнал идет первым, а здесь должен быть последним !!!
  26,   // Temperature Sensor (S.E)

};


#pragma data_alignment= 64 // Выравнивание необязательно
const uint8_t  adc1_ch_cfg[ADC_SCAN_SZ] =
{
        // Сигнал        Канал АЦП             Аналоговые функции
        //                                      вывода
  26,   // Temperature Sensor (S.E)
  29,   // VREFH (S.E)
  30,   // VREFL

  // В структуре T_ADC_res этот сигнал идет первым, а здесь должен быть последним !!!
  26,   // Temperature Sensor (S.E)
};


#pragma data_alignment= 64 // Массивы результатов выравниваем по границе с пятью младшими битами равными нулю
volatile uint16_t adc0_results[2][ADC_SCAN_SZ];

#pragma data_alignment= 64 // Массивы результатов выравниваем по границе с пятью младшими битами равными нулю
volatile uint16_t adc1_results[2][ADC_SCAN_SZ];


// Массив структур для инициализации каналов DMA обслуживающих 2-а модуля ADC.
// Используем только каналы 16-31 DMA, для упрощения массива структуры, поскольку только к мультиплексору DMAMUX1 подведены источники запросов от всех ADC
// Всего занимаем для нужд ADC 8-мь каналов DMA: 16,17,20,21 и вектор прерывания INT_DMA4_DMA20
const T_init_ADC_DMA_cbl init_ADC_DMA_cbl[2] =
{
  { ADC0_BASE_PTR, adc0_results[0], adc0_ch_cfg, DMA_ADC0_RES_CH, DMA_ADC0_CFG_CH, DMA_ADC0_DMUX_SRC },
  { ADC1_BASE_PTR, adc1_results[0], adc1_ch_cfg, DMA_ADC1_RES_CH, DMA_ADC1_CFG_CH, DMA_ADC1_DMUX_SRC },
};



int32_t Get_TSensor1     (void)         { return (int32_t)adcs.smpl_TSensor1     ;}
int32_t Get_VREFH1       (void)         { return (int32_t)adcs.smpl_VREFH1       ;}
int32_t Get_VREFL1       (void)         { return (int32_t)adcs.smpl_VREFL1       ;}



float   Value_TSensor1    (int32_t v)     { return (25.0f -((v * VREF / ADC_PREC)- 0.719f) / 0.001715f); }
float   Value_VREFH1      (int32_t v)     { return (float)v; }
float   Value_VREFL1      (int32_t v)     { return (float)v; }











