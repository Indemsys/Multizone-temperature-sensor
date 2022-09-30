#include "app.h"



/*-----------------------------------------------------------------------------------------------------
  Деинициализация периферии использованной модулем Modbus
  
  \param   
-----------------------------------------------------------------------------------------------------*/
void  MB_BSP_deinit(void)
{
  uint8_t        i;

  for (i = 0; i < MODBUS_CFG_MAX_CH; i++)
  {
    K66BLEZ1_UART_DeInit(modbus_cbls[i].uart_number);
  }
}


/*-----------------------------------------------------------------------------------------------------
  Callback функци вызываема в прерывании при приеме данных по MODBUS
  
  \param portn  
  \param ch  
-----------------------------------------------------------------------------------------------------*/
static void  MB_BSP_rx_handler(uint8_t portn, uint8_t ch)
{
  T_MODBUS_ch   *pch;

  pch =&modbus_cbls[portn];

#if (MODBUS_ANS_LATENTION_STAT == 1)
  if (pch->rx_packet_size==0) 
  {
    MQX_TICK_STRUCT now;
    bool           overfl;
    uint32_t       tdif;


    _time_get_ticks(&now);
    tdif = _time_diff_microseconds(&now,&pch->time_stump,&overfl);
    if (tdif > pch->ans_max_time)  pch->ans_max_time = tdif;
    if (tdif < pch->ans_min_time)  pch->ans_min_time = tdif;
  }
#endif

  pch->recv_bytes_count++;
  MB_rx_byte(pch, ch);
}


/*-------------------------------------------------------------------------------------------------------------
  Инициализируем DMA для отправки данных по UART 

  Данные передаются 1-о, 2-х или 4-х байтами словами
  sz - количество передаваемых слов (Это не размер буфера в байтах!)

  Может быть передано не более 2048 слов (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
static void MB_BSP_Start_DMA_TX(T_DMA_cbl *pdma_cbl, void *buf, uint32_t sz)
{
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  T_DMA_TCD  volatile *pTDCmf;
  // Подготовка дескриптора DMA предназначенных для записи буфера в UART

  // Сбрасываем бит DONE
  DMA->CDNE = pdma_cbl->dma_tx_channel;

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pdma_cbl->dma_tx_channel];
  pTDCmf->SADDR         = (uint32_t)buf;
  pTDCmf->SOFF          = pdma_cbl->minor_tranf_sz;
  pTDCmf->BITER_ELINKNO =(pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;
  pTDCmf->CSR |= BIT(1);  // Разрешаем прерывания по окончанию DMA

  DMA->SERQ = pdma_cbl->dma_tx_channel;  // Разрешаем поступление запроса DMA на канал  dma_tx_channel
}

/*-----------------------------------------------------------------------------------------------------
  
  
  \param pch  
-----------------------------------------------------------------------------------------------------*/
void  MB_BSP_stop_receiving(T_MODBUS_ch  *pch)
{
  K66BLEZ1_UART_Set_RX_state(pch->uart_number, 0); // Выключаем приемник
  K66BLEZ1_UART_Set_RX_Int(pch->uart_number, 0);   // Запрещаем прерывания от приемника
}


/*-----------------------------------------------------------------------------------------------------
  Посылка пакета MODBUS через UART по DMA  
  
  \param pch  
-----------------------------------------------------------------------------------------------------*/
void  MB_BSP_send_packet(T_MODBUS_ch  *pch)
{

  K66BLEZ1_UART_Set_RX_state(pch->uart_number, 0); // Выключаем приемник
  K66BLEZ1_UART_Set_RX_Int(pch->uart_number, 0);   // Запрещаем прерывания от приемника
  K66BLEZ1_UART_clear_rx_fifo(pch->uart_number);   // Очищаем FIFO приемника
  
  // Запускаем DMA на передачу пакета 
  MB_BSP_Start_DMA_TX(&pch->dma_cbl, pch->tx_buf, pch->tx_packet_size);

  K66BLEZ1_UART_Set_TX_Int(pch->uart_number, 1);   // Разрешаем прерывания и запросы DMA от передатчика
                                                   // После этой команды стартует передача по UART 
}

/*-----------------------------------------------------------------------------------------------------
  Callback функци вызываемая в прерывании после окончания передаче данных по MODBUS
  
  \param portn  
  
  \return uint8_t 
-----------------------------------------------------------------------------------------------------*/
static void MB_BSP_tc_handler(uint8_t portn)
{
  T_MODBUS_ch   *pch;

  K66BLEZ1_UART_Set_TX_Int(portn, 0);   // Запрещаем прерывания и запросы DMA по флагу готовности передатчика
  
  // Могут быть ситуации когда прерывание по флагу окончания передачи (TC) происходит раньше чем возникнет передача инициированная по DMA
  // Тогда это прерывание игнорируем
  // Такая ситуация наблюдается после первого пакета DMA после включения UART, прерывание может повторяться несколько раз подряд в течении 10 мкс 
  if (K66BLEZ1_UART_Get_TXEMPT_flag(portn)==0) 
  {
    return;
  }

  pch =&modbus_cbls[portn];
  pch->sent_bytes_count += pch->tx_packet_size;
  pch->tx_packet_ptr =&pch->tx_buf[0];

#if (MODBUS_CFG_MASTER_EN == 1)
  if (pch->master_slave_flag == MODBUS_MASTER)
  {
#if (MODBUS_CFG_RTU_EN == 1)
    pch->rtu_timeout_en = MODBUS_FALSE; // После окончания передачи тайм аут ожидания завершения пакета не еще активируем
                                        // Он будет активирован после получения первого байта ответа
#endif
    pch->rx_packet_size  = 0;
  }
#endif

#if (MODBUS_ANS_LATENTION_STAT == 1)
  _time_get_ticks(&pch->time_stump);
#endif

  // Очищаем FIFO приемника на случай если в нем оказались случайные данные после сбоя UART-а 
  K66BLEZ1_UART_clear_rx_fifo(portn);
  // Все выслали и начинаем прием. Разрешаем прерывания по приему данных
  K66BLEZ1_UART_Set_TXC_Int(portn, 0);  // Запрещаем прерывания по флагу окончания передачи данных
  K66BLEZ1_UART_Set_RX_state(portn, 1); // Включаем приемник
  K66BLEZ1_UART_Set_RX_Int(portn, 1);   // Разрешаем прерывания от приемника
}

/*------------------------------------------------------------------------------
  Обработчик прерывания от модуля DMA по завершению передачи данных в UART
  Разрешает прерывание UART по флагу окончания передачи,
  поскольку после завешения DMA еще не все данные покинули UARТ и надо дождаться пока послений бит уйдет в линию 
 
 \param user_isr_ptr
 ------------------------------------------------------------------------------*/
static void MB_BSP_DMA_tx_handler(void *user_isr_ptr)
{
  uint32_t portn;
  UART_MemMapPtr UART;

  DMA_CINT = DMA_UART_MF_CH; // Сбрасываем флаг прерываний  канала
  portn = (uint32_t)user_isr_ptr;
  UART   = K66BLEZ1_get_UART(portn);

  K66BLEZ1_UART_Set_TXC_Int(portn, 1);  // Разрешаем прерывание по флагу окончания передачи UART
}

/*------------------------------------------------------------------------------
  Конфигурирование DMA для передачи по UART
 ------------------------------------------------------------------------------*/
static void MB_BSP_config_tx_DMA(T_MODBUS_ch  *pch, uint8_t  portn)
{
  T_DMA_config   tx_cfg;
  UART_MemMapPtr UART     = K66BLEZ1_get_UART(portn);
                          
  tx_cfg.dma_channel      = DMA_UART_MF_CH;             // номер канала DMA
  tx_cfg.periph_port      = (uint32_t)&(UART->D);       //
  tx_cfg.DMAMUX           = DMA_UART_DMUX_PTR;          // Указатель на мультиплексор входных сигналов для DMA
  tx_cfg.dmux_src         = K66BLEZ1_get_UART_DMA_tx_src(portn);       // номер входа периферии для выбранного мультиплексора DMAMUX для передачи на DMA.
  tx_cfg.minor_tranf_sz   = DMA_1BYTE_MINOR_TRANSFER;
  tx_cfg.enable_interrupt = 1;
  Config_DMA_TX(&tx_cfg,&(pch->dma_cbl));

  _int_install_isr(DMA_UART_TX_INT_NUM, MB_BSP_DMA_tx_handler, (void *)portn);
  _bsp_int_init(DMA_UART_TX_INT_NUM, MODBUS_DMA_PRIO, 0, TRUE);
}


/*-----------------------------------------------------------------------------------------------------
  Инициализация UART-а для канала MODBUS
  
  \param pch          указатель на управляющую структуру
  \param uart_nbr     номер UART-а
  \param baud         скрость UART-а  
  \param bits         количество бит 
  \param parity       режим контроля четности 
  \param stops        количество STOP бит
-----------------------------------------------------------------------------------------------------*/
void  MB_BSP_config_UART(T_MODBUS_ch   *pch, uint8_t  portn, uint32_t  baud, uint8_t  bits, uint8_t  parity, uint8_t  stops)
{
  pch->uart_number   = portn;
  pch->baud_rate     = baud;
  pch->parity_chek   = parity;
  pch->bits_num      = bits;
  pch->stop_bits     = stops;

  K66BLEZ1_UART_Init(portn, baud, bits, parity, stops);

  K66BLEZ1_UART_Set_txrx_callback(portn , MB_BSP_rx_handler, 0);
  K66BLEZ1_UART_Set_tc_callback(portn, MB_BSP_tc_handler);
  MB_BSP_config_tx_DMA(pch, portn);
  K66BLEZ1_UART_enable_tx_DMA(portn);
  K66BLEZ1_UART_Set_TX_state(portn, 1); // Включаем передатчик
  K66BLEZ1_UART_Set_RX_state(portn, 0); // Выключаем приемник
}




#if (MODBUS_CFG_RTU_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Callback функция вызываем по прерываниям таймера 
  
  \param void  
-----------------------------------------------------------------------------------------------------*/
static void  MB_BSP_timer_tick_handler(void)
{
  rtu_timer_tick_counter++;
  MB_RTU_packet_end_detector();
}

#endif


#if (MODBUS_CFG_RTU_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Инициализируем таймер 
  
  \param   
-----------------------------------------------------------------------------------------------------*/
void  MB_BSP_timer_init(void)
{
  uint32_t period_us = 1000000ul / rtu_timer_tick_freq;

  PIT_init_interrupt(MODBUS_PIT_TIMER, period_us, MODBUS_PIT_PRIO , MB_BSP_timer_tick_handler);
  MB_RTU_TmrResetAll();
}
#endif


#if (MODBUS_CFG_RTU_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Прекращаем работу таймера 
  
  \param void  
-----------------------------------------------------------------------------------------------------*/
void  MB_BSP_timer_deinit(void)
{
  PIT_disable(MODBUS_PIT_TIMER);
}
#endif



