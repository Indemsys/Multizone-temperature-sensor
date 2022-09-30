#ifndef K66BLEZ1_UART_H
#define K66BLEZ1_UART_H

#define UART_INTFS_NUM  5

#define UART_PARITY_NONE 0
#define UART_PARITY_EVEN 2
#define UART_PARITY_ODD  3

#define UART_STOP_1BIT   0
#define UART_STOP_2BIT   1



typedef void (*T_uart_tx_callback)(uint8_t portn);
typedef void (*T_uart_rx_callback)(uint8_t portn, uint8_t ch);
typedef void (*T_uart_tx_compl_callback)(uint8_t portn);


typedef struct
{

    T_uart_tx_callback          uart_tx_callback;
    T_uart_rx_callback          uart_rx_callback;
    T_uart_tx_compl_callback    uart_tx_compl_callback;

} T_UART_cbl;


extern T_UART_cbl      UART_cbls         [];
extern UART_MemMapPtr  UART_intfs_map    [];
extern INT_ISR_FPTR    UART_isrs         [];
extern int32_t         UART_priots       [];
extern int32_t         UART_int_nums     [];
extern int32_t         UART_err_int_nums [];
extern int32_t         UART_DMA_tx_srcs  [];

UART_MemMapPtr  K66BLEZ1_get_UART(uint32_t intf_num);
uint32_t        K66BLEZ1_get_UART_DMA_tx_src(uint32_t intf_num);
void            K66BLEZ1_UART_Init(uint32_t intf_num, uint32_t  baud, uint8_t  bits, uint8_t  parity, uint8_t  stops);
void            K66BLEZ1_UART_Set_TX_state(uint32_t intf_num, uint8_t flag);
void            K66BLEZ1_UART_Set_RX_state(uint32_t intf_num, uint8_t flag);
void            K66BLEZ1_UART_Set_RX_Int(uint32_t intf_num, uint8_t flag);
void            K66BLEZ1_UART_Set_TX_Int(uint32_t intf_num, uint8_t flag);
void            K66BLEZ1_UART_Set_TXC_Int(uint32_t intf_num, uint8_t flag);
void            K66BLEZ1_UART_clear_rx_fifo(uint32_t intf_num);
uint8_t         K66BLEZ1_UART_Get_TXEMPT_flag(uint32_t intf_num);
void            K66BLEZ1_UART_enable_tx_DMA(uint32_t intf_num);
void            K66BLEZ1_UART_Set_txrx_callback(uint32_t intf_num, T_uart_rx_callback rx_callback, T_uart_tx_callback tx_callback);
void            K66BLEZ1_UART_Set_tc_callback(uint32_t intf_num, T_uart_tx_compl_callback tc_callback);
void            K66BLEZ1_UART_send_string(uint32_t intf_num, uint8_t *str);
void            K66BLEZ1_UART_DeInit(uint32_t intf_num);

void            K66BLEZ1_UART_activate(uint32_t  intf_num);
void            K66BLEZ1_UART_get_clock(uint32_t intf_num, uint32_t *module_clock);

uint8_t         K66BLEZ1_UART_Solve_TWFIFO(uint8_t v);
#endif // K66BLEZ1_UART_H



