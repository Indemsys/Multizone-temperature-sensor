#include "App.h"
#include   "freemaster_tsa.h"

const T_parmenu parmenu[4]=
{
  {MZTS_0,MZTS_main,"Parameters and settings","PARAMETERS", -1},//  ???????? ?????????
  {MZTS_main,MZTS_General,"General settings","GENERAL_SETTINGS", -1},//  
  {MZTS_main,MZTS_Sensors,"Sensors settings","SENSORS", -1},//  
  {MZTS_main,MZTS_BLE,"BLE parameters","BLE", -1},//  
};


const T_work_params dwvar[14]=
{
  {
    " Version ",
    "BLEVERS",
    (void*)&wvar.ble_ver,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_BLE,
    "MZTS1",
    "%s",
    0,
    sizeof(wvar.ble_ver),
  },
  {
    " Pin code ",
    "BLEPINC",
    (void*)&wvar.pin_code,
    tint32u,
    123456,
    0,
    999999,
    0,
    MZTS_BLE,
    "",
    "%d",
    0,
    sizeof(wvar.pin_code),
  },
  {
    " Advertising device name ",
    "BLEADVN",
    (void*)&wvar.adv_dev_name,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_BLE,
    "MZTS01",
    "%s",
    0,
    sizeof(wvar.adv_dev_name),
  },
  {
    " Product  name ",
    "SYSNAM",
    (void*)&wvar.name,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_General,
    "MZTS",
    "%s",
    0,
    sizeof(wvar.name),
  },
  {
    " Screen rotation (0-0, 1- 90, 2-180, 3-270) ",
    "SCRNROT",
    (void*)&wvar.screen_rot,
    tint8u,
    0,
    0,
    3,
    0,
    MZTS_General,
    "",
    "%d",
    0,
    sizeof(wvar.screen_rot),
  },
  {
    " Maximal log file size (byte) ",
    "MAXLGFS",
    (void*)&wvar.max_sens_log_file_size,
    tint32u,
    1000000,
    0,
    2000000000,
    0,
    MZTS_General,
    "",
    "%d",
    0,
    sizeof(wvar.max_sens_log_file_size),
  },
  {
    " Zone 1 sensor ID ",
    "Z1SNSID",
    (void*)&wvar.zone1_sensor_id,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_Sensors,
    "",
    "%s",
    0,
    sizeof(wvar.zone1_sensor_id),
  },
  {
    " Zone 2 sensor ID ",
    "Z2SNSID",
    (void*)&wvar.zone2_sensor_id,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_Sensors,
    "",
    "%s",
    0,
    sizeof(wvar.zone2_sensor_id),
  },
  {
    " Zone 3 sensor ID ",
    "Z3SNSID",
    (void*)&wvar.zone3_sensor_id,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_Sensors,
    "",
    "%s",
    0,
    sizeof(wvar.zone3_sensor_id),
  },
  {
    " Zone 4 sensor ID ",
    "Z4SNSID",
    (void*)&wvar.zone4_sensor_id,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_Sensors,
    "",
    "%s",
    0,
    sizeof(wvar.zone4_sensor_id),
  },
  {
    " Zone 5 sensor ID ",
    "Z5SNSID",
    (void*)&wvar.zone5_sensor_id,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_Sensors,
    "",
    "%s",
    0,
    sizeof(wvar.zone5_sensor_id),
  },
  {
    " Zone 6 sensor ID ",
    "Z6SNSID",
    (void*)&wvar.zone6_sensor_id,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_Sensors,
    "",
    "%s",
    0,
    sizeof(wvar.zone6_sensor_id),
  },
  {
    " Zone 7 sensor ID ",
    "Z7SNSID",
    (void*)&wvar.zone7_sensor_id,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_Sensors,
    "",
    "%s",
    0,
    sizeof(wvar.zone7_sensor_id),
  },
  {
    " Zone 8 sensor ID ",
    "Z8SNSID",
    (void*)&wvar.zone8_sensor_id,
    tstring,
    0,
    0,
    0,
    0,
    MZTS_Sensors,
    "",
    "%s",
    0,
    sizeof(wvar.zone8_sensor_id),
  },
};
