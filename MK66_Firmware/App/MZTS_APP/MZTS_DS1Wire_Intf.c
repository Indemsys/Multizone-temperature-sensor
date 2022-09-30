// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2022-07-26
// 17:04:38
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"


#define PRESENCE_SIG_BUAD_RATE 18000

static LWEVENT_STRUCT           one_wire_lwev;
#define ONEWIRE_TX_COMPLETE     BIT(0)
#define ONEWIRE_RX_COMPLETE     BIT(1)
#define ONEWIRE_PULSE_COMPLETE  BIT(2)

static uint8_t            *rx_buf;
static volatile uint32_t  recv_cnt;

/*-----------------------------------------------------------------------------------------------------
  Инициализация UART с номером intf_num
  Режим модема не используется
  Используется сигнал RTS для управления драйвером RS485


  \param intf_num  - номер порта
  \param baud
  \param bits
  \param parity
  \param stops
-----------------------------------------------------------------------------------------------------*/
static void OneWire_UART_hw_Init(uint32_t intf_num, uint32_t  baud)
{
  UART_MemMapPtr UART;
  T_UART_cbl     *pcbl;
  uint32_t       module_clock;



  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];
  pcbl =&UART_cbls[intf_num];

  // Включаем модуль периферии
  K66BLEZ1_UART_activate(intf_num);

  // Отключаем приемник и передатчик если они был включены
  UART->C2 = 0;

  // Определяем частоту тактирования модуля UART
  K66BLEZ1_UART_get_clock(intf_num,&module_clock);

  // UART baud rate = UART module clock / (16 * (SBR[12:0] + BRFD))
  uint32_t sbr  = module_clock / (baud * 16);
  uint32_t brfa =((module_clock %(baud * 16)) * 32) / baud;


// UART Baud Rate Registers: High (UARTx_BDH)  | Default = 0
  UART->BDH = 0
             + LSHIFT(0,  7)                  // LBKDIE  | LIN Break Detect Interrupt Enable
             + LSHIFT(0,  6)                  // RXEDGIE | RxD Input Active Edge Interrupt Enable
             + LSHIFT(0,  5)                  // SBNS    | Stop Bit Number Select
             + LSHIFT(((sbr >> 8) & 0x1F), 0) // SBR     | UART Baud Rate Bits 12..8
  ;

// UART Baud Rate Registers: Low (UARTx_BDL)  | Default = 4
  UART->BDL = 0
             + LSHIFT((sbr & 0xFF),0) // SBR     | UART Baud Rate Bits 7..0
  ;

// UART Control Register 1 (UARTx_C1)  | Default = 0
  UART->C1 = 0
            + LSHIFT(0, 7) // LOOPS    | Loop Mode Select              | 0 Normal operation.
            + LSHIFT(0, 6) // UARTSWAI | UART Stops in Wait Mode       | 0 UART clock continues to run in Wait mode
            + LSHIFT(0, 5) // RSRC     | Receiver Source Select
            + LSHIFT(0, 4) // M        | 9-bit or 8-bit Mode Select    | 0 Normal—start + 8 data bits (MSB/LSB first as determined by MSBF) + stop
            + LSHIFT(0, 3) // WAKE     | Receiver Wakeup Method Select | 0 Idle line wakeup.
            + LSHIFT(0, 2) // ILT      | Idle Line Type Select         | 0 Idle character bit count starts after start bit
            + LSHIFT(0, 1) // PE       | Parity Enable                 | 0 Parity function disabled.
            + LSHIFT(0, 0) // PT       | Parity Type                   | 0 Even parity.
  ;

// UART Control Register 2 (UARTx_C2)  | Default = 0
  UART->C2 = 0
            + LSHIFT(0, 7) // TIE  | Transmitter Interrupt or DMA Transfer Enable.  | 0 TDRE interrupt and DMA transfer requests disabled.
            + LSHIFT(0, 6) // TCIE | Transmission Complete Interrupt Enable         | 0 TC interrupt requests disabled. Прерывание происходит когда все биты переданы
            + LSHIFT(0, 5) // RIE  | Receiver Full Interrupt or DMA Transfer Enable | 0 RDRF interrupt and DMA transfer requests disabled.
            + LSHIFT(0, 4) // ILIE | Idle Line Interrupt Enable                     | 0 IDLE interrupt requests disabled.
            + LSHIFT(0, 3) // TE   | Transmitter Enable                             | 0 Transmitter off
            + LSHIFT(0, 2) // RE   | Receiver Enable                                | 0 Receiver off.
            + LSHIFT(0, 1) // RWU  | Receiver Wakeup Control                        | 0 Normal operation.
            + LSHIFT(0, 0) // SBK  | Send Break                                     | 0 Normal transmitter operation.
  ;

// UART Status Register 1 (UARTx_S1)
  UART->S1 = 0
            + LSHIFT(0, 7) // TDRE (ro)| Transmit Data Register Empty Flag  | Взведен всегда, когда можно разместить данные для отправки в регистр или FIFO
            + LSHIFT(0, 6) // TC   (ro)| Transmit Complete Flag             | Взведен всегда, когда все данные отправлены
            + LSHIFT(0, 5) // RDRF (ro)| Receive Data Register Full Flag
            + LSHIFT(0, 4) // IDLE (ro)| Idle Line Flag
            + LSHIFT(0, 3) // OR   (ro)| Receiver Overrun Flag
            + LSHIFT(0, 2) // NF   (ro)| Noise Flag
            + LSHIFT(0, 1) // FE   (ro)| Framing Error Flag
            + LSHIFT(0, 0) // PF   (ro)| Parity Error Flag
  ;

// UART Status Register 2 (UARTx_S2)  | Default = 0
  UART->S2 = 0
            + LSHIFT(1, 7) // LBKDIF  (w1c)| LIN Break Detect Interrupt Flag
            + LSHIFT(1, 6) // RXEDGIF (w1c)| RxD Pin Active Edge Interrupt Flag
            + LSHIFT(0, 5) // MSBF         | Most Significant Bit First
            + LSHIFT(0, 4) // RXINV        | Receive Data Inversion
            + LSHIFT(0, 3) // RWUID        | Receive Wakeup Idle Detect
            + LSHIFT(0, 2) // BRK13        | Break Transmit Character Length
            + LSHIFT(0, 1) // LBKDE        | LIN Break Detection Enable
            + LSHIFT(0, 0) // RAF     (ro) | Receiver Active Flag
  ;

// UART Control Register 3 (UARTx_C3)  | Default = 0
  UART->C3 = 0
            + LSHIFT(0, 7) // R8 (ro) | Received Bit 8
            + LSHIFT(0, 6) // T8      | Transmit Bit 8
            + LSHIFT(0, 5) // TXDIR   | Transmitter Pin Data Direction in Single-Wire mode
            + LSHIFT(0, 4) // TXINV   | Transmit Data Inversion
            + LSHIFT(0, 3) // ORIE    | Overrun Error Interrupt Enable
            + LSHIFT(0, 2) // NEIE    | Noise Error Interrupt Enable
            + LSHIFT(0, 1) // FEIE    | Framing Error Interrupt Enable
            + LSHIFT(0, 0) // PEIE    | Parity Error Interrupt Enable
  ;

// UART Control Register 4 (UARTx_C4)  | Default = 0
  UART->C4 = 0
            + LSHIFT(0, 7) // MAEN1 | Match Address Mode Enable 1
            + LSHIFT(0, 6) // MAEN2 | Match Address Mode Enable 2
            + LSHIFT(0, 5) // M10   | 10-bit Mode select
            + LSHIFT(brfa & 0x1F, 0) // BRFA  | Baud Rate Fine Adjust
  ;

// UART Control Register 5 (UARTx_C5)  | Default = 0
  UART->C5 = 0
            + LSHIFT(0, 7) // TDMAS | Transmitter DMA Select
            + LSHIFT(0, 5) // RDMAS | Receiver Full DMA Select
  ;


// UART Extended Data Register (UARTx_ED)
  UART->ED = 0
            + LSHIFT(0, 7) // NOISY   (ro) | The current received dataword contained in D and C3[R8] was received with noise
            + LSHIFT(0, 6) // PARITYE (ro) | The current received dataword contained in D and C3[R8] was received with a parity error.
  ;

// UART Modem Register (UARTx_MODEM)  | Default = 0
  UART->MODEM = 0
               + LSHIFT(0, 3) // RXRTSE   | Receiver request-to-send enable       |
               + LSHIFT(0, 2) // TXRTSPOL | Transmitter request-to-send polarity  | 1 Transmitter RTS is active high.
               + LSHIFT(0, 1) // TXRTSE   | Transmitter request-to-send enable    | 1 -  RTS asserts one bit time before the start bit is transmitted.
               + LSHIFT(0, 0) // TXCTSE   | Transmitter clear-to-send enable      |
  ;

// UART FIFO Parameters (UARTx_PFIFO)  | Default = 0
  UART->PFIFO = 0
               + LSHIFT(0, 7) // TXFE           | Transmit FIFO Enable
               + LSHIFT(0, 4) // TXFIFOSIZE (ro)| Transmit FIFO. Buffer Depth
               + LSHIFT(1, 3) // RXFE           | Receive FIFO Enable
               + LSHIFT(0, 0) // RXFIFOSIZE (ro)| Receive FIFO. Buffer Depth
  ;

  UART->TWFIFO = K66BLEZ1_UART_Solve_TWFIFO((UART->PFIFO >> 4) & 0x07); // Устанавливаем сигнализирующую границу FIFO передатчика на 1 меньше чем емкость FIFO передатчика

// UART FIFO Control Register (UARTx_CFIFO)  | Default = 0
  UART->CFIFO = 0
               + LSHIFT(1, 7) // TXFLUSH (wo)| Transmit FIFO/Buffer Flush
               + LSHIFT(1, 6) // RXFLUSH (wo)| Receive FIFO/Buffer Flush
               + LSHIFT(0, 2) // RXOFE       | Receive FIFO Overflow Interrupt Enable
               + LSHIFT(0, 1) // TXOFE       | Transmit FIFO Overflow Interrupt Enable
               + LSHIFT(0, 0) // RXUFE       | Receive FIFO Underflow Interrupt Enable
  ;

// UART FIFO Status Register (UARTx_SFIFO)  | Default = 0
  UART->SFIFO = 0
               + LSHIFT(0, 7) // TXEMPT (ro)| Transmit Buffer/FIFO Empty
               + LSHIFT(0, 6) // RXEMPT (ro)| Receive Buffer/FIFO Empty
               + LSHIFT(1, 2) // RXOF  (w1c)| Receiver Buffer Overflow Flag
               + LSHIFT(1, 1) // TXOF  (w1c)| Transmitter Buffer Overflow Flag
               + LSHIFT(1, 0) // RXUF  (w1c)| Receiver Buffer Underflow Flag
  ;
}

/*-----------------------------------------------------------------------------------------------------


  \param intf_num
  \param baud
-----------------------------------------------------------------------------------------------------*/
static void OneWire_UART_set_baud_rate(uint32_t intf_num, uint32_t  baud)
{
  UART_MemMapPtr   UART;
  uint32_t         module_clock;

  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];


  // Включаем модуль периферии
  K66BLEZ1_UART_activate(intf_num);

  // Отключаем приемник и передатчик
  UART->C2 &= ~(0
                + BIT(3) // TE   | Transmitter Enable                             | 0 Transmitter off
                + BIT(2) // RE   | Receiver Enable                                | 0 Receiver off.
               );

  // Определяем частоту тактирования модуля UART
  K66BLEZ1_UART_get_clock(intf_num,&module_clock);

  // UART baud rate = UART module clock / (16 * (SBR[12:0] + BRFD))
  uint32_t sbr  = module_clock / (baud * 16);

  uint32_t brfa =((module_clock %(baud * 16)) * 32) / (baud * 16);


// UART Baud Rate Registers: High (UARTx_BDH)  | Default = 0
  UART->BDH = 0
             + LSHIFT(0,  7)                  // LBKDIE  | LIN Break Detect Interrupt Enable
             + LSHIFT(0,  6)                  // RXEDGIE | RxD Input Active Edge Interrupt Enable
             + LSHIFT(0,  5)                  // SBNS    | Stop Bit Number Select
             + LSHIFT(((sbr >> 8) & 0x1F), 0) // SBR     | UART Baud Rate Bits 12..8
  ;

// UART Baud Rate Registers: Low (UARTx_BDL)  | Default = 4
  UART->BDL = 0
             + LSHIFT((sbr & 0xFF),0) // SBR     | UART Baud Rate Bits 7..0
  ;
// UART Control Register 4 (UARTx_C4)  | Default = 0
  UART->C4 = 0
            + LSHIFT(0, 7) // MAEN1 | Match Address Mode Enable 1
            + LSHIFT(0, 6) // MAEN2 | Match Address Mode Enable 2
            + LSHIFT(0, 5) // M10   | 10-bit Mode select
            + LSHIFT(brfa & 0x1F, 0) // BRFA  | Baud Rate Fine Adjust
  ;

  // Включаем приемник и передатчик
  UART->C2 |=(0
              + BIT(3) // TE   | Transmitter Enable                             | 0 Transmitter off
              + BIT(2) // RE   | Receiver Enable                                | 0 Receiver off.
             );

}


/*-----------------------------------------------------------------------------------------------------
  Callback функци вызываема в прерывании при приеме данных по MODBUS

  \param portn
  \param ch
-----------------------------------------------------------------------------------------------------*/
static void  OneWire_rx_handler(uint8_t portn, uint8_t ch)
{
  if (recv_cnt != 0)
  {
    *rx_buf = ch;
    rx_buf++;
    recv_cnt--;
    if (recv_cnt == 0)
    {
      // Выставляем флаг получения всего количества затребованных данных
      _lwevent_set(&one_wire_lwev, ONEWIRE_RX_COMPLETE);
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Обработчик по событию - освободилось место в FIFO передатчика, можно передать следующий байт

  \param portn
-----------------------------------------------------------------------------------------------------*/
static  void OneWire_tx_handler(uint8_t portn)
{
}

/*-----------------------------------------------------------------------------------------------------
  Обработчик по событию - окончилась передача всего что было в FIFO и в буфере передачи
  Возникает после окончания передачи стоп бита, т.е. спустя один бит с момента перехода в высокое состояние на динии 1Wire
  По этой причине возникновение прерывания может опоздать к моменту появления импульса отклика на 1Wire

  \param portn
-----------------------------------------------------------------------------------------------------*/
static void OneWire_tc_handler(uint8_t portn)
{
  UART_MemMapPtr UART;

  //ITM_EVENT8(2, 2);
  //PTE_BASE_PTR->PCOR = BIT(26); // Диагностический вывод на внешний LED

  UART = UART_intfs_map[ONEWIRE_UART_PORT];
  UART->C2 &= ~BIT(6); // TCIE | Transmission Complete Interrupt Enable         | 0 TC interrupt requests disabled.

  //ITM_EVENT8(3,0);
  _lwevent_set(&one_wire_lwev, ONEWIRE_TX_COMPLETE); // Не используем это сорбытие
}



static uint32_t prd_step;
static uint32_t imp_fall_edge_time;
static uint32_t imp_rise_edge_time;

/*-----------------------------------------------------------------------------------------------------
  Прерывания по фронту и спаду импульса 1Wire

  Сначала ожидаем подъема на линии RX  после окончания импульса передаваемого UART
  Потом ждем спада импульса присутствия
  И затем ждем подъема по окончании импульса присутствия

  \param
-----------------------------------------------------------------------------------------------------*/
static void PORTE_isr(void)
{
  PORT_MemMapPtr PORTE  = PORTE_BASE_PTR;

  switch (prd_step)
  {
  case 0:
    // Обнаружили подъем после импульса от UART, ждем спад
    PORTE->PCR[25] &= ~(0xF << 16);
    PORTE->PCR[25] |= LSHIFT(IRQ_IFE, 16); // ISF flag and Interrupt on falling-edge.
    prd_step++;
    break;
  case 1:
    // Обнаружили спад на начале импульса присутствия, ждем подъем
    imp_fall_edge_time = SYST_CVR;    // Захватываем врем спада. Таймер SYST_CVR работает в режиме декремента
    PORTE->PCR[25] &= ~(0xF << 16);
    PORTE->PCR[25] |= LSHIFT(IRQ_IRE, 16); // ISF flag and Interrupt on rising-edge.
    prd_step++;
    break;
  case 2:
    // Обнаружили подъем на окончании импульса присутствия, жавершаем процедуру
    imp_rise_edge_time = SYST_CVR;    // Захватываем врем подъема. Таймер SYST_CVR работает в режиме декремента
    PORTE->PCR[25] &= ~(0xF << 16);
    NVIC_STIR(0)= 84-16; // Вызываем системное прерывание с номером 84, периферией не используется, а здесь нужно чтобы передать в его обработчике событие операционной системе
    prd_step = 0;
    break;

  }

  PORTE->ISFR           = 0xFFFFFFFF; // Стираем флаги прерываний
}

/*-----------------------------------------------------------------------------------------------------
  Прерывание по вектору INT_Reserved84 сообщающее системе об окончании измерения импульса присутствия 1Wire

  \param ptr
-----------------------------------------------------------------------------------------------------*/
static void OneWire_pulse_end_isr(void *ptr)
{
  _lwevent_set(&one_wire_lwev, ONEWIRE_PULSE_COMPLETE);
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void OneWire_UART_config(void)
{
  _lwevent_create(&one_wire_lwev, LWEVENT_AUTO_CLEAR);

  OneWire_UART_hw_Init(ONEWIRE_UART_PORT, PRESENCE_SIG_BUAD_RATE);

  Install_and_enable_kernel_isr(INT_PORTE, PORTE_KERN_PRIO, PORTE_isr); // Прерывания по фронту сигнала ENC_A квадратурного энкодера
  Install_and_enable_isr(UART_int_nums[ONEWIRE_UART_PORT], UART_priots[ONEWIRE_UART_PORT], UART_isrs[ONEWIRE_UART_PORT]);
  Install_and_enable_isr(INT_Reserved84, UART_priots[ONEWIRE_UART_PORT], OneWire_pulse_end_isr);

  K66BLEZ1_UART_Set_txrx_callback(ONEWIRE_UART_PORT , OneWire_rx_handler, 0);
  K66BLEZ1_UART_Set_tc_callback(ONEWIRE_UART_PORT, OneWire_tc_handler);
  K66BLEZ1_UART_Set_TXC_Int(ONEWIRE_UART_PORT, 0);
  K66BLEZ1_UART_Set_TX_Int(ONEWIRE_UART_PORT, 0);   // Запрещаем прерывания от передатчика
  K66BLEZ1_UART_Set_RX_Int(ONEWIRE_UART_PORT, 1);   // Разрешаем прерывания от приемника

}


/*-----------------------------------------------------------------------------------------------------


  \param baud_rate
-----------------------------------------------------------------------------------------------------*/
void OneWire_UART_init(uint32_t baud_rate)
{
  OneWire_UART_set_baud_rate(ONEWIRE_UART_PORT, baud_rate);
}


/*-----------------------------------------------------------------------------------------------------
  Функция ожидания импульса присутствия
  Сначала ожидаем подъема на линии RX  после окончания импульса передаваемого UART
  Потом ждем спада импульса присутствия
  И затем ждем подъема по окончании импульса присутствия

  Возвращает 0 если импульс присутствия не обнаружен
  Иначе возвращает длительность импульса в микросекундах
-----------------------------------------------------------------------------------------------------*/
uint32_t OneWire_wait_presence_pulse(void)
{
  //  Конфигурируем порт PTE25 на прерывание
  PORT_MemMapPtr PORTE  = PORTE_BASE_PTR;

  PORTE->ISFR        = 0xFFFFFFFF; // Стираем флаги прерываний
  prd_step           = 0;
  imp_rise_edge_time = 0;
  imp_fall_edge_time = 0;
  PORTE->PCR[25] &= ~(0xF << 16);
  PORTE->PCR[25] |= LSHIFT(IRQ_IRE, 16); // ISF flag and Interrupt on rising-edge.

  _lwevent_clear(&one_wire_lwev, ONEWIRE_PULSE_COMPLETE);
  // Ожидаем результатов измерения длительности импульса присутствия 1Wire
  if (_lwevent_wait_ticks(&one_wire_lwev, ONEWIRE_PULSE_COMPLETE, TRUE, ms_to_ticks(10)) == MQX_OK)
  {
    uint32_t d;
    if (imp_rise_edge_time >= imp_fall_edge_time) return 0;
    d = imp_fall_edge_time - imp_rise_edge_time;
    // Переводим в микросекунды
    d =d / (BSP_SYSTEM_CLOCK/1000000ul);
    //ITM_EVENT8(4,0);
    return d;
  }

  return 0;

}


/*-----------------------------------------------------------------------------------------------------


  \param val
-----------------------------------------------------------------------------------------------------*/
void OneWire_UART_send_byte(uint8_t val)
{
  volatile uint8_t d;
  UART_MemMapPtr UART;
  UART = UART_intfs_map[ONEWIRE_UART_PORT];

   _lwevent_clear(&one_wire_lwev, ONEWIRE_TX_COMPLETE);

//  ITM_EVENT8(2, 1)
//  PTE_BASE_PTR->PSOR = BIT(26); // Диагностический вывод на внешний LED
  d = UART->S1;
  UART->D = val;
  UART->C2 |= BIT(6); // TCIE
}


/*-----------------------------------------------------------------------------------------------------
  Ждем флага окончания передачи по UART


  \param timeout

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t OneWire_UART_wait_transmit_complete(uint32_t timeout)
{
  uint32_t status;
  status = _lwevent_wait_ticks(&one_wire_lwev, ONEWIRE_TX_COMPLETE, TRUE, ms_to_ticks(timeout));
  return status;
}

/*-----------------------------------------------------------------------------------------------------
  Назначить ожидание приема заданного количества байт в буфер

  \param buf
  \param timeout
-----------------------------------------------------------------------------------------------------*/
void OneWire_UART_assign_rx_data_wait(uint8_t *buf, uint32_t sz)
{
  _lwevent_clear(&one_wire_lwev, ONEWIRE_RX_COMPLETE);
  rx_buf = buf;   // Назначаем буфер приема
  recv_cnt = sz;  // Назначаем количество данных кторые необходимо принять

}


/*-----------------------------------------------------------------------------------------------------
  Ожидание  заданного ранее количества байт в буфер в течении заданного таймаута

  \param timeout

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t OneWire_UART_wait_data(uint32_t timeout)
{
  if (_lwevent_wait_ticks(&one_wire_lwev, ONEWIRE_RX_COMPLETE, TRUE, ms_to_ticks(timeout)) == MQX_OK)
  {
    return RES_OK;
  }
  else
  {
    return RES_ERROR;
  }
}

