#include "App.h"
#include "freemaster_tsa.h"


extern volatile uint16_t          adc0_results[2][ADC_SCAN_SZ];
extern volatile uint16_t          adc1_results[2][ADC_SCAN_SZ];
extern const T_init_ADC_DMA_cbl   init_ADC_DMA_cbl[2];

extern void FMSTR_Recorder(void);


static T_ADC_state   adc_state;
static uint8_t       event_div_cnt;


static uint32_t res_bank;
void Init_ADC_by_DMA(void);

/*-------------------------------------------------------------------------------------------------------------
 Включаем тактирование всех ADC
-------------------------------------------------------------------------------------------------------------*/
void ADC_switch_on_all(void)
{
  SIM_SCGC3 |= BIT(27); // Тактирование ADC1
  SIM_SCGC6 |= BIT(27); // Тактирование ADC0
}

/*-------------------------------------------------------------------------------------------------------------
  Первоначальная инициализация ADC


  // Самое короткое время преобразования
  // ADLSMP = 0
  // ADICLK = 0
  // 3 ADCK cycles + 5 bus clock cycles

  // 13-и битное дифференциальное
  // 30 ADCK cycles

  // ADLSMP = 0
  // 0 ADCK cycles

  // ADHSC = 0
  // 0 ADCK cycles

  // В сумме: 3 ADCK cycles + 5 bus clock cycles +  30 ADCK cycles + 0 ADCK cycles + 0 ADCK cycles = 33*4 + 5 = 137 тактов шины = 2.28 мкс

  Программируем на 12-и битное преобразование

  ab - Выбор между каналами с суфксами a или b. 0 - a, 1 - b
-------------------------------------------------------------------------------------------------------------*/
void ADC_converter_init(ADC_MemMapPtr ADC, uint32_t ab)
{
  // Выбор частоты тактирования, разрядность результата
  // Частота тактирования для 16-и битного результата не должна быть больше 12 МГц, для 13-и битного 18 МГц
  // Устанавливаем 15 МГц
  ADC->CFG1 = 0
             + LSHIFT(0, 7) // ADLPC.  Low power configuration. The power is reduced at the expense of maximum clock speed. 0 Normal power configuration
             + LSHIFT(2, 5) // ADIV.   Clock divide select. 00 The divide ratio is 1 and the clock rate is input clock.
                            // 10 - The divide ratio is 4 and the clock rate is (input clock)/4. = 7.5 Mhz
             + LSHIFT(1, 4) // ADLSMP. Sample time configuration. 0 Short sample time.
             + LSHIFT(1, 2) // MODE.   Conversion mode selection. 01 When DIFF=0: It is single-ended 12-bit conversion; when DIFF=1, it is differential 13-bit conversion with 2's complement output.
             + LSHIFT(1, 0) // ADICLK. Input clock select. 01 Bus clock divided by 2 = 30 MHz
  ;

  ADC->CFG2 = 0
             + LSHIFT(ab & 1, 4) // MUXSEL.  0 ADxxa channels are selected. Выбор между каналами с суфксами a или b
             + LSHIFT(0, 3) // ADACKEN. Asynchronous clock output enable
             + LSHIFT(0, 2) // ADHSC.   High speed configuration. 0 Normal conversion sequence selected.
             + LSHIFT(1, 0) // ADLSTS.  Default longest sample time (20 extra ADCK cycles; 24 ADCK cycles total). If ADLSMP = 1
  ;

  // Регистр статуса и управления. Выбор типа тригера, управление функцией сравнения, разрешение DMA, выбор типа опоры
  // Выбор источника сигнала  тригера для ADC производится в регистре SIM_SOPT7.
  // Источниками сигнала триггера могут быть PDB 0-3, High speed comparator 0-3, PIT 0-3, FTM 0-3, RTC, LP Timer,

  ADC->SC2 = 0
            + LSHIFT(0, 7) // ADACT.   Read only. 1 Conversion in progress.
            + LSHIFT(1, 6) // ADTRG.   Conversion trigger select. 0 Software trigger selected.
            + LSHIFT(0, 5) // ACFE.    Compare function enable. 0 Compare function disabled.
            + LSHIFT(0, 4) // ACFGT.   Compare function greater than enable
            + LSHIFT(0, 3) // ACREN.   Compare function range enable
            + LSHIFT(1, 2) // DMAEN.   DMA enable
            + LSHIFT(0, 0) // REFSEL.  Voltage reference selection. 00 Default voltage reference pin pair (external pins VREFH and VREFL)
  ;
  // Регистр статуса и управления. Управление и статус калибровки, управление усреднением, управление непрерывным преобразованием
  ADC->SC3 = 0
            + LSHIFT(0, 7) // CAL.     CAL begins the calibration sequence when set.
            + LSHIFT(0, 3) // ADCO.    Continuous conversion enable
            + LSHIFT(0, 2) // AVGE.
            + LSHIFT(0, 0) // AVGS.
  ;


  // Управляющий регистр B
  // Результат управляемый этим регистром будет находиться в регистре ADC->RB
  // Используеться только при включенном аппаратном триггере
  ADC->SC1[1] = 0
               + LSHIFT(0, 7) // COCO. Read only. 1 Conversion completed.
               + LSHIFT(0, 6) // AIEN. 1 Conversion complete interrupt enabled.
               + LSHIFT(0, 5) // DIFF. 1 Differential conversions and input channels are selected.
               + LSHIFT(0, 0) // ADCH. Input channel select. 11111 Module disabled.
                              //                             00000 When DIFF=0, DADP0 is selected as input; when DIFF=1, DAD0 is selected as input.
  ;
  // Управляющий регистр A
  // Результат управляемый этим регистром будет находиться в регистре ADC0->RA
  // Запись в данный регист либо прерывает текущее преобразование либо инициирует новое (если установлен флаг софтварныого тригера - ADTRG = 0)
  ADC->SC1[0] = 0
               + LSHIFT(0, 7) // COCO. Read only. 1 Conversion completed.
               + LSHIFT(0, 6) // AIEN. 1 Conversion complete interrupt enabled.
               + LSHIFT(0, 5) // DIFF. 1 Differential conversions and input channels are selected.
               + LSHIFT(0, 0) // ADCH. Input channel select. 11111 Module disabled.
                              //                             00000 When DIFF=0, DADP0 is selected as input; when DIFF=1, DAD0 is selected as input.
  ;

}

/*-------------------------------------------------------------------------------------------------------------


   Устанавливаем ADICLK = 1, т.е. тактирование от  Bus clock он же Peripheral Clock поделенной на 2 = 30 Mhz

   Во время калибровки рекомендуется установить чатоту тактирования меньшей или равной 4 МГц
    и установить максимальную глубину усреднения
-------------------------------------------------------------------------------------------------------------*/
int32_t ADC_calibrating(ADC_MemMapPtr ADC)
{

  unsigned short tmp;

  ADC->SC1[0] = 0
               + LSHIFT(0, 7) // COCO. Read only. 1 Conversion completed.
               + LSHIFT(0, 6) // AIEN. 1 Conversion complete interrupt enabled.
               + LSHIFT(1, 5) // DIFF. 1 Differential conversions and input channels are selected.
               + LSHIFT(0, 0) // ADCH. Input channel select. 11111 Module disabled.
                              //                             00000 When DIFF=0, DADP0 is selected as input; when DIFF=1, DAD0 is selected as input.
  ;
  // Устанавливаем частоту ADC 4.6875 MHz
  ADC->CFG1 = 0
             + LSHIFT(0, 7) // ADLPC.  Low power configuration. The power is reduced at the expense of maximum clock speed. 0 Normal power configuration
             + LSHIFT(3, 5) // ADIV.   Clock divide select. 11 - The divide ratio is 8 and the clock rate is (input clock)/8. = 4.6875 MHz
             + LSHIFT(0, 4) // ADLSMP. Sample time configuration. 0 Short sample time.
             + LSHIFT(1, 2) // MODE.   Conversion mode selection. 01 When DIFF=0: It is single-ended 12-bit conversion; when DIFF=1, it is differential 13-bit conversion with 2's complement output.
             + LSHIFT(1, 0) // ADICLK. Input clock select. 01 Bus clock divided by 2 = 30 MHz
  ;
  ADC->CFG2 = 0
             + LSHIFT(0, 4) // MUXSEL.  0 ADxxa channels are selected.
             + LSHIFT(0, 3) // ADACKEN. Asynchronous clock output enable
             + LSHIFT(0, 2) // ADHSC.   High speed configuration. 0 Normal conversion sequence selected.
             + LSHIFT(0, 0) // ADLSTS.  Long sample time select
  ;
  // Регистр статуса и управления. Выбор типа тригера, управление функцией сравнения, разрешение DMA, выбор типа опоры
  ADC->SC2 = 0
            + LSHIFT(0, 7) // ADACT.   Read only. 1 Conversion in progress.
            + LSHIFT(0, 6) // ADTRG.   Conversion trigger select. 0 Software trigger selected.
            + LSHIFT(0, 5) // ACFE.    Compare function enable. 0 Compare function disabled.
            + LSHIFT(0, 4) // ACFGT.   Compare function greater than enable
            + LSHIFT(0, 3) // ACREN.   Compare function range enable
            + LSHIFT(0, 2) // DMAEN.   DMA enable
            + LSHIFT(0, 0) // REFSEL.  Voltage reference selection. 00 Default voltage reference pin pair (external pins VREFH and VREFL)
  ;
  ADC->SC3 = 0
            + LSHIFT(1, 7) // CAL.     CAL begins the calibration sequence when set.
            + LSHIFT(1, 6) // CALF.    Read Only. 1 Calibration failed.
            + LSHIFT(0, 3) // ADCO.    Continuous conversion enable
            + LSHIFT(1, 2) // AVGE.    1 Hardware average function enabled.
            + LSHIFT(3, 0) // AVGS.    11 32 samples averaged.
  ;

  // Ожидать завершения калибровки
  while (ADC->SC3 & BIT(7));

  tmp = ADC->CLP0;
  tmp += ADC->CLP1;
  tmp += ADC->CLP2;
  tmp += ADC->CLP3;
  tmp += ADC->CLP4;
  tmp += ADC->CLPS;
  tmp /= 2;
  ADC->PG = tmp | 0x8000;

  tmp =  ADC->CLM0;
  tmp += ADC->CLM1;
  tmp += ADC->CLM2;
  tmp += ADC->CLM3;
  tmp += ADC->CLM4;
  tmp += ADC->CLMS;
  tmp /= 2;
  ADC->MG = tmp | 0x8000;

  if (ADC->SC3 & BIT(6))
  {
    return MQX_ERROR;
  }
  else
  {
    return MQX_OK;
  }
}

/*-------------------------------------------------------------------------------------------------------------
  Включить запуск заданного модуля ADC от аппаратного тригера , программный тригер перестает работать
-------------------------------------------------------------------------------------------------------------*/
void ADC_activate_hrdw_trig(ADC_MemMapPtr ADC)
{
  ADC->SC2 |= BIT(6); // ADTRG.   Conversion trigger select. 0 Software trigger selected.
}

/*-------------------------------------------------------------------------------------------------------------
  Включить запуск заданного модуля ADC от программного тригера , аппаратный тригер перестает работать
-------------------------------------------------------------------------------------------------------------*/
void ADC_activate_soft_trig(ADC_MemMapPtr ADC)
{
  ADC->SC2 &= ~BIT(6); // ADTRG.   Conversion trigger select. 0 Software trigger selected.
}



/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
T_ADC_state* ADC_get_state(void)
{
  return &adc_state;
}


/*-------------------------------------------------------------------------------------------------------------
  Подготовка подсистемы  ADC к работе.
  Калибровка и старт работы всех конвертеров ADCР

  dma_en = 1 указывает на использование DMA при сканировании ADC
-------------------------------------------------------------------------------------------------------------*/
void ADCs_subsystem_init(void)
{
  SIM_MemMapPtr  SIM   = SIM_BASE_PTR;
  //ADC_MemMapPtr  ADC2  = ADC2_BASE_PTR;

  ADC_switch_on_all();
  adc_state.adc0_cal_res = ADC_calibrating(ADC0_BASE_PTR);  // Проводим процедуру калибровки модулей ADC
  adc_state.adc1_cal_res = ADC_calibrating(ADC1_BASE_PTR);  // Проводим процедуру калибровки модулей ADC
  ADC_converter_init(ADC0_BASE_PTR, ADC0_CHAN_SEL);
  ADC_converter_init(ADC1_BASE_PTR, ADC1_CHAN_SEL);

  // Устновка источника сигналов аппаратных триггеров для ADC. Выбираем PIT канал 0
  // Сигнал от PIT может быть тригером только одного из двух управляющих регистров ADC
  // (Сигналы от модуля PDB могут последовательно активировать оба управляющих регистра ADC)
  SIM->SOPT7 = 0
              + LSHIFT(1, 15) // ADC1ALTTRGEN  | ADC alternate trigger enable | 1 Alternate trigger selected for ADC1.
              + LSHIFT(0, 12) // ADC1PRETRGSEL | ADC pre-trigger select       | 0 Pre-trigger A selected for ADC1. Выбор управляющего регистра ADC1 (A или B), который будет активизировать тригер
              + LSHIFT(4,  8) // ADC1TRGSEL    | ADC trigger select           | 0100 PIT trigger 0
              + LSHIFT(1,  7) // ADC0ALTTRGEN  | ADC alternate trigger enable | 1 Alternate trigger selected for ADC0.
              + LSHIFT(0,  4) // ADC0PRETRGSEL | ADC pre-trigger select       | 0 Pre-trigger A selected for ADC0. Выбор управляющего регистра ADC0 (A или B), который будет активизировать тригер
              + LSHIFT(4,  0) // ADC0TRGSEL    | ADC trigger select           | 0100 PIT trigger 0
  ;
  ADC_activate_hrdw_trig(ADC0_BASE_PTR); // Запускаем работу ADC от аппаратного триггера
  ADC_activate_hrdw_trig(ADC1_BASE_PTR); // Запускаем работу ADC от аппаратного триггера


  Init_ADC_by_DMA();
  PIT_init_for_ADC_trig(ADC_PIT_TIMER, ADC_PERIOD, ADC_SCAN_SZ);    // Запускаем работу АЦП по периодическому сигналу
}

/*-------------------------------------------------------------------------------------------------------------
  Копирование из буфера DMA в структуру содержащую результаты ADC для обработки задачами
  Вызывается из процедуры обработки прерываний DMAР
  n - номер модуля ADC
  buf - буффер DMA
-------------------------------------------------------------------------------------------------------------*/
void Copy_to_ADC_res(uint8_t res_bank)
{
  uint32_t addr = (uint32_t)&adcs;
  memcpy((void *)addr,  (uint16_t *)adc0_results[res_bank], ADC_SCAN_SZ * 2);
  addr += ADC_SCAN_SZ * 2;
  memcpy((void *)addr,  (uint16_t *)adc1_results[res_bank], ADC_SCAN_SZ * 2);
}


/*-------------------------------------------------------------------------------------------------------------
  Прерывание после окончания мажорного цикла канала  DMA
  Вызывается с периодом ADC_PERIOD
-------------------------------------------------------------------------------------------------------------*/
static void DMA_ADC_Isr(void *user_isr_ptr)
{
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;
  //ITM_EVENT8(1,0);
  DMA->INT = BIT(DMA_ADC0_RES_CH); // Сбрасываем флаг прерываний  канала

  // В случае приоритета 1 или 2 масимальная длительность не превышает 6.2 мкс
  Copy_to_ADC_res(res_bank);
  res_bank ^= 1;


  if (event_div_cnt>=INPUTS_TASK_PERIOD_DIV)
  {
    Inputs_set_ADC_evt(); //
    event_div_cnt = 0;
  }
  else
  {
    event_div_cnt++;
  }

  if (g_fmstr_rate_src == FMSTR_SMPS_ADC_ISR)
  {
    FMSTR_Recorder(); // Вызываем функцию записи сигнала для инструмента FreeMaster
  }
  //ITM_EVENT8(1,1); //Длительность от предыдущего ITM_EVENT8 = 6.94 мкс без вызова Inputs_set_ADC_evt,  9.72 мкс с вызовом Inputs_set_ADC_evt

}

/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для автоматической выборки из заданных ADC каналов
  По мотивам Document Number: AN4590 "Using DMA to Emulate ADC Flexible Scan Mode on Kinetis K Series"

  Здесь используется механизм связанного вызова минорного цикла одного канала DMA по окончании минорного цикла другого канала DMA


  n     -  номер конфигурационной записи в массиве init_ADC_DMA_cbl
  enint - флаг разрешения прерывания от данной конфигурации
-------------------------------------------------------------------------------------------------------------*/
void Config_DMA_for_ADC(uint8_t n, uint8_t enint)
{
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;
  DMAMUX_MemMapPtr DMAMUX  = DMA_ADC_DMUX_PTR;

  ADC_MemMapPtr      ADC    = init_ADC_DMA_cbl[n].ADC;
  uint8_t            res_ch = init_ADC_DMA_cbl[n].results_channel; // Канал res_ch DMA (более приоритетный) считывает данные из ADC
  uint8_t            cfg_ch = init_ADC_DMA_cbl[n].config_channel;  // Канал cfg_ch DMA (менее приоритетный) загружает новым значением регистр ADC->SC1[0]


  // Конфигурируем первую выборку из ADC по корректному номеру канала согласно заданной конфигурации
  ADC->SC1[0] = init_ADC_DMA_cbl[n].config_vals[ ADC_SCAN_SZ - 1 ];

  // Конфигурируем более приоритетный канал пересылки данных из ADC
  DMA->TCD[res_ch].SADDR = (uint32_t)&ADC->R[0];      // Источник - регистр данных ADC
  DMA->TCD[res_ch].SOFF = 0;                        // Адрес источника не изменяется после чтения
  DMA->TCD[res_ch].SLAST = 0;                       // Не корректируем адрес источника после завершения всего цикла DMA (окончания мажорного цикла)
  DMA->TCD[res_ch].DADDR = (uint32_t)init_ADC_DMA_cbl[n].results_vals; // Адрес приемника данных из ADC - массив отсчетов
  DMA->TCD[res_ch].DOFF = 2;                        // После каждой записи смещаем указатель приемника на два байта
  DMA->TCD[res_ch].DLAST_SGA = 0;                   // Здесь не корректируем адреса приемника после мажорного цикла. Для коррекции используем поле DMOD в регистре ATTR
  // DMA->TCD[res_ch].DLAST_SGA = (uint32_t)(-ADC_SCAN_SZ*2);  // Здесь корректируем адреса приемника после мажорного цикла.
  DMA->TCD[res_ch].NBYTES_MLNO = 2;                 // Количество байт пересылаемых за один запрос DMA (в минорном цикле)
  DMA->TCD[res_ch].BITER_ELINKNO = 0
                                  + LSHIFT(1, 15)           // ELINK  | Включаем линковку к минорному циклу другого канала
                                  + LSHIFT(cfg_ch, 9)       // LINKCH | Линкуемся к каналу cfg_ch
                                  + LSHIFT(ADC_SCAN_SZ, 0)  // BITER  | Количество итераций в мажорном цикле
  ;
  DMA->TCD[res_ch].CITER_ELINKNO = 0
                                  + LSHIFT(1, 15)           // ELINK  | Включаем линковку к минорному циклу другого канала
                                  + LSHIFT(cfg_ch, 9)       // LINKCH | Линкуемся к каналу cfg_ch
                                  + LSHIFT(ADC_SCAN_SZ, 0)  // BITER  | Количество итераций в мажорном цикле
  ;
  DMA->TCD[res_ch].ATTR = 0
                         + LSHIFT(0, 11) // SMOD  | Модуль адреса источника не используем
                         + LSHIFT(1, 8)  // SSIZE | 16-и битная пересылка из источника
                         + LSHIFT(DEST_MODULO, 3)  // DMOD  | Модуль адреса приемника
                         //  + LSHIFT(0, 3)  // DMOD  | Модуль адреса приемника
                         + LSHIFT(1, 0)  // DSIZE | 16-и битная пересылка в приемник
  ;
  DMA->TCD[res_ch].CSR = 0
                        + LSHIFT(3, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
                        + LSHIFT(cfg_ch, 8)  // MAJORLINKCH | Поскольку минорный линк не срабатывает на последней минорной пересылке, то линк надо настроить и для мажорного цикла
                        + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
                        + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
                        + LSHIFT(1, 5)  // MAJORELINK  | Линкуемся к каналу cfg_ch после завершения мажорного цикла
                        + LSHIFT(0, 4)  // ESG         | Enable Scatter/Gather Processing
                        + LSHIFT(0, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
                        + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
                        + LSHIFT(enint, 1)  // INTMAJOR    | Разрешаем прерывание после мажорного цикла
                        + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;

  // Конфигурируем менее приоритетный канала пересылки настройки в ADC
  DMA->TCD[cfg_ch].SADDR = (uint32_t)init_ADC_DMA_cbl[n].config_vals;     // Источник - массив настроек для ADC
  DMA->TCD[cfg_ch].SOFF = 1;                        // Адрес источника после прочтения смещается на 1 байт вперед
  DMA->TCD[cfg_ch].SLAST = (uint32_t)(-ADC_SCAN_SZ);            // Корректируем адрес источника после завершения всего цикла DMA (окончания мажорного цикла)
  DMA->TCD[cfg_ch].DADDR = (uint32_t)&ADC->SC1[0];  // Адрес приемника данных - регистр управления ADC
  DMA->TCD[cfg_ch].DOFF = 0;                        // Указатель приемника неменяем после записи
  DMA->TCD[cfg_ch].DLAST_SGA = 0;                   // Коррекцию адреса приемника не производим после окончания всей цепочки сканирования (окончания мажорного цикла)
  DMA->TCD[cfg_ch].NBYTES_MLNO = 1;                 // Количество байт пересылаемых за один запрос DMA (в минорном цикле)
  DMA->TCD[cfg_ch].BITER_ELINKNO = 0
                                  + LSHIFT(0, 15)           // ELINK  | Линковка выключена
                                  + LSHIFT(0, 9)            // LINKCH |
                                  + LSHIFT(ADC_SCAN_SZ, 0)  // BITER  | Количество итераций в мажорном цикле
  ;
  DMA->TCD[cfg_ch].CITER_ELINKNO = 0
                                  + LSHIFT(0, 15)           // ELINK  | Линковка выключена
                                  + LSHIFT(0, 9)            // LINKCH |
                                  + LSHIFT(ADC_SCAN_SZ, 0)  // BITER  | Количество итераций в мажорном цикле
  ;
  DMA->TCD[cfg_ch].ATTR = 0
                         + LSHIFT(0, 11) // SMOD  | Модуль адреса источника не используем
                         + LSHIFT(0, 8)  // SSIZE | 8-и битная пересылка из источника
                         + LSHIFT(0, 3)  // DMOD  | Модуль адреса приемника не используем
                         + LSHIFT(0, 0)  // DSIZE | 8-и битная пересылка в приемник
  ;
  DMA->TCD[cfg_ch].CSR = 0
                        + LSHIFT(3, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
                        + LSHIFT(0, 8)  // MAJORLINKCH | Link Channel Number
                        + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
                        + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
                        + LSHIFT(0, 5)  // MAJORELINK  | Линкуемся к каналу 0  после завершения мажорного цикла
                        + LSHIFT(0, 4)  // ESG         | Enable Scatter/Gather Processing
                        + LSHIFT(0, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
                        + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
                        + LSHIFT(0, 1)  // INTMAJOR    | Прерывания не используем
                        + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;
  DMAMUX->CHCFG[res_ch] = init_ADC_DMA_cbl[n].req_src + BIT(7); // Через мультиплексор связываем сигнал от внешней периферии (здесь от канала ADC) с входом выбранного канала DMA
                                                                // BIT(7) означает DMA Channel Enable
  DMA->SERQ = res_ch;                                           // Разрешаем запросы от внешней периферии для канала DMA с номером res_ch
}


/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для автоматической выборки из заданных ADC каналов
-------------------------------------------------------------------------------------------------------------*/
void Init_ADC_by_DMA(void)
{

  Install_and_enable_isr(DMA_ADC_INT_NUM, DMA_ADC_PRIO, DMA_ADC_Isr);
  Config_DMA_for_ADC(1, 0);
  Config_DMA_for_ADC(0, 1);
  // Назначаем прерывание только от одного канала DMA
  // Поскольку он самый низкоприоритетный, то должен сработать позже всех когда все данные из всех ADC уже будут в памяти
}

