#ifndef _MZTS_APP
  #define _MZTS_APP

  #include "MZTS_DS1Wire.h"

  #define HARDWARE_VER           "MZTS"
  #define FIRMARE_VER            "1.0"


  //#define ENABLE_CAN_LOG

  #define LIGHTING_PWM_FREQ           (100000ul) // Частота ШИМ для управления подсветкой
  #define FTM_LIGHTING_MOD            (FTM_SYSCLK/(LIGHTING_PWM_FREQ))


  #define MAX_SYSSTR_LEN              20
  #define MAX_ESCSTR_LEN              20


  #define TEMP_ZONES_NUM              8

typedef struct
{

    uint8_t        sync_obj_ready;

    float          temperatures[TEMP_ZONES_NUM];
    uint8_t        sensors_state[TEMP_ZONES_NUM];
    float          temperature_aver;
    uint32_t       onew_devices_num;
    D1W_device     onew_devices[TEMP_ZONES_NUM];
    uint32_t       current_sensor;
} T_app;


typedef struct
{
    uint32_t  up_time_sec; // Секунды счетчика времени прошедшего от момента последнего сброса
    uint32_t  up_time_min; //
    uint32_t  up_time_hour; //
    uint32_t  up_time_day; //

    float     CPU_t;     // Температура микроконтроллера

    uint8_t   sd_card_state;     // Состояние SD карты. 1- отсутствует либо не работает. 0 - присутствует и работоспособна
    uint8_t   RTC_state;        // Состояние часов реального времени. 1 - не работают

    uint32_t  cpu_usage;

    int32_t   max_x_a_ndc;
    int32_t   max_y_a_ndc;
    int32_t   max_z_a_ndc;
    int32_t   aver_accel_rms;
    int32_t   max_aver_accel_rms;


    float     temperatures[TEMP_ZONES_NUM];
    uint8_t   sensors_state[TEMP_ZONES_NUM];
    float     temperature_aver;
    uint32_t  onew_devices_num;
    uint32_t  current_sensor;

    uint32_t        rtc_time;
    TIME_STRUCT     ts;
    DATE_STRUCT     tm;

    uint64_t        card_free_space;

} T_app_snapshot;

  #define FW_VERSION_STR_LEN  64

  #include "CAN_bus_defs.h"

  #include "MZTS_outputs.h"
  #include "MZTS_CAN.h"
  #include "MZTS_task.h"
  #include "MZTS_Analog_INs.h"
  #include "MZTS_Sound.h"
  #include "MZTS_FreemasterCmdMan.h"
  #include "MZTS_Params.h"
  #include "MZTS_TFT_display_control.h"
  #include "MZTS_LSM6DSO32.h"
  #include "MZTS_DS1Wire.h"
  #include "MZTS_DS1Wire_Intf.h"
  #include "MZTS_filters.h"
  #include "MZTS_SPI_bus.h"


extern char       fw_version[FW_VERSION_STR_LEN+1];
extern T_app      app;


#endif
