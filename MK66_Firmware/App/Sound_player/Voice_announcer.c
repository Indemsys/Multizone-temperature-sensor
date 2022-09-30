// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2020.07.13
// 16:29:15
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

static uint32_t en_arrival_on_floor_announcer_mode;
static uint32_t en_voice_announcer;
static uint32_t sound_volume;
static uint32_t voice_language;
static uint32_t sound_freq  = 4000;

uint32_t Get_en_arrivall_on_floors_announcer_mode(void) { return  en_arrival_on_floor_announcer_mode;}
uint32_t Get_en_voice_announcer(void) { return  en_voice_announcer;}
uint32_t Get_sound_volume(void) { return  sound_volume;}
uint32_t Get_voice_language(void) { return  voice_language;}
uint32_t Get_sound_freq(void) { return  sound_freq;}

void     Set_en_voice_announcer(uint8_t val) {  en_voice_announcer = val; }


const T_vannouncer_item vannouncer_map[] =
{
  { VOAN_MSG_ERROR_,                Play_error_msg                , "ERROR MESSAGE       " },  // 0
  { VOAN_MSG_EMERGENCY_STOP,        Play_msg_emergency_stop       , "EMERGENCY STOP MESS." },  // 1
  { VOAN_MSG_SENSOR_NUMBER,         Play_msg_sensor_number        , "SENSOR NUMBER       " },  // 2
  { VOAN_MSG_OVERLOAD,              Play_msg_overload             , "OVERLOAD MESSAGE    " },  // 3
  { VOAN_MSG_OPENED_LOCK,           Play_msg_opened_lock          , "OPENED LOCK MESSAGE " },  // 4
  { VOAN_MSG_SERVICE_MODE,          Play_msg_service_mode         , "SERVICE MODE MESSAGE" },  // 5
  { VOAN_MSG_WORK_MODE,             Play_msg_work_mode            , "WORK MODE MESSAGE   " },  // 6
  { VOAN_MSG_LAND_SET_MADE,         Play_msg_land_set_made        , "LAND. SET. MADE MSG." },  // 7
  { VOAN_MSG_FIRE_ALARM,            Play_msg_fire_alarm           , "FIRE ALARM MESSAGE  " },  // 8
  { VOAN_ARRIVAL_SND,               Play_arrival_snd              , "ARRIVAL SOUND       " },  // 9
  { VOAN_MSG_ADOOR_CALIBR0ANGLE   , Play_msg_adoor_calibr0angle   , "ADOOR CALIBR0ANGLE  " },  // 10
  { VOAN_MSG_ADOOR_CALIBR90ANGLE  , Play_msg_adoor_calibr90angle  , "ADOOR CALIBR90ANGLE " },  // 11
  { VOAN_MSG_ADOOR_CALIBROPENPOS  , Play_msg_adoor_calibropenpos  , "ADOOR CALIBROPENPOS " },  // 12
  { VOAN_MSG_ADOOR_CALIBRCLOSEPOS , Play_msg_adoor_calibrclosepos , "ADOOR CALIBRCLOSEPOS" },  // 13
  { VOAN_MSG_ADOOR_CALIBRDONE     , Play_msg_adoor_calibrdone     , "ADOOR CALIBRDONE    " },  // 14
  { VOAN_MSG_ADOOR_CALIBRCANCEL   , Play_msg_adoor_calibrcancel   , "ADOOR CALIBRCANCEL  " },  // 15
  { VOAN_MSG_ALARM                , Play_msg_alarm                , "ALARM MESSAGE       " },  // 16
};

#define  SIZEOF_SOUNDS_MAP   (sizeof(vannouncer_map)/ sizeof(vannouncer_map[0]) )

/*------------------------------------------------------------------------------
  Обработка команд приходящих по CAN
 ------------------------------------------------------------------------------*/
void VAnnouncer_messages_processing(const uint8_t *buf)
{
  uint32_t res;

  if (buf[0] == VOAN_CMD_SETT)
  {
    switch (buf[1])
    {
    case VOAN_ARRIVAL_ON_FLOORS_ANNOUNCER_MODE :
      memcpy(&en_arrival_on_floor_announcer_mode,&buf[2], 4);
      LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Arrival on floor announcer mode = %d", en_arrival_on_floor_announcer_mode);
      break;
    case VOAN_EN_VOICE_ANNOUNCER :
      memcpy(&en_voice_announcer,&buf[2], 4);
      LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Voice announcer mode = %d", en_voice_announcer);
      break;
    case VOAN_SOUND_VOLUME  :
      memcpy(&sound_volume,&buf[2], 4);
      res = Audio_set_attenuation(sound_volume);
      LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Sound attenuation = %d (result = %d)", sound_volume, res );
      break;
    case VOAN_VOICE_LANGUAGE:
      memcpy(&voice_language,&buf[2], 4);
      LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Voice laguage = %d", voice_language);
      break;

    }
  }
  else if (buf[0] == VOAN_CMD_PLAY)
  {
    uint32_t msg_num;
    uint32_t msg_arg;

    msg_num = buf[1];
    memcpy(&msg_arg,&buf[2], 4);
    Play_msg_by_num(msg_num, msg_arg);
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Play command = %d, arg = %d", msg_num, msg_arg);
  }
  else if (buf[0] == VOAN_CMD_SOUND_FREQ)
  {
    memcpy(&sound_freq,&buf[1], 4);
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Sound frequency = %d", sound_freq);
  }
  else if (buf[0] == VOAN_CMD_SOUND_GEN)
  {
    Play_sound(sound_freq);
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Play sound %d", sound_freq);
  }
  else if (buf[0] == VOAN_CMD_STOP)
  {
    Stop_play();
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Stop play");
  }
}


/*------------------------------------------------------------------------------
   Получить префикс для пути к директории с аудиофайлами
 ------------------------------------------------------------------------------*/
const char* VAnnouncer_get_files_dir(void)
{
  switch (voice_language)
  {
  case 0:
    return DIR_ENG;
  case 1:
    return DIR_RUS;
  case 2:
    return DIR_NOR;
  case 3:
    return DIR_DER;
  case 4:
    return DIR_SWE;
  default:
    return DIR_ENG;
  }
}


/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
void Play_msg_by_num(unsigned int  msg_num, int msg_arg)
{
  unsigned int i;
  unsigned int n = sizeof(vannouncer_map) / sizeof(vannouncer_map[0]);

  for (i = 0; i < n; i++)
  {
    if (vannouncer_map[i].msg_id == msg_num)
    {
      vannouncer_map[i].play_func(msg_arg);
      return;
    }
  }
}


/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_msg_arrival_on_floor(int num)
{
  Enqueue_file(SND_GONG);

  switch (num)
  {
  case 0:
    Enqueue_file(FLOOR0_PROMPT);
    break;
  case 1:
    Enqueue_file(FLOOR1_PROMPT);
    break;
  case 2:
    Enqueue_file(FLOOR2_PROMPT);
    break;
  case 3:
    Enqueue_file(FLOOR3_PROMPT);
    break;
  case 4:
    Enqueue_file(FLOOR4_PROMPT);
    break;
  case 5:
    Enqueue_file(FLOOR5_PROMPT);
    break;
  case 6:
    Enqueue_file(FLOOR5_PROMPT);
    break;
  }
  Enqueue_file(SND_SILENCE);
}

/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_error_msg(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_GONG);
    Enqueue_file(SND_OBSTACLE_TO_MOVEMENT);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(SND_SILENCE);
    Enqueue_file((num / 100)% 10);
    Enqueue_file(SND_SILENCE);
    Enqueue_file((num / 10)% 10);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(num % 10);
    Enqueue_file(SND_SILENCE);
  }
}

/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_msg_opened_lock(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_GONG);
    Enqueue_file(SND_OBSTACLE_TO_MOVEMENT);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(SND_LOCK_IS_NOT_CLOSET);
  }
}

/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/

void Play_msg_service_mode(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_GONG);
    //    Enqueue_file_num(VKLUCEN_SERVISNYJ_REZIM_RU);
    Enqueue_file(SND_SILENCE);
  }
}

/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/

void Play_msg_work_mode(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_GONG);
    //    Enqueue_file_num(VKLUCEN_RABOCIJ_REZIM_RU);
    Enqueue_file(SND_SILENCE);
  }
}

/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_msg_land_set_made(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_GONG);
    Enqueue_file(SND_LANDING_SETTING_IS_MADE);
    Enqueue_file(SND_SILENCE);
  }
}
/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_msg_fire_alarm(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_OVERLOADSOUND);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(SND_FIRE_ALARM);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(SND_OVERLOADSOUND);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(SND_FIRE_ALARM);
  }
}

/*-----------------------------------------------------------------------------------------------------
  
  
  \param void  
-----------------------------------------------------------------------------------------------------*/
void Play_msg_alarm(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_ALARM);
  }
  else
  {
    Play_sound(440);
  }
  Delayed_Stop_play();
}

/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_msg_emergency_stop(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_GONG);
    Enqueue_file(SND_EMERGENCY_STOP);
    Enqueue_file(SND_SILENCE);
  }

}

/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_msg_sensor_number(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_SILENCE);
    Enqueue_file(SND_SENSOR);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(SND_SILENCE);
    Enqueue_file((num / 10)% 10);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(num % 10);
    Enqueue_file(SND_SILENCE);
  }
}

/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_msg_overload(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_GONG);
    Enqueue_file(SND_OVERLOAD);
  }
  else
  {
    Enqueue_file(SND_OVERLOADSOUND);
  }
}



/*------------------------------------------------------------------------------
    Английская версия
 ------------------------------------------------------------------------------*/
void Play_arrival_snd(int num)
{
  switch (en_arrival_on_floor_announcer_mode)
  {
  case ARRIVAL_ON_FLOOR_SILENCE:
    break;
  case ARRIVAL_ON_FLOOR_GONG:
    Enqueue_file(SND_GONG);
    Enqueue_file(SND_SILENCE);
    Enqueue_file(SND_GONG);
    break;
  case ARRIVAL_ON_FLOOR_VOICE_MSG:
    Play_msg_arrival_on_floor(num);
    break;
  }

}


/*-----------------------------------------------------------------------------------------------------
  
  
  \param num  
-----------------------------------------------------------------------------------------------------*/
void Play_msg_adoor_calibr0angle(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_ADOOR_CALIBR0ANGLE);
  }
}
/*-----------------------------------------------------------------------------------------------------
  
  
  \param num  
-----------------------------------------------------------------------------------------------------*/
void Play_msg_adoor_calibr90angle(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_ADOOR_CALIBR90ANGLE);
  }
}
/*-----------------------------------------------------------------------------------------------------
  
  
  \param num  
-----------------------------------------------------------------------------------------------------*/
void Play_msg_adoor_calibropenpos(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_ADOOR_CALIBROPENPOS);
  }
}
/*-----------------------------------------------------------------------------------------------------
  
  
  \param num  
-----------------------------------------------------------------------------------------------------*/
void Play_msg_adoor_calibrclosepos(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_ADOOR_CALIBRCLOSEPOS);
  }
}
/*-----------------------------------------------------------------------------------------------------
  
  
  \param num  
-----------------------------------------------------------------------------------------------------*/
void Play_msg_adoor_calibrdone(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_ADOOR_CALIBRDONE);
  }
}
/*-----------------------------------------------------------------------------------------------------
  
  
  \param num  
-----------------------------------------------------------------------------------------------------*/
void Play_msg_adoor_calibrcancel(int num)
{
  if (en_voice_announcer)
  {
    Enqueue_file(SND_ADOOR_CALIBRCANCEL);
  }
}

/*-----------------------------------------------------------------------------------------------------
  
  
  \param sid  
  
  \return const char* 
-----------------------------------------------------------------------------------------------------*/
const char *Get_sound_description(uint32_t sid)
{
  if (sid>=SIZEOF_SOUNDS_MAP )
  {
    return "Undefined";
  }

  return vannouncer_map[sid].description;

}




