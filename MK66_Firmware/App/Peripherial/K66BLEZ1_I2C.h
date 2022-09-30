#ifndef K66BLEZ1_I2C_H
# define K66BLEZ1_I2C_H

// Дефайны для скорости I2С шины в kbit при MULT=0, и частоте шины = 60 МГц
// Данные сняты экспериментально
#define I2C_SPEED_1733	0
#define I2C_SPEED_1637	1
#define I2C_SPEED_1553	2
#define I2C_SPEED_1475	3
#define I2C_SPEED_1404	4
#define I2C_SPEED_1370	8
#define I2C_SPEED_1340	5
#define I2C_SPEED_1255	9
#define I2C_SPEED_1229	6
#define I2C_SPEED_1158	10
#define I2C_SPEED_1094	7
#define I2C_SPEED_1074	11
#define I2C_SPEED_1000	12
#define I2C_SPEED_939	13
#define I2C_SPEED_938	16
#define I2C_SPEED_834	17
#define I2C_SPEED_750	18
#define I2C_SPEED_714	15
#define I2C_SPEED_682	19
#define I2C_SPEED_625	24
#define I2C_SPEED_577	21
#define I2C_SPEED_535	25
#define I2C_SPEED_500	22
#define I2C_SPEED_468	26
#define I2C_SPEED_416	27
#define I2C_SPEED_375	28
#define I2C_SPEED_374	32
#define I2C_SPEED_341	29
#define I2C_SPEED_312	33
#define I2C_SPEED_288	36
#define I2C_SPEED_268	34
#define I2C_SPEED_234	35
#define I2C_SPEED_187	40
#define I2C_SPEED_156	41
#define I2C_SPEED_134	42
#define I2C_SPEED_125	39
#define I2C_SPEED_117	43
#define I2C_SPEED_104	44
#define I2C_SPEED_94	45
#define I2C_SPEED_93	48
#define I2C_SPEED_78	49
#define I2C_SPEED_67	50
#define I2C_SPEED_62	47
#define I2C_SPEED_52	52
#define I2C_SPEED_50	51
#define I2C_SPEED_47	56
#define I2C_SPEED_46	53
#define I2C_SPEED_39	57
#define I2C_SPEED_33	58
#define I2C_SPEED_31	55
#define I2C_SPEED_29	59
#define I2C_SPEED_26	60
#define I2C_SPEED_23	61
#define I2C_SPEED_19	62
#define I2C_SPEED_15	63


typedef struct
{
    uint32_t        mode;        // 1 - прием, 0 - передача 
    uint8_t         addr;        // Адрес устройства которому передаются данные
    uint8_t         addr2;       // Младшая часть 10-и битнго адреса 
    uint32_t        flags;       // Флаги
    uint8_t         f_stop;      // Флаг останова обмена
    uint32_t        strt_cnt;    // Счетчик повторный стартов
    uint32_t        wrlen;       // Количество записываемых в слэйв байт
    uint8_t        *outbuf;      // Буфер записываемх байт
    uint32_t        rdlen;       // Количество читаемых из слэйва байт
    uint8_t        *inbuf;       // Буфер читаемых байт
    uint32_t        cnt_nack;    // Счетчик байт не получивших подтверждение
    uint32_t        cnt_break;   // Счетчик незавершенных транзакций обмена
    uint32_t        cnt_timeout; // Счетчик транзакций обмена не завершившихся в течении заданного времени
    LWEVENT_STRUCT  i2c_event;   // Объект синхронизации для передачи флага завершения транзакции 

} T_I2C_cbl;

typedef  enum   {I2C_7BIT_ADDR, I2C_10BIT_ADDR} T_I2C_addr_mode;


uint32_t I2C_master_init(uint32_t intf_num, uint32_t speed);

uint32_t I2C_WriteRead(uint32_t intf_num, T_I2C_addr_mode addr_mode, uint16_t addr, uint32_t wrlen, uint8_t *outbuf, uint32_t rdlen, uint8_t *inbuf, uint32_t timeout);


#endif // K66BLEZ1_I2C_H



