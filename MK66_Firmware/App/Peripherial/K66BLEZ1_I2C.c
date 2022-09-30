// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016-11-17
// 14:55:58
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "App.h"

#define I2C_INTFS_NUM  4

static void I2C0_isr(void *ptr);
static void I2C1_isr(void *ptr);
static void I2C2_isr(void *ptr);
static void I2C3_isr(void *ptr);
static void I2C_isr(uint32_t intf_num);

static T_I2C_cbl       I2C_cbls[I2C_INTFS_NUM]; // Контрольные блоки I2C
static I2C_MemMapPtr   I2C_intfs_map[I2C_INTFS_NUM] = {I2C0_BASE_PTR, I2C1_BASE_PTR, I2C2_BASE_PTR, I2C3_BASE_PTR};
static INT_ISR_FPTR    I2C_isrs[I2C_INTFS_NUM] = {I2C0_isr, I2C1_isr, I2C2_isr, I2C3_isr};
static int32_t         I2C_priots[I2C_INTFS_NUM] = {I2C0_PRIO, I2C1_PRIO, I2C2_PRIO, I2C3_PRIO};
static int32_t         I2C_int_nums[I2C_INTFS_NUM] = {INT_I2C0, INT_I2C1, INT_I2C2, INT_I2C3};

typedef struct
{
  uint32_t divisor;
  uint32_t speed;
} T_I2C_div_to_speed;

// Таблица соответствий делителя ICR и получаемой скорости (kbit/s) при MULT=0, и частоте шины = 60 МГц
// Данные сняты экспериментально
T_I2C_div_to_speed I2C_div_to_speed[]=
{
  { 0, 1733  },
  { 1, 1637  },
  { 2, 1553  },
  { 3, 1475  },
  { 4, 1404  },
  { 5, 1340  },
  { 6, 1229  },
  { 7, 1094  },
  { 8, 1370  },
  { 9, 1255  },
  {10, 1158  },
  {11, 1074  },
  {12, 1000  },
  {13, 939   },
  {14, 834   },
  {15, 714   },
  {16, 938   },
  {17, 834   },
  {18, 750   },
  {19, 682   },
  {20, 625   },
  {21, 577   },
  {22, 500   },
  {23, 416   },
  {24, 625   },
  {25, 535   },
  {26, 468   },
  {27, 416   },
  {28, 375   },
  {29, 341   },
  {30, 288   },
  {31, 234   },
  {32, 374   },
  {33, 312   },
  {34, 268   },
  {35, 234   },
  {36, 288   },
  {37, 187   },
  {38, 156   },
  {39, 125   },
  {40, 187   },
  {41, 156   },
  {42, 134   },
  {43, 117   },
  {44, 104   },
  {45, 94    },
  {46, 78    },
  {47, 62    },
  {48, 93    },
  {49, 78    },
  {50, 67    },
  {51, 50    },
  {52, 52    },
  {53, 46    },
  {54, 39    },
  {55, 31    },
  {56, 47    },
  {57, 39    },
  {58, 33    },
  {59, 29    },
  {60, 26    },
  {61, 23    },
  {62, 19    },
  {63, 15    }
};

#define I2C_FLT_STOPF         BIT(6)
#define I2C_FLT_STARTF        BIT(4)

#define I2C_C1_MST            BIT(5)
#define I2C_C1_TX             BIT(4)
#define I2C_C1_RSTA           BIT(2)
#define I2C_C1_TXAK           BIT(3)
#define I2C_C1_IICIE          BIT(6)

#define I2C_S_IICIF           BIT(1)
#define I2C_S_RXAK            BIT(0)
#define I2C_S_TCF             BIT(7)

#define I2C_EVT_END           BIT(0)  // Флаг завершения транзакции обмена



#define I2C_FLAG_10BIT_ADDR   BIT(0)
#define I2C_FLAG_REP_ADDR     BIT(1)
#define I2C_FLAG_READ_REQ     BIT(2)

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
static void I2C0_isr(void *ptr)
{
  I2C_isr(0);
}
static void I2C1_isr(void *ptr)
{
  I2C_isr(1);
}
static void I2C2_isr(void *ptr)
{
  I2C_isr(2);
}
static void I2C3_isr(void *ptr)
{
  I2C_isr(3);
}
/*-----------------------------------------------------------------------------------------------------
  Обработка прерываний в режиме мастера

  Не отлажены транзакции содержащие запись данных вместе с чтением и 10-и битная адресация!!!

  \param arg
-----------------------------------------------------------------------------------------------------*/
static void I2C_isr(uint32_t intf_num)
{
  I2C_MemMapPtr I2C;
  T_I2C_cbl     *pcbl;
  uint8_t       reg_S;
  uint8_t       reg_FLT;
  uint8_t       b;
  uint8_t       f_startbit = 0;

  //ITM_EVENT8(1,1);

  if (intf_num >= I2C_INTFS_NUM)
  {
    //ITM_EVENT8(1,11);
    return;
  }
  I2C=I2C_intfs_map[intf_num];
  pcbl =&I2C_cbls[intf_num];

  reg_S = I2C->S;
  reg_FLT = I2C->FLT;

  if (reg_FLT & I2C_FLT_STOPF)
  {
    // Получили STOP бит. Заканчиваем процедуру обмена
    I2C->FLT |= I2C_FLT_STOPF; // Сбрасываем бит I2C_STOPF
    pcbl->strt_cnt = 0;
    pcbl->f_stop   = 1;
    I2C->C1 &= ~I2C_C1_IICIE; // Запрещаем прерывания
    _lwevent_set(&pcbl->i2c_event, I2C_EVT_END);
    //ITM_EVENT8(1,10);
  }
  if (reg_FLT & I2C_FLT_STARTF)
  {
    I2C->FLT |= I2C_FLT_STARTF; // Сбрасываем бит I2C_STARTF
    pcbl->strt_cnt++;
    if (pcbl->strt_cnt > 1)
    {
      // Повторный старт. В данной реализации ничего не вызывает.
      // Появляется после окнчания записи и перед началом фазы приема
    }
    else
    {
      f_startbit = 1;
    }
    //ITM_EVENT8(1,2);
  }

  // Если передача предыдущего байта завершена и обмен не блокирован то продолжать обмен

  if ((f_startbit == 0) && (pcbl->f_stop  == 0)) // На флаг I2C_S_TCF не обращаем внимания, поскольку он взводится видимо когда окончена передача из буфера сдвига
  {
    // Проверяем флаг подтверждения RXAK
    if ((I2C->C1 & I2C_C1_TX) &&  (reg_S & I2C_S_RXAK) && (reg_S &  I2C_S_TCF))
    {
      // Не получили подтверждения при передаче, останавливаем передачу.
      I2C->C1 &= ~I2C_C1_MST;  // Передаем STOP бит
      I2C->C1 &= ~I2C_C1_TX;   // Переключаемся на прием
      pcbl->cnt_nack++;        // Увеличиваем счетчик отсутствия подтверждения
      pcbl->f_stop   = 2;      // Блокируем дальнейший обмен

      //ITM_EVENT8(1,3);

    }

    if (pcbl->f_stop == 0)
    {
      if (pcbl->flags & I2C_FLAG_10BIT_ADDR)
      {
        // Передаем второй байт 10-и битного адреса
        pcbl->flags &= ~I2C_FLAG_10BIT_ADDR;
        if (pcbl->rdlen != 0) pcbl->flags |=I2C_FLAG_REP_ADDR; // Если надо принимать с 10-ю битной адресацией, то установить флаг необходимости повторно переадать адрес
        I2C->D = pcbl->addr2;
        if ((pcbl->wrlen == 0) && (pcbl->rdlen != 0))
        {
          // Если нет данных для передачи, но требуются данные для приема, то отправляем повторный старт, чтобы начать прием
          I2C->C1 |= I2C_C1_RSTA + I2C_C1_TXAK; // Посылаем бит рестарта и установливаем флаг подтверждения принимаемых байт
          I2C->C1 &=~(I2C_C1_TX);               // Переключаемся на прием
        }
        //ITM_EVENT8(1,4);

      }
      else if (pcbl->wrlen != 0)
      {
        // Передаем данные
        b =*pcbl->outbuf++;
        //ITM_EVENT8(3,b);
        I2C->D = b;
        pcbl->wrlen--;
        if ((pcbl->wrlen == 0) && (pcbl->rdlen != 0))
        {
          I2C->C1 |= I2C_C1_RSTA + I2C_C1_TXAK; // Посылаем бит повторного старта и установливаем флаг подтверждения принимаемых байт
        }
        //ITM_EVENT8(1,5);

      }
      else if (pcbl->flags & I2C_FLAG_REP_ADDR)
      {
        // Повторно передаем адрес в случае 10-и битного адреса

        pcbl->flags &= ~I2C_FLAG_REP_ADDR;
        I2C->D =(pcbl->addr<<1) | 1; // Здесь установлен флаг чтений
        //ITM_EVENT8(1,6);

      }
      else if (pcbl->flags & I2C_FLAG_READ_REQ)
      {
        // Здесь когда отправден последний байт на запись и надо переключится на прием
        pcbl->flags &= ~I2C_FLAG_READ_REQ;
        I2C->C1 &=~(I2C_C1_TX);                       // Переключаемся на прием
        I2C->D;                                       // Пустое чтение чтобы инициироваь процесс приема
        if (pcbl->rdlen == 1) I2C->C1 |= I2C_C1_TXAK; // Последний принятый байт не подтвержадем, поэтому предварительно сбрасываем флаг подтверждения
        else  I2C->C1 &= ~I2C_C1_TXAK;

        //ITM_EVENT8(1,7);

      }
      else if (pcbl->rdlen != 0)
      {
        // Получаем данные

        pcbl->rdlen--;
        if (pcbl->rdlen == 0)
        {
          I2C->C1 |=  I2C_C1_TX;   // Переключаемся на передачу
          I2C->C1 &= ~I2C_C1_MST;  // Отправляем STOP бит
        }
        b = I2C->D;
        //ITM_EVENT8(4,b);
        if (pcbl->rdlen == 1) I2C->C1 |= I2C_C1_TXAK; // Последний принятый байт не подтвержадем, поэтому предварительно сбрасываем флаг подтверждения
        *pcbl->inbuf = b;
        pcbl->inbuf++;
        //ITM_EVENT8(1,8);

      }
      else
      {

        // Тут завершен прием или передача.
        I2C->C1 |=  I2C_C1_TX;   // Переключаемся на передачу
        I2C->C1 &= ~I2C_C1_MST;  // Отправляем STOP бит
        //ITM_EVENT8(1,9);

      }
    }
  }

  I2C->S = reg_S; // Сбрасываем IICIF и ARBL если они были установлены

  DELAY_1us;

  //ITM_EVENT8(1,0);

}


/*-----------------------------------------------------------------------------------------------------
 I2C тактируется от частоты шины BSP_BUS_CLOCK (60 МГц)


  \param intf_num    - номер интерфейса
  \param speed       - параметр скорости

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t I2C_master_init(uint32_t intf_num, uint32_t speed)
{
  I2C_MemMapPtr I2C;
  T_I2C_cbl     *pcbl;

  if (intf_num >= I2C_INTFS_NUM) return MQX_ERROR;
  I2C=I2C_intfs_map[intf_num];
  pcbl =&I2C_cbls[intf_num];


  pcbl->strt_cnt = 0;
  pcbl->mode     = 0;
  pcbl->addr     = 0;
  pcbl->addr2    = 0;
  pcbl->flags    = 0;
  pcbl->f_stop   = 0;
  pcbl->strt_cnt = 0;
  pcbl->wrlen    = 0;
  pcbl->outbuf   = 0;
  pcbl->rdlen    = 0;
  pcbl->inbuf    = 0;
  pcbl->cnt_nack = 0;


  switch (intf_num)
  {
  case 0:
    SIM_SCGC4 |= BIT(6);
    break;
  case 1:
    SIM_SCGC4 |= BIT(7);
    break;
  case 2:
    SIM_SCGC1 |= BIT(6);
    break;
  case 3:
    SIM_SCGC1 |= BIT(7);
    break;
  }
  I2C->C1 = 0; // Выключаем модуль
  DELAY_ms(1);

  I2C->C1 = 0 // I2C Control Register 1
           + LSHIFT(1, 7) // IICEN     | Enables I2C module operation.  | 1 Enabled
           + LSHIFT(0, 6) // IICIE     | Enables I2C interrupt requests | 1 Enabled
           + LSHIFT(0, 5) // MST       | Master Mode Select             | Установка бита генерирует START сигнал, сброс генерирует STOP сигнал
           + LSHIFT(0, 4) // TX        | Transmit Mode Select           | 1 Transmit
           + LSHIFT(0, 3) // TXAK      | Transmit Acknowledge Enable    |
           + LSHIFT(0, 2) // RSTA (wo) | Repeat START                   | Writing 1 to this bit generates a repeated START condition provided it is the current master.
           + LSHIFT(0, 1) // WUEN      | Wakeup Enable                  | 0 Normal operation. No interrupt generated when address matching in low power mode.
           + LSHIFT(0, 0) // DMAEN     | DMA Enable                     | 0 All DMA signalling disabled.
  ;

  I2C->A1 = 0  // I2C Address Register 1
           + LSHIFT(0,  1) // AD[7:1] Address | Contains the primary slave address used by the I2C module when it is addressed as a slave.
           + LSHIFT(0,  0) // Reserved
  ;
  I2C->F = 0   // I2C Frequency Divider register
          + LSHIFT(0,   6)    // 7:6 MULT | Defines the multiplier factor (mul). This factor is used along with the SCL divider to generate the I2C baud rate.
          + LSHIFT(speed,  0) // 5:0 ICR  | Prescales the I2C module clock for bit rate selection
  ;

  I2C->S = 0 // I2C Status register
          + LSHIFT(0, 7) // TCF   (ro)  | Transfer Complete Flag | Этот флаг в нулу тлько пока передается байт по I2C
          + LSHIFT(0, 6) // IAAS        | Addressed As A Slave   | Этот флаг устанавливается когда принятый адресс соответствует заданному
          + LSHIFT(0, 5) // BUSY  (ro)  | Bus Busy               | Это флаг остается установленным между приемом бита STRAT и приемом бита STOP
          + LSHIFT(1, 4) // ARBL  (w1c) | Arbitration Lost       |
          + LSHIFT(0, 3) // RAM         | Range Address Match
          + LSHIFT(0, 2) // SRW   (r0)  | Slave Read/Write
          + LSHIFT(1, 1) // IICIF (w1c) | Interrupt Flag
          + LSHIFT(0, 0) // RXAK  (ro)  | Receive Acknowledge
  ;
  ;I2C->D = 0; // I2C Data I/O register

  I2C->C2 = 0 // I2C Control Register 2
           + LSHIFT(0, 7) // GCAEN           | General Call Address Enable
           + LSHIFT(0, 6) // ADEXT           | Address Extension
           + LSHIFT(0, 5) // HDRS            | High Drive Select
           + LSHIFT(0, 4) // SBRC            | Slave Baud Rate Control
           + LSHIFT(0, 3) // RMEN            | Range Address Matching Enable
           + LSHIFT(0, 0) // [2:0] AD[10:8]  | Slave Address. Contains the upper three bits of the slave address in the 10-bit address scheme
  ;

  I2C->FLT = 0 // I2C Programmable Input Glitch Filter register
            + LSHIFT(0, 7) // SHEN         | Stop Hold Enable.
            + LSHIFT(0, 6) // STOPF  (w1c) | I2C Bus Stop Detect Flag
            + LSHIFT(1, 5) // SSIE         | I2C Bus Stop or Start Interrupt Enable
            + LSHIFT(0, 4) // STARTF (w1c) | I2C Bus Start Detect Flag
            + LSHIFT(4, 0) // [3:0] FLT    | I2C Programmable Filter Factor
  ;

  I2C->RA = 0 // I2C Range Address register
           + LSHIFT(0, 1) // [7:1] RAD | Range Slave Address
           + LSHIFT(0, 0) // Reserved
  ;


  I2C->SMB = 0 // I2C SMBus Control and Status register
            + LSHIFT(0, 7) // FACK    | Fast NACK/ACK Enable
            + LSHIFT(0, 6) // ALERTEN | SMBus Alert Response Address Enable
            + LSHIFT(0, 5) // SIICAEN | Second I2C Address Enable
            + LSHIFT(0, 4) // TCKSEL  | Timeout Counter Clock Select
            + LSHIFT(0, 3) // SLTF    | SCL Low Timeout Flag
            + LSHIFT(0, 2) // SHTF1   | SCL High Timeout Flag 1
            + LSHIFT(0, 1) // SHTF2   | SCL High Timeout Flag 2
            + LSHIFT(0, 0) // SHTF2IE | SHTF2 Interrupt Enable
  ;


  I2C->A2 = 0 // I2C Address Register 2
           + LSHIFT(0x61, 1) // [7:1] SAD | SMBus Address
           + LSHIFT(0, 0) // Reserved
  ;

  I2C->SLTH = 0; // I2C SCL Low Timeout Register High. Most significant byte of SCL low timeout value that determines the timeout period of SCL low.
  I2C->SLTL = 0; // I2C SCL Low Timeout Register Low. Least significant byte of SCL low timeout value that determines the timeout period of SCL low.

  Install_and_enable_isr(I2C_int_nums[intf_num], I2C_priots[intf_num], I2C_isrs[intf_num]);

  if (pcbl->i2c_event.VALID == 0)
  {
    if (_lwevent_create(&pcbl->i2c_event, LWEVENT_AUTO_CLEAR) != MQX_OK) return MQX_ERROR;
  }

  return MQX_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Запись-чтение в режме мастера

  \param intf_num
  \param addr_mode
  \param addr
  \param wrlen
  \param outbuf
  \param rdlen
  \param inbuf
  \param timeout

  \return int32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t I2C_WriteRead(uint32_t intf_num, T_I2C_addr_mode addr_mode, uint16_t addr, uint32_t wrlen, uint8_t *outbuf, uint32_t rdlen, uint8_t *inbuf, uint32_t timeout)
{
  I2C_MemMapPtr I2C;
  T_I2C_cbl     *pcbl;
  uint8_t       d;
  uint32_t      res;

  if ((wrlen == 0) && (rdlen == 0)) return MQX_ERROR;
  if (intf_num >= I2C_INTFS_NUM) return MQX_ERROR;


  I2C=I2C_intfs_map[intf_num];
  pcbl =&I2C_cbls[intf_num];

  pcbl->wrlen    = wrlen;
  pcbl->outbuf   = outbuf;
  pcbl->rdlen    = rdlen;
  pcbl->inbuf    = inbuf;
  pcbl->flags    = 0;
  pcbl->f_stop   = 0;
  if (rdlen != 0) pcbl->flags |= I2C_FLAG_READ_REQ;
  if (addr_mode == I2C_10BIT_ADDR)
  {
    pcbl->flags    |= I2C_FLAG_10BIT_ADDR;
    pcbl->addr2    = addr & 0xFF;
    addr =((addr>>8) & 0x03) | 0x78;
    pcbl->addr     = addr;
    d =(pcbl->addr<<1); // Здесь установлен флаг записи, в 10-и битной адресации тут всегда должен быть флаг записи
  }
  else
  {
    pcbl->addr2    = 0;
    pcbl->addr     = addr & 0xFF;
    if (wrlen == 0) d =(pcbl->addr<<1) | 1; // Здесь установлен флаг чтения
    else d =(pcbl->addr<<1);                // Здесь установлен флаг записи
  }
  __disable_interrupt();
  I2C->FLT = I2C->FLT;                   // Сбрасываем флаги STOPF и STARTF
  I2C->S   = I2C->S;                     // Сбрасываем флаги ARBL и IICIF
  I2C->C1 |= I2C_C1_TX + I2C_C1_IICIE;   // Переключаемся на передачу и разрешаем прерывания
  I2C->C1 |= I2C_C1_MST;                 // Переходим в режим мастера, генерируется START сигнал
  I2C->D = d;                            // Предаем байт адреса
  __enable_interrupt();

  res = _lwevent_wait_ticks(&pcbl->i2c_event, I2C_EVT_END, FALSE, ms_to_ticks(10));
  if (res != MQX_OK)
  {
    pcbl->cnt_timeout++;
    return res;   // Ошибка при ожидании флага или таймаут
  }
  if ((pcbl->wrlen != 0) || (pcbl->rdlen != 0))
  {
    pcbl->cnt_break++;
    return MQX_ERROR;     // Не все данные переданы
  }
  return MQX_OK;
}
