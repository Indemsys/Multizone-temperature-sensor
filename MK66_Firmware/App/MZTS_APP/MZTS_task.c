// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2018.09.18
// 14:25:59
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"


static LWEVENT_STRUCT appl_lwev;
T_app                 app;
T_app_snapshot        app_snapshot;
char                  fw_version[FW_VERSION_STR_LEN+1];


T_sensors_congig sensor_configs[TEMP_ZONES_NUM];


char *sensor_ids[TEMP_ZONES_NUM] =
{
  (char *)wvar.zone1_sensor_id,
  (char *)wvar.zone2_sensor_id,
  (char *)wvar.zone3_sensor_id,
  (char *)wvar.zone4_sensor_id,
  (char *)wvar.zone5_sensor_id,
  (char *)wvar.zone6_sensor_id,
  (char *)wvar.zone7_sensor_id,
  (char *)wvar.zone8_sensor_id,
};

uint8_t f_skip[TEMP_ZONES_NUM];
char      fname[128];
char      results_string[256];


/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void Init_sensors_config(void)
{
  char      str[17];
  memset(sensor_configs, 0, sizeof(sensor_configs));
  for (uint32_t k=0; k < TEMP_ZONES_NUM; k++)
  {
    strncpy(str, sensor_ids[k], 17);
    if (strlen(str) == 16)
    {
      char byte_str[3];
      byte_str[2] = 0;
      for (uint32_t i=0; i < 8; i++)
      {
        byte_str[0] = str[i * 2];
        byte_str[1] = str[i * 2+1];
        sensor_configs[k].sensor_id[i] = strtol(byte_str, 0, 16);
      }
    }
  }
}

/*-----------------------------------------------------------------------------------------------------
  Возвращаем 1  если идентификатор не нулевой

  \param arr

  \return uint8_t
-----------------------------------------------------------------------------------------------------*/
static uint32_t Is_id_not_empty(uint8_t *arr)
{
  for (uint32_t n=0; n < 8; n++)
  {
    if (arr[n] != 0)
    {
      return 1;
    }
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t Get_valid_sensors_num(void)
{
  uint32_t  cnt;

  cnt = 0;
  for (uint32_t i=0; i < TEMP_ZONES_NUM; i++)
  {
    if (Is_id_not_empty(sensor_configs[i].sensor_id) == 0) break;
    else cnt++;
  }
  return cnt;
}


/*-----------------------------------------------------------------------------------------------------
  Просматириваем все идентификаторы обраруженных устройств
  Если идентификатора устройства нет среди идентификаторов в настройках то добавить его в настройки

  \param devs
  \param devs_num
-----------------------------------------------------------------------------------------------------*/
void Append_new_sensors_id(D1W_device *devs, uint32_t devs_num)
{
  char     str[17];
  uint32_t not_z;
  uint8_t  found;

  memset(f_skip,0,TEMP_ZONES_NUM);

  // Проходим по всем обнаруженным устройствам
  for (uint32_t k=0; k < devs_num; k++)
  {
    found = 0;
    not_z = 0;
    // проходим по всем идентификаторам из настроек
    for (uint32_t i=0; i < TEMP_ZONES_NUM; i++)
    {
      if (f_skip[i] == 1)
      {
        not_z = 1;  // Прекращаем поиск если этот идентификатор уже совпал с одним из обнаруженных устройств
        continue;
      }
      not_z = Is_id_not_empty(sensor_configs[i].sensor_id); // Прекращаем поиск если этот идентификатор не нулевой

      if (not_z)
      {
        // Сравниваем с идентификатором обнаруженного устройства
        if (memcmp(sensor_configs[i].sensor_id, devs[k].id_arr, 8) == 0)
        {
           // идентификаторы совпали
          f_skip[i] = 1;
          found     = 1;
          break;
        }
      }
      else
      {
        break; // Прекращаем поск среди идентификаторов в настройках после обнаружения первого нулевого
      }
    }

    if (found == 0)
    {
      // Этот идентификатор устройства не наден в настройках. Записать его в настройки и сохранить
      uint32_t n = Get_valid_sensors_num();
      sprintf(str, "%02X%02X%02X%02X%02X%02X%02X%02X", devs[k].id_arr[0], devs[k].id_arr[1], devs[k].id_arr[2], devs[k].id_arr[3], devs[k].id_arr[4], devs[k].id_arr[5], devs[k].id_arr[6], devs[k].id_arr[7]);
      strcpy(sensor_ids[n],str);
      AppLog_pend_saving_params();
    }

  }

}


/*-----------------------------------------------------------------------------------------------------
  Передаем событие в основное приложение
-----------------------------------------------------------------------------------------------------*/
void App_set_event(_mqx_uint eventmask)
{
  _lwevent_set(&appl_lwev, eventmask);
}

/*-----------------------------------------------------------------------------------------------------


  \param

  \return int32_t
-----------------------------------------------------------------------------------------------------*/
static int32_t Create_sync_objects(void)
{
  _mqx_uint res;

  app.sync_obj_ready = 0;
  res = _lwevent_create(&appl_lwev, LWEVENT_AUTO_CLEAR); // Все события автоматически сбрасываемые
  if (res != MQX_OK) return res;

  app.sync_obj_ready = 1;
  return MQX_OK;
}

/*-----------------------------------------------------------------------------------------------------
  Ожидание события
-----------------------------------------------------------------------------------------------------*/
_mqx_uint Wait_for_app_event(uint32_t ticks, uint32_t evt_mask)
{
  // Ожидаем все возможные события
  if (_lwevent_wait_ticks(&appl_lwev, evt_mask, FALSE, ticks) == MQX_OK)
  {
    return _lwevent_get_signalled();
  }
  return 0;
}

/*-----------------------------------------------------------------------------------------------------


  \param p_app_snapshot
-----------------------------------------------------------------------------------------------------*/
void Get_app_snapshot(T_app_snapshot *p_app_snapshot)
{
  memcpy(&p_app_snapshot->temperatures,&app.temperatures, sizeof(app.temperatures));
  memcpy(&p_app_snapshot->sensors_state,&app.sensors_state, sizeof(app.sensors_state));
  p_app_snapshot->temperature_aver = app.temperature_aver;
  p_app_snapshot->onew_devices_num = app.onew_devices_num;
  p_app_snapshot->current_sensor = app.current_sensor;

  _rtc_get_time(&p_app_snapshot->rtc_time);
  p_app_snapshot->ts.SECONDS = p_app_snapshot->rtc_time;
  p_app_snapshot->ts.MILLISECONDS = 0;
  _time_to_date(&p_app_snapshot->ts,&p_app_snapshot->tm);

  p_app_snapshot->card_free_space = g_fs_free_space;

}


/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
void MZTS_task(void)
{
  int32_t         res;
  uint32_t        rtc_time;
  TIME_STRUCT     ts;
  DATE_STRUCT     tm;
  MQX_FILE_PTR    f;

  if (Create_sync_objects() != MQX_OK)
  {
    // Не удалось создать объект синхронизации. Прекращаем раьоту задачи
    LOG("Task error.", __FUNCTION__, __LINE__, SEVERITY_DEFAULT);
    return;
  }

  SPI0_Init_Bus();
  GUI_task_create();

  for (uint32_t i=0; i < TEMP_ZONES_NUM; i++)
  {
    app.temperatures[i]  = 0.0f;
    app.sensors_state[i] = 0;
  }

  Init_sensors_config();
  OneWire_task_create();


  // Открываем файл лога
  _rtc_get_time(&rtc_time);
  ts.SECONDS = rtc_time;
  ts.MILLISECONDS = 0;
  _time_to_date(&ts,&tm);
  sprintf(fname,"a:%04d_%02d_%02d__%02d_%02d_%02d.csv", tm.YEAR, tm.MONTH, tm.DAY, tm.HOUR, tm.MINUTE, tm.SECOND);


  if (wvar.max_sens_log_file_size > 0)
  {
    do
    {
      if (Wait_for_app_event(10, EVENT_SENSOR_RESULTS_READY) != 0)
      {
        f = _io_fopen(fname, "a+");
        if (f == NULL) break;

        if (f->SIZE > wvar.max_sens_log_file_size)
        {
          _io_fclose(f);
          // Открываем новый файл лога
          _rtc_get_time(&rtc_time);
          ts.SECONDS = rtc_time;
          ts.MILLISECONDS = 0;
          _time_to_date(&ts,&tm);
          sprintf(fname,"a:%04d_%02d_%02d__%02d_%02d_%02d.csv", tm.YEAR, tm.MONTH, tm.DAY, tm.HOUR, tm.MINUTE, tm.SECOND);
          f = _io_fopen(fname, "a+");
          if (f == NULL) break;
        }

        _rtc_get_time(&rtc_time);
        ts.SECONDS = rtc_time;
        ts.MILLISECONDS = 0;
        _time_to_date(&ts,&tm);
        res = sprintf(results_string, "%04d.%02d.%02d %02d:%02d:%02d, ", tm.YEAR, tm.MONTH, tm.DAY, tm.HOUR, tm.MINUTE, tm.SECOND);
        _io_write(f, results_string, res);
        res = sprintf(results_string, "%0.1f, %0.1f, %0.1f, %0.1f, %0.1f, %0.1f, %0.1f, %0.1f\r\n",
             app.temperatures[0],
             app.temperatures[1],
             app.temperatures[2],
             app.temperatures[3],
             app.temperatures[4],
             app.temperatures[5],
             app.temperatures[6],
             app.temperatures[7]
             );
        _io_write(f, results_string, res);
        res = _io_fclose(f);
        if (res != MQX_OK) break;
      }
    } while (1);
  }

}


