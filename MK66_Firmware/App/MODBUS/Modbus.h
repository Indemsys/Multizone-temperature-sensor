#ifndef  __MODBUS_H
#define  __MODBUS_H

#include "stdint.h"
#include <Modbus_config.h>
#include <Modbus_defs.h>


typedef  struct  modbus_ch
{
    uint8_t      modbus_channel;                    // Канал MODBUS. Каждый канал работает на отдельном UART и может быть мастером или слэйвом

    uint8_t      slave_node_addr;                   //  Адрес этого узла если работаем в режиме слэйва
    uint8_t      uart_number;
    uint32_t     baud_rate;
    uint8_t      parity_chek;
    uint8_t      bits_num;
    uint8_t      stop_bits;
    uint8_t      rtu_ascii_mode;                    // режим ASCII когда = MODBUS_MODE_ASCII, режим RTU когда = MODBUS_MODE_RTU
    uint8_t      master_slave_flag;                 // Слэйв когда = MODBUS_SLAVE, мастер когда = MODBUS_MASTER

    T_DMA_cbl    dma_cbl;                           // Управляющая структура DMA канала   
#if (MODBUS_CFG_RTU_EN == 1)
    uint8_t      rtu_timeout_en;                    // Флаг разрешения контроля таймаута ожидания пакета RTU
    uint16_t     rtu_timeout;                       // Таймаут ожидания пакета RTU в тиках таймера
    uint16_t     rtu_timeout_counter;               // Счетчик тиков тамера для отслеживания таймаута RTU
#endif

    uint32_t     rx_timeout;                        // Тайм аут ожидания приема данных в мс

    uint32_t     recv_bytes_count;                  // Счетчик принятых байт
    uint16_t     rx_packet_size;                    // Размер принятого пакета
    uint8_t      *rx_packet_ptr;                    // Указатель на пакет принимаемых данных
    uint8_t      rx_buf[MODBUS_CFG_BUF_SIZE];       // Буфер принимаемого пакета данных

    uint32_t     sent_bytes_count;                  // Счетчик отправленных байт
    uint16_t     tx_packet_size;                    // Размер посылаемого пакета
    uint8_t      *tx_packet_ptr;                    // Указатель на пакет посылаемых данных
    uint8_t      tx_buf[MODBUS_CFG_BUF_SIZE];       // Буфер посылаемого пакета данных

    uint8_t      rx_frame_buf[MODBUS_CFG_BUF_SIZE]; // Промежуточный буфер хранения поступивших данных. В него копируются данные из rx_buf кроме адреса узла и CRC
    uint16_t     rx_data_cnt;                       // Счетчик обработанных данных
    uint16_t     rx_packet_crc_ref;                 // Значение crc принятое с пакетом
    uint16_t     rx_packet_crc;                     // Расчитнанное CRC принятого пакета

    uint8_t      tx_frame_buf[MODBUS_CFG_BUF_SIZE]; // Промежуточный буфер хранения отправляемых данных. Из него копируются данные в tx_buf кроме CRC
    uint16_t     tx_data_cnt;                       // Количество данных в пакете для отправки
    uint16_t     tx_packet_crc;                     // crc пакета высылаемых данных

    uint32_t     last_err;
    uint32_t     err_cnt;
    uint8_t      last_except_code; 

#if (MODBUS_ANS_LATENTION_STAT == 1)
    MQX_TICK_STRUCT  time_stump;
    uint32_t     ans_min_time;
    uint32_t     ans_max_time;
#endif
}
T_MODBUS_ch;



#if (MODBUS_CFG_RTU_EN == 1)

extern   uint32_t        rtu_timer_tick_freq;
extern   uint32_t        rtu_timer_tick_counter;

#endif

extern   uint8_t         modbus_cbls_counter;
extern   T_MODBUS_ch     modbus_cbls[MODBUS_CFG_MAX_CH];


void          Modbus_module_init(uint32_t  freq);
T_MODBUS_ch*  Modbus_channel_init(uint8_t  node_addr, uint8_t  master_slave, uint32_t  rx_timeout, uint8_t  modbus_mode, uint8_t  port_nbr, uint32_t  baud, uint8_t  bits, uint8_t  parity, uint8_t  stops);
void          MB_set_rx_timeout(T_MODBUS_ch  *pch, uint32_t  timeout);
void          MB_set_mode(T_MODBUS_ch  *pch, uint8_t  master_slave, uint8_t  mode);
void          MB_set_node_addr(T_MODBUS_ch  *pch, uint8_t  addr);
void          MB_set_UART_port(T_MODBUS_ch  *pch, uint8_t  port_nbr);
void          Modbus_module_deinit(void);

#if (MODBUS_CFG_ASCII_EN == 1)
void          MB_ASCII_rx_byte(T_MODBUS_ch   *pch, uint8_t   rx_byte);
#endif

#if (MODBUS_CFG_RTU_EN == 1)
void          MB_RTU_rx_byte(T_MODBUS_ch   *pch, uint8_t   rx_byte);
void          MB_RTU_TmrReset(T_MODBUS_ch   *pch);
void          MB_RTU_TmrResetAll(void);
void          MB_RTU_packet_end_detector(void);
#endif

void          MB_rx_byte(T_MODBUS_ch   *pch, uint8_t   rx_byte);
void          MB_slave_rx_task(T_MODBUS_ch   *pch);

#if (MODBUS_CFG_ASCII_EN == 1)
uint8_t       MB_ASCII_Rx(T_MODBUS_ch   *pch);
void          MB_M_send_ASCII_packet(T_MODBUS_ch   *pch);
#endif


#if (MODBUS_CFG_RTU_EN == 1)
uint8_t       MB_RTU_Rx(T_MODBUS_ch   *pch);
void          MB_M_send_RTU_packet(T_MODBUS_ch   *pch);
#endif


void          MB_OS_Init(void);
void          MB_OS_deinit(void);
void          MB_OS_packet_end_signal(T_MODBUS_ch  *pch);
void          MB_OS_wait_rx_packet_end(T_MODBUS_ch *pch, uint16_t *perr);


#if (MODBUS_CFG_ASCII_EN == 1)
uint8_t*      MB_ASCII_BinToHex(uint8_t  value, uint8_t *pbuf);
uint8_t       MB_ASCII_HexToBin(uint8_t *phex);
uint8_t       MB_ASCII_RxCalcLRC(T_MODBUS_ch  *pch);
uint8_t       MB_ASCII_TxCalcLRC(T_MODBUS_ch  *pch, uint16_t tx_bytes);
#endif


#if (MODBUS_CFG_RTU_EN == 1)
uint16_t      MB_RTU_CalcCRC(T_MODBUS_ch  *pch);
uint16_t      MB_RTU_TxCalcCRC(T_MODBUS_ch  *pch);
uint16_t      MB_RTU_RxCalcCRC(T_MODBUS_ch  *pch);
#endif


#if (MODBUS_CFG_FC01_EN == 1)
uint8_t       MB_CoilRd(uint16_t   coil, uint16_t  *perr);
#endif

#if (MODBUS_CFG_FC05_EN == 1)
void          MB_CoilWr(uint16_t   coil, uint8_t  coil_val, uint16_t  *perr);
#endif

#if (MODBUS_CFG_FC02_EN == 1)
uint8_t       MB_DIRd(uint16_t   di, uint16_t  *perr);
#endif

#if (MODBUS_CFG_FC04_EN == 1)
uint16_t      MB_InRegRd(uint16_t   reg, uint16_t  *perr);

float         MB_InRegRdFP(uint16_t   reg, uint16_t  *perr);
#endif

#if (MODBUS_CFG_FC03_EN == 1)
uint16_t      MB_HoldingRegRd(uint16_t   reg, uint16_t  *perr);

float         MB_HoldingRegRdFP(uint16_t   reg, uint16_t  *perr);
#endif

#if (MODBUS_CFG_FC06_EN == 1) || (MODBUS_CFG_FC16_EN == 1)
void          MB_HoldingRegWr(uint16_t   reg, uint16_t   reg_val_16, uint16_t  *perr);

void          MB_HoldingRegWrFP(uint16_t   reg, float     reg_val_fp, uint16_t  *perr);
#endif

#if (MODBUS_CFG_FC20_EN == 1)
uint16_t      MB_FileRd(uint16_t   file_nbr, uint16_t   record_nbr, uint16_t   ix, uint8_t   record_len, uint16_t  *perr);
#endif

#if (MODBUS_CFG_FC21_EN == 1)
void          MB_FileWr(uint16_t   file_nbr, uint16_t   record_nbr, uint16_t   ix, uint8_t   record_len, uint16_t   value, uint16_t  *perr);
#endif


void          MB_BSP_deinit(void);
void          MB_BSP_config_UART(T_MODBUS_ch   *pch,  uint8_t  uart_nbr, uint32_t  baud, uint8_t  bits, uint8_t  parity, uint8_t  stops);
void          MB_BSP_send_packet(T_MODBUS_ch  *pch);
void          MB_BSP_stop_receiving(T_MODBUS_ch  *pch);

#if (MODBUS_CFG_RTU_EN == 1)

void          MB_BSP_timer_init(void);
void          MB_BSP_timer_deinit(void);
void          MB_RTU_TmrISR_Handler(void);

#endif


#if (MODBUS_CFG_SLAVE_EN == 1)

void          MB_S_rx_task(T_MODBUS_ch   *pch);   // Задача слэйва. Требует реализации!

  #if (MODBUS_CFG_FC08_EN == 1)
void          MB_S_StatInit(T_MODBUS_ch   *pch);  // Функция статитстики для задачи слэйва. Требует реализации!
  #endif
#endif


#if (MODBUS_CFG_MASTER_EN == 1)

  #if (MODBUS_CFG_FC01_EN == 1)
uint16_t      MB_M_FC01_coils_read(T_MODBUS_ch   *pch, uint8_t   slave_addr, uint16_t   start_addr, uint8_t  *p_coil_tbl, uint16_t   nbr_coils);
  #endif

  #if (MODBUS_CFG_FC02_EN == 1)
uint16_t      MB_M_FC02_input_read(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, uint8_t  *p_di_tbl, uint16_t   nbr_di);
  #endif

  #if (MODBUS_CFG_FC03_EN == 1)
uint16_t      MB_M_FC03_hold_regs_read(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, uint16_t  *p_reg_tbl, uint16_t   nbr_regs);
  #endif

  #if (MODBUS_CFG_FC03_EN == 1) && (MODBUS_CFG_FP_EN   == 1)
uint16_t      MB_M_FC03_hold_reg_fp_read(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, float    *p_reg_tbl, uint16_t   nbr_regs);
  #endif

  #if (MODBUS_CFG_FC04_EN == 1)
uint16_t      MB_M_FC04_input_reg_read(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, uint16_t  *p_reg_tbl, uint16_t   nbr_regs);
  #endif

  #if (MODBUS_CFG_FC05_EN == 1)
uint16_t      MB_M_FC05_single_coil_write(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, uint8_t  coil_val);
  #endif

  #if (MODBUS_CFG_FC06_EN == 1)
uint16_t      MB_M_FC06_single_reg_write(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, uint16_t   reg_val);
  #endif

  #if (MODBUS_CFG_FC06_EN == 1) && (MODBUS_CFG_FP_EN   == 1)
uint16_t      MB_M_FC06_single_reg_fp_write(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, float     reg_val_fp);
  #endif

  #if (MODBUS_CFG_FC08_EN == 1)
uint16_t      MB_M_FC08_diagnostic(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   fnct, uint16_t   fnct_data, uint16_t  *pval);
  #endif

  #if (MODBUS_CFG_FC15_EN == 1)
uint16_t      MB_M_FC15_coils_write(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, uint8_t  *p_coil_tbl, uint16_t   nbr_coils);
  #endif

  #if (MODBUS_CFG_FC16_EN == 1)
uint16_t      MB_M_FC16_registers_write(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, uint16_t  *p_reg_tbl, uint16_t   nbr_regs);
  #endif

  #if (MODBUS_CFG_FC16_EN == 1) && (MODBUS_CFG_FP_EN   == 1)
uint16_t      MB_M_FC16_registers_fp_write(T_MODBUS_ch   *pch, uint8_t   slave_node, uint16_t   slave_addr, float    *p_reg_tbl, uint16_t   nbr_regs);
  #endif


uint16_t  MB_M_FC17_regs_write_read(T_MODBUS_ch *pch, uint8_t slave_node, uint16_t wr_addr, uint16_t *wr_regs, uint16_t wr_num, uint16_t rd_addr, uint16_t *rd_regs, uint16_t rd_num);

#endif

#endif

