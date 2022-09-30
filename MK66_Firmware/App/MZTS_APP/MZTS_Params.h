#ifndef __PARAMS_H
  #define __PARAMS_H

#define  APP_PROFILE        MZTS
#define  MAIN_PARAMS_ROOT   MZTS_main
#define  PARAMS_ROOT        MZTS_0

#define  DWVAR_SIZE        14
#define  PARMNU_ITEM_NUM   4



  #define VAL_LOCAL_EDITED 0x01  //
  #define VAL_READONLY     0x02  // Можно только читать
  #define VAL_PROTECT      0x04  // Защишено паролем
  #define VAL_UNVISIBLE    0x08  // Не выводится на дисплей
  #define VAL_NOINIT       0x10  // Не инициализируется


enum vartypes
{
    tint8u  = 1,
    tint16u  = 2,
    tint32u  = 3,
    tfloat  = 4,
    tarrofdouble  = 5,
    tstring  = 6,
    tarrofbyte  = 7,
    tint32s  = 8,
};


enum enm_parmnlev
{
    MZTS_Sensors,
    MZTS_BLE,
    MZTS_0,
    MZTS_main,
    MZTS_General,
};


typedef struct 
{
  enum enm_parmnlev prevlev;
  enum enm_parmnlev currlev;
  const char* name;
  const char* shrtname;
  const char  visible;
}
T_parmenu;


typedef struct
{
  const uint8_t*     name;         // Строковое описание
  const uint8_t*     abbreviation; // Короткая аббревиатура
  void*              val;          // Указатель на значение переменной в RAM
  enum  vartypes     vartype;      // Идентификатор типа переменной
  float              defval;       // Значение по умолчанию
  float              minval;       // Минимальное возможное значение
  float              maxval;       // Максимальное возможное значение  
  uint8_t            attr;         // Аттрибуты переменной
  unsigned int       parmnlev;     // Подгруппа к которой принадлежит параметр
  const  void*       pdefval;      // Указатель на данные для инициализации
  const  char*       format;       // Строка форматирования при выводе на дисплей
  void               (*func)(void);// Указатель на функцию выполняемую после редактирования
  uint16_t           varlen;       // Длинна переменной
} T_work_params;


typedef struct
{
  uint8_t        adv_dev_name[20];              // Advertising device name | def.val.= MZTS01
  uint8_t        ble_ver[16];                   // Version | def.val.= MZTS1
  uint32_t       max_sens_log_file_size;        // Maximal log file size (byte) | def.val.= 1000000
  uint8_t        name[64];                      // Product  name | def.val.= MZTS
  uint32_t       pin_code;                      // Pin code | def.val.= 123456
  uint8_t        screen_rot;                    // Screen rotation (0-0, 1- 90, 2-180, 3-270) | def.val.= 0
  uint8_t        zone1_sensor_id[17];           // Zone 1 sensor ID | def.val.= 
  uint8_t        zone2_sensor_id[17];           // Zone 2 sensor ID | def.val.= 
  uint8_t        zone3_sensor_id[17];           // Zone 3 sensor ID | def.val.= 
  uint8_t        zone4_sensor_id[17];           // Zone 4 sensor ID | def.val.= 
  uint8_t        zone5_sensor_id[17];           // Zone 5 sensor ID | def.val.= 
  uint8_t        zone6_sensor_id[17];           // Zone 6 sensor ID | def.val.= 
  uint8_t        zone7_sensor_id[17];           // Zone 7 sensor ID | def.val.= 
  uint8_t        zone8_sensor_id[17];           // Zone 8 sensor ID | def.val.= 
} WVAR_TYPE;


#endif
