#ifndef __SOUND_PLAYER
  #define __SOUND_PLAYER

  #include "Sound_list.h"
  #include "Sound_gen.h"
  #include "WAV_reader.h"
  #include "Voice_announcer.h"


  #define AUDIO_DEFAULT_SR   8000ul
  #define AUDIO_DEFAULT_MOD  (BSP_SYSTEM_CLOCK/AUDIO_DEFAULT_SR)
  #define AUDIO_MIN_MOD      (BSP_SYSTEM_CLOCK/48000ul)

  #define MAX_AUDIO_IC_VOLUME         63

  #define AUDIO_BUF_SAMPLES_NUM       512

  #define DEALAYED_STOP_TIMEOUT_MS    1000


  #define EVT_PLAY_SOUND     BIT( 1 )
  #define EVT_READ_NEXT      BIT( 2 )
  #define EVT_STOP_PLAY      BIT( 3 )
  #define EVT_PLAY_FILE      BIT( 4 )
  #define EVT_DELAYED_STOP   BIT( 5 )


  #define PLAYER_IDLE        0
  #define PLAYING_SOUND      1
  #define PLAYING_FILE       2

  #define PLAY_QUEUE_SZ      32

  #define MAX_FILE_PATH_LEN  128

  #define MUTE_ON            Set_MUTE(1)
  #define MUTE_OFF           Set_MUTE(0)


typedef struct
{
    uint8_t       mode;       // Режим работы плеера. 0- неактивен, 1 - воспроизведение тона, 2- воспроизведение файла
    uint8_t       bank;       // Индекс текущего сэмпла
    uint32_t      tone_freq;  // Частота тона в Гц
    int16_t      *file_ptr;   // Указатель на данные файла
    char          file_name[MAX_FILE_PATH_LEN+1];  // Имя файла
    MQX_FILE_PTR  fp;
    int16_t       tmp_audio_buf[AUDIO_BUF_SAMPLES_NUM];
    int           block_sz;   // Размер прочитанного их файл блока данных
    int           file_sz;    // Размер текущего воспроизводимого файла. В случае воспроизведения файлов их Flash памяти
    int           file_end;   // Флаг завершения воспроизведения файла

    int           play_queue[PLAY_QUEUE_SZ]; //  Очередь на воспроизведение файлов
    int           queue_head;
    int           queue_tail;
    int           queue_sz;
    uint32_t      audio_ic_errors;

} T_sound_player;


void      Player_task_create(void);

void      Set_MUTE(uint8_t state);
uint32_t  Audio_set_attenuation(uint32_t v);

void      Play_sound(uint32_t tone_freq);
void      Stop_play(void);
void      Play_file(int file);
void      Delayed_Stop_play(void);

void      Enqueue_file(int file);

unsigned int Get_wave_file_sample_rate(void);

#endif
