#include "App.h"

static volatile uint8_t   dummy_tx = 0xFF;
static volatile uint8_t   dummy_rx;

T_DMA_cbl SPI0_DS_cbl;
/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
void Init_DMA(void)
{
  unsigned int  i;
  DMA_MemMapPtr DMA = DMA_BASE_PTR;
  SIM_MemMapPtr SIM = SIM_BASE_PTR;


  SIM->SCGC6 |= 0
                + LSHIFT(1, 1) // DMAMUX. DMAMUX clock gate control. 1 Clock is enabled.
  ;

  SIM->SCGC7 |= 0
                + LSHIFT(1, 1) // DMA. DMA controller clock gate control. 1 Clock is enabled.
  ;

  // Прерываем все операции DMA если они производились в данный момент и останавливаем все последующие операции
  DMA->CR = 0
            + LSHIFT(1, 17) // CX      | Cancel Transfer. 1 Cancel the remaining data transfer
            + LSHIFT(0, 16) // ECX     | Error Cancel Transfer. 1 Cancel the remaining data transfer in the same fashion as the CX bit.
            + LSHIFT(1, 10) // GRP1PRI | Channel Group 1 Priority. Group 1 priority level when fixed priority group arbitration is enabled
            + LSHIFT(0, 8)  // GRP0PRI | Channel Group 0 Priority. Group 0 priority level when fixed priority group arbitration is enabled.
            + LSHIFT(0, 7)  // EMLM    | Enable Minor Loop Mapping. 1 Enabled. TCDn.word2 is redefined to include individual enable fields, an offset field, and the NBYTES field.
            + LSHIFT(0, 6)  // CLM     | Continuous Link Mode. 1 A minor loop channel link made to itself does not go through channel arbitration before being activated again.
            + LSHIFT(1, 5)  // HALT    | Halt DMA Operations. 1 Stall the start of any new channels
            + LSHIFT(0, 4)  // HOE     | Halt On Error. 1 Any error causes the HALT bit to set.
            + LSHIFT(0, 3)  // ERGA    | Enable Round Robin Group Arbitration. 1 Round robin arbitration is used for selection among the groups.
            + LSHIFT(0, 2)  // ERCA    | Enable Round Robin Channel Arbitration. 1 Round robin arbitration is used for channel selection within each group.
            + LSHIFT(0, 1)  // EDBG    | Enable Debug. 1 When in debug mode, the DMA stalls the start of a new channel.
  ;
  while (DMA->CR & BIT(17));  // Ожидаем сброса флага CX
  DMA->CR = BIT(17) + BIT(10) + BIT(5); // Повторяем прерывание и остановку операций на случай наличия вложенных операций
  while (DMA->CR & BIT(17));  // Ожидаем сброса флага CX

  DMA->EEI = 0; // Запрещаем прерывания на всех каналах в случае ошибок
  DMA->CINT = BIT(6); // CAIR. Clear All Interrupt Requests. 1 Clear all bits in INT
  DMA->CERR = BIT(6); // CAEI. Clear All Error Indicators. Clear all bits in ERR
  DMA->CERQ = BIT(6); // CAER. Clear All Enable Requests. 1 Clear all bits in ERQ

/*
  В каждой группе по 16 каналов канал 0 имеет низший приоритет, канал 15 - высший
  DMA->DCHPRI0 = 0
                 + LSHIFT(1, 7) // ECP    | Enable Channel Preemption.
                 + LSHIFT(0, 6) // DPA    | Disable Preempt Ability.
                 + LSHIFT(0, 4) // GRPPRI | Channel n Current Group Priority (read only)
                 + LSHIFT(0, 4) // CHPRI  | Channel n Arbitration Priority
  ;
*/
  // Изменяем приоритеты в попытку решить проблему работы канала RGB светодиода через таймер FTM0
  DMA->DCHPRI0  = 6 ;  // Повышаем приоритет и запрещаем вытеснение этого канала
  DMA->DCHPRI1  = 7 ;  // Повышаем приоритет и запрещаем вытеснение этого канала
  DMA->DCHPRI2  = 2 ;
  DMA->DCHPRI3  = 3 ;
  DMA->DCHPRI4  = 4 ;
  DMA->DCHPRI5  = 5 ;
  DMA->DCHPRI6  = 0 + BIT(6) + BIT(7);  // Понижаем приоритет этого канала,разрешаем его вытеснять и не разрешаем ему вытеснять
  DMA->DCHPRI7  = 1 + BIT(6) + BIT(7);  // Понижаем приоритет этого канала,разрешаем его вытеснять и не разрешаем ему вытеснять
  DMA->DCHPRI8  = 8 ;
  DMA->DCHPRI9  = 9 ;
  DMA->DCHPRI10 = 10;
  DMA->DCHPRI11 = 11;
  DMA->DCHPRI12 = 12;
  DMA->DCHPRI13 = 13;
  DMA->DCHPRI14 = 14;
  DMA->DCHPRI15 = 15;
  DMA->DCHPRI16 = 16;
  DMA->DCHPRI17 = 17;
  DMA->DCHPRI18 = 18;
  DMA->DCHPRI19 = 19;
  DMA->DCHPRI20 = 20;
  DMA->DCHPRI21 = 21;
  DMA->DCHPRI22 = 22;
  DMA->DCHPRI23 = 23;
  DMA->DCHPRI24 = 24;
  DMA->DCHPRI25 = 25;
  DMA->DCHPRI26 = 26;
  DMA->DCHPRI27 = 27;
  DMA->DCHPRI28 = 28;
  DMA->DCHPRI29 = 29;
  DMA->DCHPRI30 = 30;
  DMA->DCHPRI31 = 31;

  // Устанвливаем Round Robin Group Arbitration и Fixed priority arbitration is used for channel selection
  // А также Enable Minor Loop Mapping
  DMA->CR = 0
            + LSHIFT(0, 17) // CX      | Cancel Transfer. 1 Cancel the remaining data transfer
            + LSHIFT(0, 16) // ECX     | Error Cancel Transfer. 1 Cancel the remaining data transfer in the same fashion as the CX bit.
            + LSHIFT(1, 10) // GRP1PRI | Channel Group 1 Priority. Group 1 priority level when fixed priority group arbitration is enabled
            + LSHIFT(0, 8)  // GRP0PRI | Channel Group 0 Priority. Group 0 priority level when fixed priority group arbitration is enabled.
            + LSHIFT(0, 7)  // EMLM    | Enable Minor Loop Mapping. 1 Enabled. TCDn.word2 is redefined to include individual enable fields, an offset field, and the NBYTES field.
            + LSHIFT(0, 6)  // CLM     | Continuous Link Mode. 1 A minor loop channel link made to itself does not go through channel arbitration before being activated again.
            + LSHIFT(0, 5)  // HALT    | Halt DMA Operations. 1 Stall the start of any new channels
            + LSHIFT(0, 4)  // HOE     | Halt On Error. 1 Any error causes the HALT bit to set.
            + LSHIFT(0, 3)  // ERGA    | Enable Round Robin Group Arbitration. 1 Round robin arbitration is used for selection among the groups.
            + LSHIFT(0, 2)  // ERCA    | Enable Round Robin Channel Arbitration. 1 Round robin arbitration is used for channel selection within each group.
            + LSHIFT(0, 1)  // EDBG    | Enable Debug. 1 When in debug mode, the DMA stalls the start of a new channel.
  ;

  for (i = 0; i < DMA_CHANNELS_COUNT; i++)
  {
    DMA->TCD[i].SADDR = 0;          // Memory address pointing to the source data
    DMA->TCD[i].SOFF = 0;           // Sign-extended offset applied to the current source address to form the next-state value as each source read is completed.
    DMA->TCD[i].ATTR = 0
                       + LSHIFT(0, 11) // SMOD  | Source Address Modulo
                       + LSHIFT(0, 8)  // SSIZE | Source data transfer size. 000 8-bit,001 16-bit,010 32-bit,011 Reserved,100 16-byte burst,101 32-byte burst
                       + LSHIFT(0, 3)  // DMOD  | Destination Address Modulo
                       + LSHIFT(0, 0)  // DSIZE | Destination data transfer size
    ;
    DMA->TCD[i].NBYTES_MLNO = 0    // Minor Byte Transfer Count
                              + LSHIFT(0, 31) // SMLOE  | Source Minor Loop Offset Enable
                              + LSHIFT(0, 30) // DMLOE  | Destination Minor Loop Offset enable
                              + LSHIFT(0, 10) // MLOFF  | If SMLOE or DMLOE is set, this field represents a sign-extended offset applied to the source or destination address to form the next-state value after the minor loop completes.
                              + LSHIFT(0, 0)  // NBYTES | Minor Byte Transfer Count
    ;

    DMA->TCD[i].SLAST = 0;          // Last Source Address Adjustment. Adjustment value added to the source address at the completion of the major iteration count
    DMA->TCD[i].DADDR = 0;          // Memory address pointing to the destination data.
    DMA->TCD[i].DOFF = 0;           // Sign-extended offset applied to the current destination address to form the next-state value as each destination write is completed.
    DMA->TCD[i].CITER_ELINKNO = 0
                                + LSHIFT(0, 15) // ELINK  | Enable channel-to-channel linking on minor-loop complete
                                + LSHIFT(0, 9)  // LINKCH | Link Channel Number
                                + LSHIFT(0, 0)  // CITER  | Current Major Iteration Count
    ;
    DMA->TCD[i].DLAST_SGA = 0;      // Destination last address adjustment or the memory address for the next transfer control descriptor to be loaded into this channel (scatter/gather).
    DMA->TCD[i].CSR = 0
                      + LSHIFT(0, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
                      + LSHIFT(0, 8)  // MAJORLINKCH | Link Channel Number
                      + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
                      + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
                      + LSHIFT(0, 5)  // MAJORELINK  | Enable channel-to-channel linking on major loop complete
                      + LSHIFT(0, 4)  // ESG         | Enable Scatter/Gather Processing
                      + LSHIFT(0, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
                      + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
                      + LSHIFT(0, 1)  // INTMAJOR    | Enable an interrupt when major iteration count completes
                      + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
    ;

    DMA->TCD[i].BITER_ELINKNO = 0
                                + LSHIFT(0, 15) // ELINK  | Enable channel-to-channel linking on minor-loop complete
                                + LSHIFT(0, 9)  // LINKCH | Link Channel Number
                                + LSHIFT(0, 0)  // BITER  | Starting major iteration count
    ;
  }
}

/*-----------------------------------------------------------------------------------------------------

 \param rt_size

 \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint8_t DMA_Get_attr_size(uint8_t rt_size)
{
  switch (rt_size)
  {
  case 1:
    return 0;
  case 2:
    return 1;
  case 4:
    return 2;
  default:
    return 0;
  }
}


/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для передачи данных в регистр периферийного модуля
  Аттрибуты процесса передачи задаются в структуре cfg
  Структура pDS_cbl заполняется для последующего использования функциями передачи
-------------------------------------------------------------------------------------------------------------*/
void Config_DMA_TX(T_DMA_config *cfg, T_DMA_cbl *pDS_cbl)
{
  uint8_t   k;
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  pDS_cbl->minor_tranf_sz = cfg->minor_tranf_sz;


  DMA->TCD[cfg->dma_channel].SADDR = 0;                         // Источник - буфер с данными (На фазе конфигурации не устанавливаем!!!)
  DMA->TCD[cfg->dma_channel].SOFF = cfg->minor_tranf_sz;        // Адрес источника смещаем  после каждой передачи
  DMA->TCD[cfg->dma_channel].SLAST = 0;                         // Не корректируем адрес источника после завершения всего цикла DMA (окончания мажорного цикла)
  DMA->TCD[cfg->dma_channel].DADDR = (uint32_t)cfg->periph_port;  // Адрес приемника - регистр PUSHR SPI
  DMA->TCD[cfg->dma_channel].DOFF = 0;                          // После  записи указатель приемника не смещаем
  DMA->TCD[cfg->dma_channel].DLAST_SGA = 0;                     // Цепочки дескрипторов не применяем
  DMA->TCD[cfg->dma_channel].NBYTES_MLNO = cfg->minor_tranf_sz; // Количество байт пересылаемых за один запрос DMA (в минорном цикле)
  DMA->TCD[cfg->dma_channel].BITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | Линковку не применяем
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  | (На фазе конфигурации не устанавливаем!!!)
  ;
  DMA->TCD[cfg->dma_channel].CITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | Линковку не применяем
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  | (На фазе конфигурации не устанавливаем!!!)
  ;
  DMA->TCD[cfg->dma_channel].ATTR = 0
    + LSHIFT(0, 11) // SMOD  | Модуль адреса источника не используем
    + LSHIFT(DMA_Get_attr_size(cfg->minor_tranf_sz), 8)  // SSIZE | Задаем количество бит передаваемых за одну операцию чтения источника
    + LSHIFT(0, 3)  // DMOD  | Модуль адреса приемника
    + LSHIFT(DMA_Get_attr_size(cfg->minor_tranf_sz), 0)  // DSIZE | Задаем количество бит передаваемых за одну операцию записи в приемник
  ;
  DMA->TCD[cfg->dma_channel].CSR = 0
    + LSHIFT(0, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
    + LSHIFT(0, 8)  // MAJORLINKCH | Линковку не применяем
    + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
    + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
    + LSHIFT(0, 5)  // MAJORELINK  | Линковку не применяем
    + LSHIFT(0, 4)  // ESG         | Цепочки дескрипторов не применяем
    + LSHIFT(1, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
    + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
    + LSHIFT(cfg->enable_interrupt & 1, 1)  // INTMAJOR    | Устанавливаем прерывание по окнчании пересылки DMA
    + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;

  pDS_cbl->dma_tx_channel  = cfg->dma_channel;
  {
    uint8_t indx = cfg->dma_channel;
    cfg->DMAMUX->CHCFG[indx] = cfg->dmux_src + BIT(7); // Через мультиплексор связываем сигнал от внешней периферии с входом выбранного канала DMA
  }

}


/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для приема данных из регистра периферийного модуля
  Аттрибуты процесса передачи задаются в структуре cfg
  Структура pDS_cbl заполняется для последующего использования функциями передачи
-------------------------------------------------------------------------------------------------------------*/
void Config_DMA_RX(T_DMA_config *cfg, T_DMA_cbl *pDS_cbl)
{
  uint8_t   k;
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  pDS_cbl->minor_tranf_sz = cfg->minor_tranf_sz;


  DMA->TCD[cfg->dma_channel].SADDR = (uint32_t)cfg->periph_port; // Источник - FIFO приемника SPI
  DMA->TCD[cfg->dma_channel].SOFF  = 0;                          // Адрес источника не изменяем
  DMA->TCD[cfg->dma_channel].SLAST = 0;                          // Не корректируем адрес источника после завершения всего цикла DMA (окончания мажорного цикла)
  DMA->TCD[cfg->dma_channel].DADDR = 0;                          // Адрес приемника - буфер данных. На фазе конфигурации не устанавливаем
  DMA->TCD[cfg->dma_channel].DOFF  = cfg->minor_tranf_sz;        // После  записи указатель приемника смещаем
  DMA->TCD[cfg->dma_channel].DLAST_SGA = 0;                      // Цепочки дескрипторов не применяем
  DMA->TCD[cfg->dma_channel].NBYTES_MLNO = cfg->minor_tranf_sz;  // Количество байт пересылаемых за один запрос DMA (в минорном цикле)
  DMA->TCD[cfg->dma_channel].BITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | Линковку не применяем
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  |
  ;
  DMA->TCD[cfg->dma_channel].CITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | Линковку не применяем
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  |
  ;
  DMA->TCD[cfg->dma_channel].ATTR = 0
    + LSHIFT(0, 11) // SMOD  | Модуль адреса источника не используем
    + LSHIFT(DMA_Get_attr_size(cfg->minor_tranf_sz), 8)  // SSIZE | | Задаем количество бит передаваемых за одну операцию чтения источника
    + LSHIFT(0, 3)  // DMOD  | Модуль адреса приемника
    + LSHIFT(DMA_Get_attr_size(cfg->minor_tranf_sz), 0)  // DSIZE | | Задаем количество бит передаваемых за одну операцию записи в приемник
  ;
  DMA->TCD[cfg->dma_channel].CSR = 0
    + LSHIFT(0, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
    + LSHIFT(0, 8)  // MAJORLINKCH | Линковку не применяем
    + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
    + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
    + LSHIFT(0, 5)  // MAJORELINK  | Линковку не применяем
    + LSHIFT(0, 4)  // ESG         | Цепочки дескрипторов не применяем
    + LSHIFT(1, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
    + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
    + LSHIFT(cfg->enable_interrupt & 1, 1)  // INTMAJOR    | Устанавливаем прерывание по окнчании пересылки DMA
    + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;

  pDS_cbl->dma_rx_channel  = cfg->dma_channel;
  {
    uint8_t indx = cfg->dma_channel;
    cfg->DMAMUX->CHCFG[indx] = cfg->dmux_src + BIT(7); // Через мультиплексор связываем сигнал от внешней периферии (здесь от канала SPI) с входом выбранного канала DMA
  }

}

/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для отправки и приема данных по SPI
  Данные передаются 1-о, 2-х или 4-х байтами словами
  sz - количество передаваемых слов (Это не размер буфера в байтах!)

  Может быть передано не более 2048 слов (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
void Start_DMA_RXTX(T_DMA_cbl *pDS_cbl, void *outbuf, void *inbuf, uint32_t sz)
{
  DMA_MemMapPtr        DMA     = DMA_BASE_PTR;
  T_DMA_TCD  volatile *pTDCmf;
  T_DMA_TCD  volatile *pTDCfm;

  // Подготовка дескриптора DMA предназначенного для чтения из SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_rx_channel;

  // Программируем дескриптор  чтения
  pTDCfm = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_rx_channel];
  pTDCfm->DADDR         = (uint32_t)inbuf;
  pTDCfm->DOFF          = pDS_cbl->minor_tranf_sz;
  pTDCfm->BITER_ELINKNO = (pTDCfm->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCfm->CITER_ELINKNO = pTDCfm->BITER_ELINKNO;

  // Подготовка дескриптора DMA предназначенных для записи буфера в SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_tx_channel;

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_tx_channel];
  pTDCmf->SADDR         = (uint32_t)outbuf;
  pTDCmf->SOFF          = pDS_cbl->minor_tranf_sz;
  pTDCmf->BITER_ELINKNO = (pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;

  DMA->SERQ = pDS_cbl->dma_rx_channel;  // Разрешаем поступление запроса DMA на канал  dma_rx_channel
  DMA->SERQ = pDS_cbl->dma_tx_channel;  // Разрешаем поступление запроса DMA на канал  dma_tx_channel
}


/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для отправки данных по SPI

  Данные передаются 1-о, 2-х или 4-х байтами словами
  sz - количество передаваемых слов (Это не размер буфера в байтах!)

  Может быть передано не более 2048 слов (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
void Start_DMA_for_SPI_TX(T_DMA_cbl *pDS_cbl, void *buf, uint32_t sz)
{
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  T_DMA_TCD  volatile *pTDCmf;
  T_DMA_TCD  volatile *pTDCfm;

  // Подготовка дескриптора DMA предназначенного для пустого чтения из SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_rx_channel;

  // Программируем дескриптор пустого чтения
  pTDCfm = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_rx_channel];
  pTDCfm->DADDR         = (uint32_t)&dummy_rx;
  pTDCfm->DOFF          = 0;
  pTDCfm->BITER_ELINKNO = (pTDCfm->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCfm->CITER_ELINKNO = pTDCfm->BITER_ELINKNO;

  // Подготовка дескриптора DMA предназначенных для записи буфера в SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_tx_channel;

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_tx_channel];
  pTDCmf->SADDR         = (uint32_t)buf;
  pTDCmf->SOFF          = pDS_cbl->minor_tranf_sz;
  pTDCmf->BITER_ELINKNO = (pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;

  DMA->SERQ = pDS_cbl->dma_rx_channel; // Первым активизируем чтение в рассчете, что оно не стартует сразу же поскольку FIFO приемника пусто
  DMA->SERQ = pDS_cbl->dma_tx_channel;
}

/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для приема данных из SPI
  Данные передаются 1-о, 2-х или 4-х байтами словами
  sz - количество передаваемых слов (Это не размер буфера в байтах!)

  Может быть передано не более 2048 слов (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
void Start_DMA_for_SPI_RX(T_DMA_cbl *pDS_cbl, void *buf, uint32_t sz)
{
  DMA_MemMapPtr        DMA     = DMA_BASE_PTR;
  T_DMA_TCD  volatile *pTDCmf;
  T_DMA_TCD  volatile *pTDCfm;

  // Подготовка дескриптора DMA предназначенного для чтения из SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_rx_channel;

  // Программируем дескрипторы с параметрами нового буффера с данными
  pTDCfm = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_rx_channel];
  pTDCfm->DADDR         = (uint32_t)buf;
  pTDCfm->DOFF          = pDS_cbl->minor_tranf_sz;
  pTDCfm->BITER_ELINKNO = (pTDCfm->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCfm->CITER_ELINKNO = pTDCfm->BITER_ELINKNO;


  // Подготовка дескриптора DMA предназначенных для пустой записи в SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_tx_channel;

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_tx_channel];
  pTDCmf->SADDR         = (uint32_t)&dummy_tx;
  pTDCmf->SOFF          = 0;
  pTDCmf->BITER_ELINKNO = (pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;


  DMA->SERQ = pDS_cbl->dma_rx_channel; // Первым активизируем чтение в рассчете, что оно не стартует сразу же поскольку FIFO приемника пусто
  DMA->SERQ = pDS_cbl->dma_tx_channel;
}


/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для отправки и приема данных по SPI
  Данные передаются 1-о, 2-х или 4-х байтами словами
  sz - количество передаваемых слов (Это не размер буфера в байтах!)

  Может быть передано не более 2048 слов (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
void Start_DMA_for_SPI_RXTX(T_DMA_cbl *pDS_cbl, void *outbuf, void *inbuf, uint32_t sz)
{
  DMA_MemMapPtr        DMA     = DMA_BASE_PTR;
  T_DMA_TCD  volatile *pTDCmf;
  T_DMA_TCD  volatile *pTDCfm;

  // Подготовка дескриптора DMA предназначенного для чтения из SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_rx_channel;

  // Программируем дескриптор  чтения
  pTDCfm = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_rx_channel];
  pTDCfm->DADDR         = (uint32_t)inbuf;
  pTDCfm->DOFF          = pDS_cbl->minor_tranf_sz;
  pTDCfm->BITER_ELINKNO = (pTDCfm->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCfm->CITER_ELINKNO = pTDCfm->BITER_ELINKNO;

  // Подготовка дескриптора DMA предназначенных для записи буфера в SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_tx_channel;

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_tx_channel];
  pTDCmf->SADDR         = (uint32_t)outbuf;
  pTDCmf->SOFF          = pDS_cbl->minor_tranf_sz;
  pTDCmf->BITER_ELINKNO = (pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;

  DMA->SERQ = pDS_cbl->dma_rx_channel; // Первым активизируем чтение в рассчете, что оно не стартует сразу же поскольку FIFO приемника пусто
  DMA->SERQ = pDS_cbl->dma_tx_channel;
}

/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для передачи данных из SPI в контроллер дисплея ST7789VW по 4-х проводной шине (SPI + D/C)
  Аттрибуты процесса передачи задаются в структуре cfg
  Структура pDS_cbl заполняется для последующего использования функциями передачи
-------------------------------------------------------------------------------------------------------------*/
void Config_DMA_TFT_TX(T_DMA_LCD_TX_config *cfg, T_DMA_cbl *pDS_cbl)
{
  uint8_t   k;
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  pDS_cbl->minor_tranf_sz = cfg->minor_tranf_sz;


  DMA->TCD[cfg->ch].SADDR = 0;                         // Источник - буфер с данными (На фазе конфигурации не устанавливаем!!!)
  DMA->TCD[cfg->ch].SOFF = cfg->minor_tranf_sz;        // Адрес источника смещаем  после каждой передачи
  DMA->TCD[cfg->ch].SLAST = 0;                         // Не корректируем адрес источника после завершения всего цикла DMA (окончания мажорного цикла)
  DMA->TCD[cfg->ch].DADDR = (uint32_t)cfg->spi_pushr;  // Адрес приемника - регистр PUSHR SPI
  DMA->TCD[cfg->ch].DOFF = 0;                          // После  записи указатель приемника не смещаем
  DMA->TCD[cfg->ch].DLAST_SGA = 0;                     // Цепочки дескрипторов не применяем
  DMA->TCD[cfg->ch].NBYTES_MLNO = cfg->minor_tranf_sz; // Количество байт пересылаемых за один запрос DMA (в минорном цикле)
  DMA->TCD[cfg->ch].BITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | Линковку не применяем
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  | (На фазе конфигурации не устанавливаем!!!)
  ;
  DMA->TCD[cfg->ch].CITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | Линковку не применяем
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  | (На фазе конфигурации не устанавливаем!!!)
  ;
  DMA->TCD[cfg->ch].ATTR = 0
    + LSHIFT(0, 11) // SMOD  | Модуль адреса источника не используем
    + LSHIFT(DMA_Get_attr_size(cfg->minor_tranf_unit_sz), 8)  // SSIZE | Задаем количество бит передаваемых за одну операцию чтения источника
    + LSHIFT(0, 3)  // DMOD  | Модуль адреса приемника
    + LSHIFT(DMA_Get_attr_size(cfg->minor_tranf_unit_sz), 0)  // DSIZE | Задаем количество бит передаваемых за одну операцию записи в приемник
  ;
  DMA->TCD[cfg->ch].CSR = 0
    + LSHIFT(0, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
    + LSHIFT(0, 8)  // MAJORLINKCH | Линковку не применяем
    + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
    + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
    + LSHIFT(0, 5)  // MAJORELINK  | Линковку не применяем
    + LSHIFT(0, 4)  // ESG         | Цепочки дескрипторов не применяем
    + LSHIFT(1, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
    + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
    + LSHIFT(0, 1)  // INTMAJOR    | Не используем прерывание по окнчании пересылки DMA
    + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;

  pDS_cbl->dma_tx_channel  = cfg->ch;
  {
    uint8_t indx = cfg->ch;
    cfg->DMAMUX->CHCFG[indx] = cfg->dmux_src + BIT(7); // Через мультиплексор связываем сигнал от внешней периферии (здесь от канала SPI) с входом выбранного канала DMA
  }

}


/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для приема данных из SPI
  Аттрибуты процесса передачи задаются в структуре cfg
  Структура pDS_cbl заполняется для последующего использования функциями передачи
-------------------------------------------------------------------------------------------------------------*/
void Config_DMA_TFT_RX(T_DMA_LCD_RX_config *cfg, T_DMA_cbl *pDS_cbl)
{
  uint8_t   k;
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  pDS_cbl->minor_tranf_sz = cfg->minor_tranf_sz;


  DMA->TCD[cfg->ch].SADDR = (uint32_t)cfg->spi_popr;   // Источник - FIFO приемника SPI
  DMA->TCD[cfg->ch].SOFF = 0;                          // Адрес источника не изменяем
  DMA->TCD[cfg->ch].SLAST = 0;                         // Не корректируем адрес источника после завершения всего цикла DMA (окончания мажорного цикла)
  DMA->TCD[cfg->ch].DADDR = 0;                         // Адрес приемника - буфер данных. На фазе конфигурации не устанавливаем
  DMA->TCD[cfg->ch].DOFF = cfg->minor_tranf_sz;        // После  записи указатель приемника смещаем
  DMA->TCD[cfg->ch].DLAST_SGA = 0;                     // Цепочки дескрипторов не применяем
  DMA->TCD[cfg->ch].NBYTES_MLNO = cfg->minor_tranf_sz; // Количество байт пересылаемых за один запрос DMA (в минорном цикле)
  DMA->TCD[cfg->ch].BITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | Линковку не применяем
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  |
  ;
  DMA->TCD[cfg->ch].CITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | Линковку не применяем
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  |
  ;
  DMA->TCD[cfg->ch].ATTR = 0
    + LSHIFT(0, 11) // SMOD  | Модуль адреса источника не используем
    + LSHIFT(DMA_Get_attr_size(cfg->minor_tranf_unit_sz), 8)  // SSIZE | | Задаем количество бит передаваемых за одну операцию чтения источника
    + LSHIFT(0, 3)  // DMOD  | Модуль адреса приемника
    + LSHIFT(DMA_Get_attr_size(cfg->minor_tranf_unit_sz), 0)  // DSIZE | | Задаем количество бит передаваемых за одну операцию записи в приемник
  ;
  DMA->TCD[cfg->ch].CSR = 0
    + LSHIFT(0, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
    + LSHIFT(0, 8)  // MAJORLINKCH | Линковку не применяем
    + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
    + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
    + LSHIFT(0, 5)  // MAJORELINK  | Линковку не применяем
    + LSHIFT(0, 4)  // ESG         | Цепочки дескрипторов не применяем
    + LSHIFT(1, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
    + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
    + LSHIFT(1, 1)  // INTMAJOR    | Используем прерывание по окнчании пересылки DMA
    + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;

  pDS_cbl->dma_rx_channel  = cfg->ch;
  {
    uint8_t indx = cfg->ch;
    cfg->DMAMUX->CHCFG[indx] = cfg->dmux_src + BIT(7); // Через мультиплексор связываем сигнал от внешней периферии (здесь от канала SPI) с входом выбранного канала DMA
  }

}


/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для отправки данных по SPI и пустого приема из SPI

  Данные передаются 1-о, 2-х или 4-х байтами словами
  sz - количество передаваемых слов (Это не размер буфера в байтах!)

  Может быть передано не более 2048 слов (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
void Start_DMA_TFT_TX_w_dummyRX(T_DMA_cbl *pDS_cbl, void *buf, uint32_t sz, uint8_t src_addr_module)
{
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  T_DMA_TCD  volatile *pTDCmf;
  T_DMA_TCD  volatile *pTDCfm;

  // Подготовка дескриптора DMA предназначенного для пустого чтения из SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_rx_channel;

  // Программируем дескриптор пустого чтения
  pTDCfm = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_rx_channel];
  pTDCfm->DADDR         = (uint32_t)&dummy_rx;
  pTDCfm->DOFF          = 0;
  pTDCfm->BITER_ELINKNO = (pTDCfm->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCfm->CITER_ELINKNO = pTDCfm->BITER_ELINKNO;


  // Подготовка дескриптора DMA предназначенных для записи буфера в SPI

  // Сбрасываем бит DONE
  DMA->CDNE = pDS_cbl->dma_tx_channel;

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->dma_tx_channel];
  pTDCmf->SADDR         = (uint32_t)buf;
  pTDCmf->SOFF          = 1;
  pTDCmf->BITER_ELINKNO = (pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;
  pTDCmf->ATTR = 0
    + LSHIFT(src_addr_module, 11) // SMOD  | Модуль адреса источника не используем
    + LSHIFT(DMA_Get_attr_size(1), 8)  // SSIZE | Задаем количество бит передаваемых за одну операцию чтения источника
    + LSHIFT(0, 3)  // DMOD  | Модуль адреса приемника
    + LSHIFT(DMA_Get_attr_size(1), 0)  // DSIZE | Задаем количество бит передаваемых за одну операцию записи в приемник
  ;

  DMA->SERQ = pDS_cbl->dma_rx_channel;  // Разрешаем поступление запроса DMA на канал  dma_rx_channel
  DMA->SERQ = pDS_cbl->dma_tx_channel;  // Разрешаем поступление запроса DMA на канал  dma_tx_channel
}


/*------------------------------------------------------------------------------
  Обработчик прерывания от модуля DMA по завершению чтения данных из SPI

 \param user_isr_ptr
 ------------------------------------------------------------------------------*/
void DMA_SPI0_bus_rx_isr(void *user_isr_ptr)
{

  DMA_CINT = DMA_LCD_FM_CH; // Сбрасываем флаг прерываний  канала

  // Очистим FIFO приемника и передатчика
  spi_mods[BUS_SPI0].spi->MCR  |= BIT(CLR_RXF)+ BIT(CLR_TXF);
  // Сбросим все флаги у SPI
  spi_mods[BUS_SPI0].spi->SR =  spi_mods[BUS_SPI0].spi->SR;

  _lwevent_set(&spi_cbl[BUS_SPI0].spi_event, SPI0_BUS_RXEND); // Уведомляем об окончании пересылки по DMA из SPI в память
}


/*------------------------------------------------------------------------------
  Конфигурирование 2-х каналов DMA на прием и на передачу для работы с модулем SPI
 ------------------------------------------------------------------------------*/
void Init_SPI0_bus_DMA(void)
{
  T_DMA_LCD_RX_config rx_cfg;
  T_DMA_LCD_TX_config tx_cfg;

  tx_cfg.ch                  = DMA_LCD_MF_CH;                           // номер канала DMA
  tx_cfg.spi_pushr           = (uint32_t)&(spi_mods[BUS_SPI0].spi->PUSHR); // адрес регистра PUSHR SPI
  tx_cfg.DMAMUX              = DMA_LCD_DMUX_PTR;                        // Указатель на мультиплексор входных сигналов для DMA
  tx_cfg.dmux_src            = DMA_LCD_DMUX_TX_SRC;                     // номер входа периферии для выбранного мультиплексора DMAMUX для передачи на DMA.
  tx_cfg.minor_tranf_unit_sz = DMA_1BYTE_MINOR_TRANSFER;
  tx_cfg.minor_tranf_sz      = DMA_1BYTE_MINOR_TRANSFER;
  Config_DMA_TFT_TX(&tx_cfg,&SPI0_DS_cbl);


  rx_cfg.ch                  = DMA_LCD_FM_CH;                          // номер канала DMA
  rx_cfg.spi_popr            = (uint32_t)&(spi_mods[BUS_SPI0].spi->POPR); // адрес регистра POPR SPI
  rx_cfg.DMAMUX              = DMA_LCD_DMUX_PTR;                       // номер входа периферии для выбранного мультиплексора DMAMUX для передачи на DMA.
  rx_cfg.dmux_src            = DMA_LCD_DMUX_RX_SRC;
  rx_cfg.minor_tranf_unit_sz = DMA_1BYTE_MINOR_TRANSFER;
  rx_cfg.minor_tranf_sz      = DMA_1BYTE_MINOR_TRANSFER;
  Config_DMA_TFT_RX(&rx_cfg,&SPI0_DS_cbl);

  Install_and_enable_isr(DMA_LCD_RX_INT_NUM, spi_mods[BUS_SPI0].prio, DMA_SPI0_bus_rx_isr); // Прерывание по завершению приема по DMA
}


