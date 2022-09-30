#ifndef _APP_PREINCLUDE_H_
#define _APP_PREINCLUDE_H_


#define ADVERTISED_DEVICE_NAME      "MZTS"                  // Имя устройство в пакетах Advertising. Ограничено 8-ю символами
#define GAP_DEVICE_NAME             "____________________"  // Имя устройство в GAP. 20 символов


#define MANUFACTURER_STR            "Indemsys"
#define MODEL_NUMBER_STR            "1.00"
#define SERIAL_NUMBER_STR           "001"
#define HARDWARE_REVISION           "K66BLEZ"
#define FIRMWARE_REVISION           "0.0"
#define SOFTWARE_REVISION           "____________________"     // Версия софтваре. 20 символов


#define ADDR_B1   0x13
#define ADDR_B2   0x02
#define ADDR_B3   0x03
#define ADDR_B4   0x04
#define ADDR_B5   0x05
#define ADDR_B6   0x06


/* Fast Connection Parameters used for Power Vector Notifications */
#define FAST_CONN_MIN_INTERVAL          6  // Minimum connection interval (7.5 ms). При установке меньшего значения PC возвращается к низкой скорости
#define FAST_CONN_MAX_INTERVAL          6  // Maximum connection interval (7.5 ms)
#define FAST_CONN_SLAVE_LATENCY         0
#define FAST_CONN_TIMEOUT_MULTIPLIER    0x03E8


#define gSerialManagerMaxInterfaces_c   0  // Defines Num of Serial Manager interfaces

// Конфигурация менеджерра памяти с блоками фиксированной длинны
#define POOLS_NUMBER           3

// Размер блоков должен быть кранен 4
#define POOL_1_BLOCKS_SIZE     32
#define POOL_2_BLOCKS_SIZE     48
#define POOL_3_BLOCKS_SIZE     80

#define POOL_1_BLOCKS_NUMBER   10
#define POOL_2_BLOCKS_NUMBER   25
#define POOL_3_BLOCKS_NUMBER   20

#define POOL_1_SIZE            ((sizeof(T_pool_block_header)+ POOL_1_BLOCKS_SIZE ) * POOL_1_BLOCKS_NUMBER)
#define POOL_2_SIZE            ((sizeof(T_pool_block_header)+ POOL_2_BLOCKS_SIZE ) * POOL_2_BLOCKS_NUMBER)
#define POOL_3_SIZE            ((sizeof(T_pool_block_header)+ POOL_3_BLOCKS_SIZE ) * POOL_3_BLOCKS_NUMBER)

#define POOLS_SIZE (POOL_1_SIZE + POOL_2_SIZE + POOL_3_SIZE)




#define TMR_APPLICATION_TIMERS_NUMBER     4     // Количество динамических таймеров запрашиваемых приложением
#define TMR_BLE_STACK_TIMERS_NUMBER       5     // Количество динамических таймеров запрашиваемых стеком BLE
#define gTMR_PIT_FreqMultipleOfMHZ_d      0     // Set this define TRUE if the PIT frequency is an integer number of MHZ
#define gTimestamp_Enabled_d              0     // Enables / Disables the precision timers platform component
#define gTMR_EnableLowPowerTimers         0     // Enable/Disable Low Power Timer



#define gTotalHeapSize_c                  7500  // Defines total heap size used by the OS



#define MAIN_TASK_STACK_SZ                800  // Defines main task stack size
#define TMR_TASK_STACK_SZ                 600  // 384  // Defines Size for Timer Task
#define IDLE_TASK_STACK_SZ                400
#define CONTROLLER_TASK_STACK_SZ          832  // Defines Controller task stack size
#define HOST_TASK_STACK_SZ                1100 // Defines Host task stack size
#define L2CA_TASK_STACK_SZ                600  // 472  // Defines L2cap task stack size
#define SERIAL_TASK_STACK_SZ              500  //
#define SPI_CHAN_TASK_STACK_SZ            500


#define CONTROLLER_TASK_PRIO              1
#define TMR_TASK_PRIO                     2
#define SERIAL_TASK_PRIO                  3
#define L2CA_TASK_PRIO                    4
#define HOST_TASK_PRIO                    5
#define SPI_CHAN_TASK_PRIO                6
#define MAIN_TASK_PRIO                    7
#define IDLE_TASK_PRIO                    8

#define BD_ADDR                           0x01,0x00,0x00,0x9F,0x04,0x00
#define gUseHciTransport_d                0



#define MEM_MANAGER_STATISTICS


#endif /* _APP_PREINCLUDE_H_ */


