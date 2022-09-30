#include "ctype.h"
#include "stddef.h"
#include "wchar.h"
#include "stdint.h"
#include "limits.h"
#include "math.h"
#include "arm_itm.h"
#include "mqx.h"
#include "lwevent.h"
#include "mutex.h"
#include "lwmsgq.h"
#include "bsp.h"
#include "message.h"


#include "K66BLEZ1_PERIPHERIAL.h"
#include "bsp_prv.h"
#include "message.h"
#include "mutex.h"
#include "sem.h"
#include "lwmsgq.h"
#include "lwevent_prv.h"
#include "mfs.h"
#include "part_mgr.h"
#include "SEGGER_RTT.h"
#include "Debug_io.h"
#include "Inputs_controller.h"
#include "Supervisor_task.h"
#include "WatchDogCtrl.h"
#include "RTOS_utils.h"
#include "MonitorVT100.h"
#include "Monitor_serial_drv.h"
#include "Monitor_usb_drv.h"
#include "App_logger.h"

#include "MFS_man.h"
#include "LED_control.h"
#include "Task_FreeMaster.h"
#include "CRC_utils.h"
#include "CAN_communication.h"
#include "Modbus.h"


// Макросы определяются в настройках IDE
//#define    USB_1VCOM      // Конфигурация с одним виртуальным COM портом
//#define    USB_2VCOM      // Конфигурация с 2-я виртуальными COM портами

#include   "usb_device_config.h"
#include   "usb.h"
#include   "usb_device_stack_interface.h"
#include   "usb_class_cdc.h"
#include   "usb_device_descriptor.h"

#ifdef     USB_2VCOM
  #include   "usb_class_composite.h"
  #include   "usb_composite_dev.h"
  #include   "USB_Virtual_com1.h"
  #include   "USB_Virtual_com2.h"
#endif

#ifdef     USB_1VCOM
  #include   "USB_Virtual_com1.h"
#endif

#include     "USB_debug.h"


#include     "Sound_player.h"
#include     "MZTS_app.h"
#include     "Parameters.h"
#include     "MKW40_Channel.h"
#include     "MKW40_comm_channel_IDs.h"


#define PARAMS_FILE_NAME DISK_NAME"Params.ini"


#define USB_VT100  // Определяем если терминал работает через USB

//#define ENABLE_CAN_LOG  Определить если нужен LOG CAN
//#define ENABLE_ACCESS_CONTROL  Определить если нужен контроль доступа
//#define DEBUG_USB_APP

#ifdef DEBUG_USB_APP
  #define DEBUG_PRINT_USP_APP RTT_terminal_printf
#else
  #define DEBUG_PRINT_USP_APP
#endif


#define     BIT(n) (1u << n)
#define     LSHIFT(v,n) (((unsigned int)(v) << n))

#define MQX_MFS   // Пределить если используется MQX MFS
#define MFS_TEST  // Определить если компилируется процедура тестирования файловой системы MFS


#define MAIN_TASK_IDX           1
#define INPUTS_IDX              2
#define PWMO_IDX                3
#define CAN_TX_IDX              4
#define CAN_RX_IDX              5
#define MKW40_IDX               6
#define VT100_UART_IDX          7
#define VT100_USB_IDX           8
#define VT100_BLE_IDX           9
#define FREEMASTER_IDX          10
#define LIDAR_IDX               11
#define USB_IDX                 12
#define SUPERVISOR_IDX          13
#define FILELOG_IDX             14
#define BACKGR_IDX              15




// Установка приоритетов задач
// Чем больше число тем меньше приоритет.
#define ONEWIRE_PRIO            6
#define INPUTS_ID_PRIO          7
#define MAIN_TASK_PRIO          8
#define PWMO_ID_PRIO            9
#define CAN_RX_ID_PRIO          9
#define CAN_TX_ID_PRIO          9
#define MKW_ID_PRIO             9
#define USB_TASK_PRIO           10
#define PLAYER_PRIO             7
#define FREEMASTER_ID_PRIO      13
#define VT100_UART_ID_PRIO      14
#define VT100_USB_ID_PRIO       15
#define VT100_BLE_ID_PRIO       16
#define SUPRVIS_ID_PRIO         17
#define FILELOG_ID_PRIO         19 // Приоритет задачи записи лога в файл
#define GUI_TASK_PRIO           20

#define BACKGR_ID_PRIO          100


#define MAX_MQX_PRIO        BSP_DEFAULT_MQX_HARDWARE_INTERRUPT_LEVEL_MAX

// Приоритеты для ISR
// Чем меньше число тем выше приоритет ISR
// ISR с более высоким приоритетом вытесняют ISR с более низким приоритетом
// Приоритет ISR использующий сервисы RTOS не может быть меньше значения BSP_DEFAULT_MQX_HARDWARE_INTERRUPT_LEVEL_MAX!!!
#define WDT_ISR_PRIO        MAX_MQX_PRIO      //
#define DMA_ADC_PRIO        MAX_MQX_PRIO      // Проритет прерывания DMA после обработки всех каналов ADC
#define CAN_ISR_PRIO        MAX_MQX_PRIO + 1  // Приоритет процедуры прерывания от контроллера CAN шины
#define FTM1_ISR_PRIO       MAX_MQX_PRIO + 1  // Приоритет прерываний квадратичного энкодера


#define SPI0_PRIO           MAX_MQX_PRIO + 1
#define SPI1_PRIO           MAX_MQX_PRIO + 1
#define SPI2_PRIO           MAX_MQX_PRIO + 1

#define I2C0_PRIO           MAX_MQX_PRIO + 1
#define I2C1_PRIO           MAX_MQX_PRIO + 1
#define I2C2_PRIO           MAX_MQX_PRIO + 1
#define I2C3_PRIO           MAX_MQX_PRIO + 1


#define UART0_PRIO           MAX_MQX_PRIO + 1
#define UART1_PRIO           MAX_MQX_PRIO + 1
#define UART2_PRIO           MAX_MQX_PRIO + 1
#define UART3_PRIO           MAX_MQX_PRIO + 1
#define UART4_PRIO           MAX_MQX_PRIO

#define PIT0_PRIO            MAX_MQX_PRIO + 1
#define PIT1_PRIO            MAX_MQX_PRIO + 1
#define PIT2_PRIO            MAX_MQX_PRIO + 1
#define PIT3_PRIO            MAX_MQX_PRIO + 1

#define ADC_PIT_TIMER        0
#define MODBUS_PIT_TIMER     1
#define MODBUS_PIT_PRIO      MAX_MQX_PRIO + 1
#define MODBUS_DMA_PRIO      MAX_MQX_PRIO + 1


#define MX2_TASK_PIT_TIMER   2
#define MX2_TASK_PIT_PRIO    MAX_MQX_PRIO + 1

#define L550_TASK_PIT_TIMER  2
#define L550_TASK_PIT_PRIO   MAX_MQX_PRIO + 1

#define QDEC_PIT_TIMER       3
#define QDEC_PIT_TIMER_INT   INT_PIT3

#define AUDIO_PLAYER_TIMER   TPM1_BASE_PTR
#define AUDIO_DMA_PRIO       MAX_MQX_PRIO + 1



// Проритеты прерываний уровня ядра
// Чем меньше число тем выше приоритет ISR
#define WDT_KERN_PRIO       MAX_MQX_PRIO-2     //
#define DMA_ADC_KERN_PRIO   MAX_MQX_PRIO-1     // Проритет прерывания DMA после обработки всех каналов ADC
#define ADC_KERN_PRIO       MAX_MQX_PRIO-1     // Проритет прерывания ADC
#define FTM2_KERN_PRIO      MAX_MQX_PRIO-1     // Приоритет прерываний измерителя скорости с hall сенсоров
#define FTM3_KERN_PRIO      MAX_MQX_PRIO-1     // Приоритет прерываний таймера FTM3 уровня ядра
#define DMA_WS28_KERN_PRIO  MAX_MQX_PRIO-1     //
#define PORTB_KERN_PRIO     MAX_MQX_PRIO-1     // Приоритет прерываний от порта B на ктором измеряется длительность импульсов энкодера
#define PIT3_KERN_PRIO      MAX_MQX_PRIO-2     //
#define PORTE_KERN_PRIO     MAX_MQX_PRIO-1     // Приоритет прерываний от порта E на ктором измеряется длительность импульсов 1Wire



#define REF_TIME_INTERVAL   5  // Интервал времени в милисекундах на котором производлится калибровка и измерение загруженности процессора

#define CAN_TX_QUEUE        8  // Уникальный номер очереди на данном процессоре. Нельзя устанавливать меньше 8-и. Согласовать с номерами в приложении




#define NO_PULT   // На платформе нет пульта,  кнопки платформы подключены на дискретные входы

//#define dbg_printf RTT_printf
#define dbg_printf printf

// Определения для файловой системы
#define    PARTMAN_NAME   "pm:"
#define    DISK_NAME      "a:"
#define    PARTITION_NAME "pm:1"

// Определения статуса записей для логгера приложения
#define   SEVERITY_RED              0
#define   SEVERITY_GREEN            1
#define   SEVERITY_YELLOW           2
#define   SEVERITY_DEFAULT          4

#define   RES_OK     MQX_OK
#define   RES_ERROR  MQX_ERROR


#define   FMSTR_SMPS_ADC_ISR        0       // Источником сэмплов для рекордера FreeMaster является процедура прерывания ADC


#define   SD_CARD_OK                0
#define   SD_CARD_ERROR1            1
#define   SD_CARD_ERROR2            2
#define   SD_CARD_ERROR3            3
#define   SD_CARD_FS_ERROR1         4
#define   SD_CARD_FS_ERROR2         5
#define   SD_CARD_FS_ERROR3         6
#define   SD_CARD_FS_ERROR4         7
#define   SD_CARD_FS_ERROR5         8
#define   SD_CARD_FS_ERROR6         9



extern WVAR_TYPE                wvar;
extern uint32_t                 ref_time;
extern volatile uint32_t        cpu_usage;
extern volatile uint32_t        g_aver_cpu_usage;
extern T_run_average_int32_N    filter_cpu_usage;
extern uint32_t                 g_fmstr_rate_src;     // Переменная управляющая источником сэмплов для рекордера FreeMaster
extern float                    g_fmstr_smpls_period; // Период сэмплирования рекордера FreeMaster в секундах
extern uint8_t                  g_sd_card_status; // Состояние SD карты
extern MQX_FILE_PTR             com_handle;
extern MQX_FILE_PTR             partition_handle;
extern MQX_FILE_PTR             g_filesystem_handle;
extern MQX_FILE_PTR             g_sdcard_handle;
extern uint64_t                 g_card_size;
extern uint64_t                 g_fs_free_space;
extern uint8_t                  g_fs_free_space_val_ready;
extern uint8_t                  g_RTC_state;


#define  DELAY_1us    Delay_m7(25)           // 1.011     мкс при частоте 180 МГц
#define  DELAY_4us    Delay_m7(102)          // 4.005     мкс при частоте 180 МГц
#define  DELAY_8us    Delay_m7(205)          // 8.011     мкс при частоте 180 МГц
#define  DELAY_32us   Delay_m7(822)          // 32.005    мкс при частоте 180 МГц
#define  DELAY_ms(x)  Delay_m7(25714*x-1)    // 999.95*N  мкс при частоте 180 МГц

extern void Delay_m7(int cnt); // Задержка на (cnt+1)*7 тактов . Передача нуля недопускается


_mqx_int  TimeManInit(void);
void      Get_time_counters(HWTIMER_TIME_STRUCT *t);
uint32_t  Eval_meas_time(HWTIMER_TIME_STRUCT t1, HWTIMER_TIME_STRUCT t2);
uint32_t  Get_usage_time(void);
void      Init_RTC(void);
