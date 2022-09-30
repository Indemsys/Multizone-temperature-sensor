#ifndef __SOUND_LIST
  #define __SOUND_LIST


  #define DIR_ENG  "a:\\ENG\\"
  #define DIR_RUS  "a:\\RUS\\"
  #define DIR_NOR  "a:\\NOR\\"
  #define DIR_DER  "a:\\DER\\"
  #define DIR_SWE  "a:\\SWE\\"



enum T_sounds_id
{
  SND_0                              ,
  SND_1                              ,
  SND_2                              ,
  SND_3                              ,
  SND_4                              ,
  SND_5                              ,
  SND_6                              ,
  SND_7                              ,
  SND_8                              ,
  SND_9                              ,
  SND_ALARM                          ,
  SND_DOOR_IS_NOT_CLOSED             ,
  SND_EMERGENCY_STOP                 ,
  SND_FIRE_ALARM                     ,
  SND_GONG                           ,
  SND_LANDING_SETTING_IS_MADE        ,
  SND_LOCK_IS_NOT_CLOSET             ,
  SND_OBSTACLE_TO_MOVEMENT           ,
  SND_OVERLOAD                       ,
  SND_OVERLOADSOUND                  ,
  SND_SENSOR                         ,
  SND_SILENCE                        ,
  FLOOR0_PROMPT                      ,
  FLOOR1_PROMPT                      ,
  FLOOR2_PROMPT                      ,
  FLOOR3_PROMPT                      ,
  FLOOR4_PROMPT                      ,
  FLOOR5_PROMPT                      ,
  FLOOR6_PROMPT                      ,
  SND_ADOOR_CALIBR0ANGLE             ,
  SND_ADOOR_CALIBR90ANGLE            ,
  SND_ADOOR_CALIBROPENPOS            ,
  SND_ADOOR_CALIBRCLOSEPOS           ,
  SND_ADOOR_CALIBRDONE               ,
  SND_ADOOR_CALIBRCANCEL             ,
};

typedef struct
{
    enum  T_sounds_id sid;
    const char        *file_name;

} T_sound_file;


typedef void (*T_play_func)(int num);


  #ifndef SOUNDS_LIST

extern const T_sound_file    sound_files_map[];

  #endif


#endif
