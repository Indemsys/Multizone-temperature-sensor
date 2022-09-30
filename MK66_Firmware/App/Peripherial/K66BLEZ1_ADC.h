#ifndef __K66BLEZ_ADC
  #define __K66BLEZ_ADC



#define ADC_AVER_4    1
#define ADC_AVER_8    2
#define ADC_AVER_16   3
#define ADC_AVER_32   4


typedef struct
{
  // Результаты калибровки модулей ADC
  int32_t adc0_cal_res;
  int32_t adc1_cal_res;
} T_ADC_state;


typedef struct
{
  const char   *name;
  float        (*int_converter)(int);
  float        (*flt_converter)(float);

} T_vals_scaling;

typedef struct
{
  ADC_MemMapPtr ADC;                           //  Указатель на модуль ADC
  unsigned short volatile     *results_vals;   //  Указатель на буффер с результатами ADC
  unsigned char const         *config_vals;    //  Указатель на массив констант содержащих конфигурацию ADC
  uint8_t                       results_channel; //  Номер канала DMA пересылающий резульат ADC.     Более приоритетный. По умолчанию канал с большим номером имеет больший приоритет.
  uint8_t                       config_channel;  //  Номер канала DMA пересылающий конфигурацию ADC. Менее приоритетный
  uint8_t                       req_src;         //  Номер источника запросов для конфигурирования мультиплексора DMAMUX
} T_init_ADC_DMA_cbl;


void ADCs_subsystem_init(void);
void ADC_switch_on_all(void);
void ADC_converter_init(ADC_MemMapPtr ADC, uint32_t ab);
void ADC_activate_hrdw_trig(ADC_MemMapPtr ADC);
void ADC_activate_soft_trig(ADC_MemMapPtr ADC);
int  ADC_calibrating(ADC_MemMapPtr ADC);


T_ADC_state *ADC_get_state(void);


void Copy_to_ADC_res(uint8_t res_bank);

#endif
