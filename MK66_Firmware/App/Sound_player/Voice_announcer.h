#ifndef VOICE_ANNOUNCER_H
  #define VOICE_ANNOUNCER_H

#define ARRIVAL_ON_FLOOR_SILENCE       0
#define ARRIVAL_ON_FLOOR_GONG          1
#define ARRIVAL_ON_FLOOR_VOICE_MSG     2


// data[0] - Тип команды
  #define VOAN_CMD_SETT                    0 // Настройка параметров проигрывателя
// data[1] - Номер параметра. data[2..5] - значение параметра 4 байта
  #define VOAN_ARRIVAL_ON_FLOORS_ANNOUNCER_MODE            1 // Установка значения переменной en_arrival_on_floor_announcer_mode
  #define VOAN_EN_VOICE_ANNOUNCER          2 // Установка значения переменной en_voice_announcer
  #define VOAN_SOUND_VOLUME                3 // Установка значения переменной sound_volume
  #define VOAN_VOICE_LANGUAGE              4 // Установка значения переменной voice_language




// data[0] - Тип команды
  #define VOAN_CMD_PLAY      1 // Команда на проигрывание определенного сообщения
// data[1] - номер сообщения, data[2..5] - аргумент сообщения 4 байта
  #define VOAN_MSG_ERROR_                  0 //
  #define VOAN_MSG_EMERGENCY_STOP          1 //
  #define VOAN_MSG_SENSOR_NUMBER           2 //
  #define VOAN_MSG_OVERLOAD                3 //
  #define VOAN_MSG_OPENED_LOCK             4 //
  #define VOAN_MSG_SERVICE_MODE            5 //
  #define VOAN_MSG_WORK_MODE               6 //
  #define VOAN_MSG_LAND_SET_MADE           7 //
  #define VOAN_MSG_FIRE_ALARM              8 //
  #define VOAN_ARRIVAL_SND                 9 //
  #define VOAN_MSG_ADOOR_CALIBR0ANGLE      10 //
  #define VOAN_MSG_ADOOR_CALIBR90ANGLE     11 //
  #define VOAN_MSG_ADOOR_CALIBROPENPOS     12 //
  #define VOAN_MSG_ADOOR_CALIBRCLOSEPOS    13 //
  #define VOAN_MSG_ADOOR_CALIBRDONE        14 //
  #define VOAN_MSG_ADOOR_CALIBRCANCEL      15 //
  #define VOAN_MSG_ALARM                   16

  #define VOAN_LAST_MSG_ID                 17 //


// data[0] - Тип команды
#define VOAN_CMD_SOUND_FREQ     2 // Установка частоты тона
// data[1...4] - частота тона

// data[0] - Тип команды
#define VOAN_CMD_SOUND_GEN      3 // Команда на генерацию тона

// data[0] - Тип команды
#define VOAN_CMD_STOP           4 // Команда на отсновку генерации звука 


typedef struct
{
    uint32_t msg_id;
    T_play_func play_func;
    const char *description;    

} T_vannouncer_item;


void        Play_msg_by_num(unsigned int  msg_num, int msg_arg);
void        Play_error_msg(int num);
void        Play_msg_emergency_stop(int num);
void        Play_msg_sensor_number(int num);
void        Play_msg_overload(int num);
void        Play_msg_opened_lock(int num);
void        Play_msg_service_mode(int num);
void        Play_msg_work_mode(int num);
void        Play_msg_land_set_made(int num);
void        Play_msg_fire_alarm(int num);
void        Play_msg_alarm(int num);
void        Play_arrival_snd(int num);
void        Play_msg_adoor_calibr0angle(int num);
void        Play_msg_adoor_calibr90angle(int num);
void        Play_msg_adoor_calibropenpos(int num);
void        Play_msg_adoor_calibrclosepos(int num);
void        Play_msg_adoor_calibrdone(int num);
void        Play_msg_adoor_calibrcancel(int num);

void        VAnnouncer_messages_processing(const uint8_t *buf);
const char* VAnnouncer_get_files_dir(void);

uint32_t Get_en_arrivall_on_floors_announcer_mode  (void);
uint32_t Get_en_voice_announcer   (void);
uint32_t Get_sound_volume         (void);
uint32_t Get_voice_language       (void);
uint32_t Get_sound_freq           (void);
void     Set_en_voice_announcer   (uint8_t val);

const char *Get_sound_description(uint32_t sid);


#endif // VOICE_ANNOUNCER_H



