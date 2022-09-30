#ifndef __MZTS_TASK_H
  #define __MZTS_TASK_H

  #define MX2_TASK_PERIOD            20000ul // Период цикла управления задачи в мкс

  #define   PLAYER_CMD_QUEUE         9 // Уникальный номер очереди на данном процессоре. Нельзя устанавливать меньше 8-и
                                     // Номер 8 занят задачей CAN


  #define EVENT_SENSOR_RESULTS_READY  BIT(0)
  #define EVENT_APP_TICK              BIT(1)
  #define EVENT_PLAYER_MSG_IN_QUEUE   BIT(2)

typedef struct
{
    uint8_t sensor_id[8];

} T_sensors_congig;


extern T_sensors_congig sensor_configs[];


void      MZTS_task(void);

void      Get_app_snapshot(T_app_snapshot *p_app_snapshot);
uint32_t  Get_valid_sensors_num(void);
void      Append_new_sensors_id(D1W_device *devs, uint32_t devs_num);
void      App_set_event(_mqx_uint eventmask);

#endif // PLATFORM_TASK_H



