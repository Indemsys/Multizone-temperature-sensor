// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2018.11.26
// 14:29:36
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "app.h"


// UART0 and UART1 are clocked from the core clock,
// the remaining UARTs are clocked on the bus clock.
// The maximum baud rate is 1/16 of related source clock frequency.

// UART0 and UART1 contain 8-entry transmit and 8-entry receive FIFOs.
// All other UARTs contain a 1-entry transmit and receive FIFOs




static void UART0_isr(void *ptr);
static void UART1_isr(void *ptr);
static void UART2_isr(void *ptr);
static void UART3_isr(void *ptr);
static void UART4_isr(void *ptr);
static void UART_isr(uint32_t intf_num);
static void UART_err_isr(uint32_t intf_num);



T_UART_cbl      UART_cbls         [UART_INTFS_NUM]; // Контрольные блоки I2C
UART_MemMapPtr  UART_intfs_map    [UART_INTFS_NUM] = {UART0_BASE_PTR, UART1_BASE_PTR, UART2_BASE_PTR, UART3_BASE_PTR, UART4_BASE_PTR};
INT_ISR_FPTR    UART_isrs         [UART_INTFS_NUM] = {UART0_isr, UART1_isr, UART2_isr, UART3_isr, UART4_isr};
int32_t         UART_priots       [UART_INTFS_NUM] = {UART0_PRIO, UART1_PRIO, UART2_PRIO, UART3_PRIO, UART4_PRIO};
int32_t         UART_int_nums     [UART_INTFS_NUM] = {INT_UART0_RX_TX, INT_UART1_RX_TX, INT_UART2_RX_TX, INT_UART3_RX_TX, INT_UART4_RX_TX };
int32_t         UART_err_int_nums [UART_INTFS_NUM] = {INT_UART0_ERR, INT_UART1_ERR, INT_UART2_ERR, INT_UART3_ERR, INT_UART4_ERR };
int32_t         UART_DMA_tx_srcs  [UART_INTFS_NUM] = {DMUX_SRC_UART0_TX, DMUX_SRC_UART1_TX, DMUX_SRC_UART2_TX, DMUX_SRC_UART3_TX, DMUX_SRC_UART4_RX};

void K66BLEZ1_UART_activate(uint32_t  intf_num);
void K66BLEZ1_UART_get_clock(uint32_t intf_num, uint32_t *module_clock);

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
static void UART0_isr(void *ptr)
{
  UART_isr(0);
}
static void UART1_isr(void *ptr)
{
  UART_isr(1);
}
static void UART2_isr(void *ptr)
{
  UART_isr(2);
}
static void UART3_isr(void *ptr)
{
  UART_isr(3);
}
static void UART4_isr(void *ptr)
{
  UART_isr(4);
}

#define BIT_TDRE   BIT(7)  // Transmit Data Register Empty Flag
#define BIT_TC     BIT(6)  // Transmit Complete Flag
#define BIT_RDRF   BIT(5)  // Receive Data Register Full Flag
#define BIT_IDLR   BIT(4)  // Idle Line Flag
#define BIT_OR     BIT(3)  // Receiver Overrun Flag
#define BIT_NF     BIT(2)  // Noise Flag
#define BIT_FE     BIT(1)  // Framing Error Flag
#define BIT_PF     BIT(0)  // Parity Error Flag

#define BIT_TXEMPT BIT(7)  // Transmit Buffer/FIFO Empty

#define BIT_TIE    BIT(7)  // Transmitter Interrupt or DMA Transfer Enable.
#define BIT_TCIE   BIT(6)
#define BIT_RIE    BIT(5)

static void UART_isr(uint32_t intf_num)
{
  uint8_t s1;
  uint8_t c2;
  uint8_t d;
  UART_MemMapPtr  UART;
  T_UART_cbl     *pcbl;

  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];
  pcbl =&UART_cbls[intf_num];

  s1 = UART->S1;
  c2 = UART->C2;


  if (c2 & BIT_TIE)
  {
    if (s1 & BIT_TDRE)
    {
      // Овободилось место в FIFO передатчика, можно передать следующий байт
      if (pcbl->uart_tx_callback != 0) pcbl->uart_tx_callback(intf_num);
    }
  }

  if (c2 & BIT_TCIE)
  {
    if (s1 & BIT_TC)
    {
      if (pcbl->uart_tx_compl_callback != 0) pcbl->uart_tx_compl_callback(intf_num);
    }
    else
    {
      UART->C2 = UART->C2 & (~BIT_TCIE); // Реагируем на прерывание только один раз, после чего запрещаем его
    }
  }

  if (s1 & BIT_RDRF)
  {
    // Есть данные в FIFO приемника, можно принять байт
    d  = UART->D;
    if (UART->C2 & BIT_RIE)
    {
      if (pcbl->uart_rx_callback != 0) pcbl->uart_rx_callback(intf_num, d);
    }
  }
}



/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
static void UART0_err_isr(void *ptr)
{
  UART_err_isr(0);
}
static void UART1_err_isr(void *ptr)
{
  UART_err_isr(1);
}
static void UART2_err_isr(void *ptr)
{
  UART_err_isr(2);
}
static void UART3_err_isr(void *ptr)
{
  UART_err_isr(3);
}
static void UART4_err_isr(void *ptr)
{
  UART_err_isr(4);
}

static void UART_err_isr(uint32_t intf_num)
{
  uint8_t s1;
  uint8_t d;
  UART_MemMapPtr  UART;
  T_UART_cbl     *pcbl;


}

/*-----------------------------------------------------------------------------------------------------


  \param intf_num

  \return UART_MemMapPtr
-----------------------------------------------------------------------------------------------------*/
UART_MemMapPtr  K66BLEZ1_get_UART(uint32_t intf_num)
{
  if (intf_num >= UART_INTFS_NUM) return 0;
  return UART_intfs_map[intf_num];
}

/*-----------------------------------------------------------------------------------------------------


  \param intf_num

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t  K66BLEZ1_get_UART_DMA_tx_src(uint32_t intf_num)
{
  if (intf_num >= UART_INTFS_NUM) return 0;
  return UART_DMA_tx_srcs[intf_num];
}
/*-----------------------------------------------------------------------------------------------------


  \param intf_num
  \param SIM
  \param module_clock
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_activate(uint32_t intf_num)
{
  SIM_MemMapPtr  SIM = SIM_BASE_PTR;

  switch (intf_num)
  {
  case 0:
    SIM->SCGC4 |=  LSHIFT(1,  10); // UART0 Clock Gate Control. 1 Clock enabled
    SIM->SOPT5 &= ~LSHIFT(3,  2); // UART 0 receive data source select
    SIM->SOPT5 |=  LSHIFT(0,  2); // 00 UART0_RX pin

    SIM->SOPT5 &= ~LSHIFT(3,  0); // UART 0 transmit data source select
    SIM->SOPT5 |=  LSHIFT(0,  0); // 00 UART0_TX pin
    break;
  case 1:
    SIM->SCGC4 |=  LSHIFT(1,  11); // UART1 Clock Gate Control. 1 Clock enabled
    SIM->SOPT5 &= ~LSHIFT(3,  6); // UART 1 receive data source select
    SIM->SOPT5 |=  LSHIFT(0,  6); // 00 UART1_RX pin

    SIM->SOPT5 &= ~LSHIFT(3,  4); // UART 1 transmit data source select
    SIM->SOPT5 |=  LSHIFT(0,  4); // 00 UART1_TX pin
    break;
  case 2:
    SIM->SCGC4 |=  LSHIFT(1,  12); // UART2 Clock Gate Control. 1 Clock enabled
    break;
  case 3:
    SIM->SCGC4 |=  LSHIFT(1,  13); // UART3 Clock Gate Control. 1 Clock enabled
    break;
  case 4:
    SIM->SCGC1 |=  LSHIFT(1,  10); // UART4 Clock Gate Control. 1 Clock enabled
    break;
  }
}


/*-----------------------------------------------------------------------------------------------------


  \param intf_num
  \param module_clock
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_get_clock(uint32_t intf_num, uint32_t *module_clock)
{
  switch (intf_num)
  {
  case 0:
    *module_clock = BSP_SYSTEM_CLOCK;
    break;
  case 1:
    *module_clock = BSP_SYSTEM_CLOCK;
    break;
  case 2:
    *module_clock = BSP_BUS_CLOCK;
    break;
  case 3:
    *module_clock = BSP_BUS_CLOCK;
    break;
  case 4:
    *module_clock = BSP_BUS_CLOCK;
    break;
  }
}




/*-----------------------------------------------------------------------------------------------------
  Управление разрешением запрещение передатчика

  \param intf_num  - номер порта
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_Set_TX_state(uint32_t intf_num, uint8_t flag)
{
  uint8_t reg;
  UART_MemMapPtr UART;
  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];

  reg = 0
       + LSHIFT(0, 7) // TIE  | Transmitter Interrupt or DMA Transfer Enable.  | 0 TDRE interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 6) // TCIE | Transmission Complete Interrupt Enable         | 0 TC interrupt requests disabled.
       + LSHIFT(0, 5) // RIE  | Receiver Full Interrupt or DMA Transfer Enable | 0 RDRF interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 4) // ILIE | Idle Line Interrupt Enable                     | 0 IDLE interrupt requests disabled.
       + LSHIFT(1, 3) // TE   | Transmitter Enable                             | 0 Transmitter off
       + LSHIFT(0, 2) // RE   | Receiver Enable                                | 0 Receiver off.
       + LSHIFT(0, 1) // RWU  | Receiver Wakeup Control                        | 0 Normal operation.
       + LSHIFT(0, 0) // SBK  | Send Break                                     | 0 Normal transmitter operation.
  ;

  if (flag == 0) UART->C2 &= ~(reg);
  else UART->C2 |= reg;
}

/*-----------------------------------------------------------------------------------------------------
  Управление разрешением запрещение приемника

  \param intf_num  - номер порта
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_Set_RX_state(uint32_t intf_num, uint8_t flag)
{
  uint8_t reg;
  UART_MemMapPtr UART;
  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];

  reg = 0
       + LSHIFT(0, 7) // TIE  | Transmitter Interrupt or DMA Transfer Enable.  | 0 TDRE interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 6) // TCIE | Transmission Complete Interrupt Enable         | 0 TC interrupt requests disabled.
       + LSHIFT(0, 5) // RIE  | Receiver Full Interrupt or DMA Transfer Enable | 0 RDRF interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 4) // ILIE | Idle Line Interrupt Enable                     | 0 IDLE interrupt requests disabled.
       + LSHIFT(0, 3) // TE   | Transmitter Enable                             | 0 Transmitter off
       + LSHIFT(1, 2) // RE   | Receiver Enable                                | 0 Receiver off.
       + LSHIFT(0, 1) // RWU  | Receiver Wakeup Control                        | 0 Normal operation.
       + LSHIFT(0, 0) // SBK  | Send Break                                     | 0 Normal transmitter operation.
  ;

  if (flag == 0) UART->C2 &= ~(reg);
  else UART->C2 |= reg;
}

/*-----------------------------------------------------------------------------------------------------
  Запрещение или разрешение прерываний от приемника

  \param intf_num
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_Set_RX_Int(uint32_t intf_num, uint8_t flag)
{

  uint8_t reg;
  UART_MemMapPtr UART;
  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];

  reg = 0
       + LSHIFT(0, 7) // TIE  | Transmitter Interrupt or DMA Transfer Enable.  | 0 TDRE interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 6) // TCIE | Transmission Complete Interrupt Enable         | 0 TC interrupt requests disabled.
       + LSHIFT(1, 5) // RIE  | Receiver Full Interrupt or DMA Transfer Enable | 0 RDRF interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 4) // ILIE | Idle Line Interrupt Enable                     | 0 IDLE interrupt requests disabled.
       + LSHIFT(0, 3) // TE   | Transmitter Enable                             | 0 Transmitter off
       + LSHIFT(0, 2) // RE   | Receiver Enable                                | 0 Receiver off.
       + LSHIFT(0, 1) // RWU  | Receiver Wakeup Control                        | 0 Normal operation.
       + LSHIFT(0, 0) // SBK  | Send Break                                     | 0 Normal transmitter operation.
  ;

  if (flag == 0) UART->C2 &= ~(reg);
  else
  {
    UART->C2 |= reg;
  }
}

/*-----------------------------------------------------------------------------------------------------
  Запрещение или разрешение прерываний по флагу готовности передавать данные

  \param intf_num
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_Set_TX_Int(uint32_t intf_num, uint8_t flag)
{
  uint8_t reg;
  UART_MemMapPtr UART;
  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];

  reg = 0
       + LSHIFT(1, 7) // TIE  | Transmitter Interrupt or DMA Transfer Enable.  | 0 TDRE interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 6) // TCIE | Transmission Complete Interrupt Enable         | 0 TC interrupt requests disabled.
       + LSHIFT(0, 5) // RIE  | Receiver Full Interrupt or DMA Transfer Enable | 0 RDRF interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 4) // ILIE | Idle Line Interrupt Enable                     | 0 IDLE interrupt requests disabled.
       + LSHIFT(0, 3) // TE   | Transmitter Enable                             | 0 Transmitter off
       + LSHIFT(0, 2) // RE   | Receiver Enable                                | 0 Receiver off.
       + LSHIFT(0, 1) // RWU  | Receiver Wakeup Control                        | 0 Normal operation.
       + LSHIFT(0, 0) // SBK  | Send Break                                     | 0 Normal transmitter operation.
  ;

  if (flag == 0) UART->C2 &= ~(reg);
  else UART->C2 |= reg;
}

/*-----------------------------------------------------------------------------------------------------
  Запрещение или разрешение прерываний по флагу завершения отправки данных

  \param intf_num
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_Set_TXC_Int(uint32_t intf_num, uint8_t flag)
{
  uint8_t reg;
  UART_MemMapPtr UART;
  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];

  reg = 0
       + LSHIFT(0, 7) // TIE  | Transmitter Interrupt or DMA Transfer Enable.  | 0 TDRE interrupt and DMA transfer requests disabled.
       + LSHIFT(1, 6) // TCIE | Transmission Complete Interrupt Enable         | 0 TC interrupt requests disabled.
       + LSHIFT(0, 5) // RIE  | Receiver Full Interrupt or DMA Transfer Enable | 0 RDRF interrupt and DMA transfer requests disabled.
       + LSHIFT(0, 4) // ILIE | Idle Line Interrupt Enable                     | 0 IDLE interrupt requests disabled.
       + LSHIFT(0, 3) // TE   | Transmitter Enable                             | 0 Transmitter off
       + LSHIFT(0, 2) // RE   | Receiver Enable                                | 0 Receiver off.
       + LSHIFT(0, 1) // RWU  | Receiver Wakeup Control                        | 0 Normal operation.
       + LSHIFT(0, 0) // SBK  | Send Break                                     | 0 Normal transmitter operation.
  ;

  if (flag == 0)
  {
    UART->C2 &= ~(reg);
  }
  else
  {
    UART->C2 |= reg;
  }
}

/*-----------------------------------------------------------------------------------------------------


  \param intf_num
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_clear_rx_fifo(uint32_t intf_num)
{

  UART_MemMapPtr UART = UART_intfs_map[intf_num];

  UART->S1;   // Процедура очистки флагов статуса
  UART->D;

  // Очистим FIFO приемника
  UART->CFIFO |=  LSHIFT(1, 6); // RXFLUSH (wo)| Receive FIFO/Buffer Flush
  // Очистим флаги FIFO
  UART->SFIFO = 0x05;
}

/*-----------------------------------------------------------------------------------------------------


  \param intf_num

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint8_t K66BLEZ1_UART_Get_TXEMPT_flag(uint32_t intf_num)
{
  UART_MemMapPtr UART = UART_intfs_map[intf_num];

  return (UART->SFIFO & BIT(7));
}

/*-----------------------------------------------------------------------------------------------------
  \param v        - емкость FIFO передатчика
  \return uint8_t - величина сигнализирующей границы
-----------------------------------------------------------------------------------------------------*/
uint8_t K66BLEZ1_UART_Solve_TWFIFO(uint8_t v)
{
  switch (v)
  {
  case 0:
    return 0;
  case 1:
    return 3;
  case 2:
    return 7;
  case 3:
    return 15;
  case 4:
    return 31;
  case 5:
    return 63;
  case 6:
    return 127;
  case 7:
    return 0;
  default:
    return 0;
  }
}
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
void K66BLEZ1_UART_Init(uint32_t intf_num, uint32_t  baud, uint8_t  bits, uint8_t  parity, uint8_t  stops)
{
  SIM_MemMapPtr  SIM = SIM_BASE_PTR;
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
             + LSHIFT(stops,  5)              // SBNS    | Stop Bit Number Select
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
            + LSHIFT(parity, 0) // PT  | Parity Type                   | 0 Even parity.
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
               + LSHIFT(1, 2) // TXRTSPOL | Transmitter request-to-send polarity  | 1 Transmitter RTS is active high.
               + LSHIFT(1, 1) // TXRTSE   | Transmitter request-to-send enable    | 1 -  RTS asserts one bit time before the start bit is transmitted.
               + LSHIFT(0, 0) // TXCTSE   | Transmitter clear-to-send enable      |
  ;

// UART FIFO Parameters (UARTx_PFIFO)  | Default = 0
  UART->PFIFO = 0
               + LSHIFT(1, 7) // TXFE           | Transmit FIFO Enable
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

  Install_and_enable_isr(UART_int_nums[intf_num], UART_priots[intf_num], UART_isrs[intf_num]);
}


/*-----------------------------------------------------------------------------------------------------


  \param intf_num
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_enable_tx_DMA(uint32_t intf_num)
{
  UART_MemMapPtr UART = UART_intfs_map[intf_num];
// UART Control Register 5 (UARTx_C5)  | Default = 0
  UART->C5 |= 0
             + LSHIFT(1, 7) // TDMAS | Transmitter DMA Select
             + LSHIFT(0, 5) // RDMAS | Receiver Full DMA Select
  ;
}
/*-----------------------------------------------------------------------------------------------------


  \param intf_num
  \param func
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_Set_txrx_callback(uint32_t intf_num, T_uart_rx_callback rx_callback, T_uart_tx_callback tx_callback)
{
  T_UART_cbl     *pcbl;
  if (intf_num >= UART_INTFS_NUM) return;
  pcbl =&UART_cbls[intf_num];

  pcbl->uart_rx_callback = rx_callback;
  pcbl->uart_tx_callback = tx_callback;
}

/*-----------------------------------------------------------------------------------------------------


  \param intf_num
  \param tc_callback
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_Set_tc_callback(uint32_t intf_num, T_uart_tx_compl_callback tc_callback)
{
  T_UART_cbl     *pcbl;
  if (intf_num >= UART_INTFS_NUM) return;
  pcbl =&UART_cbls[intf_num];
  pcbl->uart_tx_compl_callback = tc_callback;
}


/*-----------------------------------------------------------------------------------------------------


  \param str
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_send_string(uint32_t intf_num, uint8_t *str)
{
  UART_MemMapPtr  UART;
  uint32_t        i;

  UART = K66BLEZ1_get_UART(intf_num);

  for (i=0;i< strlen((char*)str);i++)
  {
    if (UART->S1 & BIT_TDRE)
    {
      UART->D = str[i];
    }
  }
}

/*-----------------------------------------------------------------------------------------------------


  \param intf_num
-----------------------------------------------------------------------------------------------------*/
void K66BLEZ1_UART_DeInit(uint32_t intf_num)
{
  volatile uint8_t dummy;
  UART_MemMapPtr UART;
  T_UART_cbl     *pcbl;

  if (intf_num >= UART_INTFS_NUM) return;
  UART=UART_intfs_map[intf_num];

  _bsp_int_disable(UART_int_nums[intf_num]);

  pcbl =&UART_cbls[intf_num];


  UART->C2 = 0;       // Запрещаем работу передатчика и приемника

  dummy = UART->S1;   // Процедура очистки флагов статуса
  dummy = UART->D;

  UART->CFIFO = 0xC0; // Стираем данные из FIFO если они там были
  UART->SFIFO = 0x07; // Сбрасываем флаги прерываний
  UART->S2    = 0xC0; // Сбрасываем флаги прерываний

  UART->C1 = 0;
  UART->C3 = 0;
  UART->C4 = 0;
  UART->C5 = 0;
  UART->MODEM = 0;
  UART->PFIFO = 0;

  pcbl->uart_rx_callback = 0;
  pcbl->uart_tx_callback = 0;
}


