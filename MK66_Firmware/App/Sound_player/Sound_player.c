#include "App.h"


extern int            Read_wav_header_from_file(FILE *fp);
extern T_WAV_pars_err Wavefile_header_parsing(void);


static _task_id       player_task_id;

static  int16_t       audio_buf[2][AUDIO_BUF_SAMPLES_NUM];

#define   DMA_ITER_COUNT   (sizeof(audio_buf) / 2)
#define   SAMPLE_SIZE  (sizeof(audio_buf[0][0]))

static T_sound_player ply;


static LWEVENT_STRUCT play_lwev;

static T_DMA_cbl      audio_cbl;
static uint8_t        f_delayed_stop = 0;
static TIME_STRUCT    delayed_stop_start;

/*-----------------------------------------------------------------------------------------------------
  
  
  \param val  
-----------------------------------------------------------------------------------------------------*/
void Set_MUTE(uint8_t state)
{
  if (state == 0) PTE_BASE_PTR->PCOR = LSHIFT(1, 27); // Устанавливаем бит в Port Clear Output Register
  else PTE_BASE_PTR->PSOR = LSHIFT(1, 27);              // Устанавливаем бит в Port Set Output Register
}


/*-----------------------------------------------------------------------------------------------------
  
  
  \param void  
-----------------------------------------------------------------------------------------------------*/
static void _Audio_set_evt_to_read_next_block(void)
{
  _lwevent_set(&play_lwev, EVT_READ_NEXT);
}

/*-----------------------------------------------------------------------------------------------------
  
  
  \param void  
-----------------------------------------------------------------------------------------------------*/
static void _Audio_set_evt_to_play_file(void)
{
  _lwevent_set(&play_lwev, EVT_PLAY_FILE);
}


/*-----------------------------------------------------------------------------------------------------
  
  
  \param user_isr_ptr  
-----------------------------------------------------------------------------------------------------*/
static void _Audio_interrupt_handler(void *user_isr_ptr)
{
  //ITM_EVENT32(1,0);
  // БЛок ниже выполянется за 2.76 мкс
  DMA_CINT = DMA_AUDIO_MF_CH; // Сбрасываем флаг прерываний  канала
  _Audio_set_evt_to_read_next_block();
}

/*-----------------------------------------------------------------------------------------------------
  Обновить частоту сэмплирования проигрывателя 
  
  \param sr  
-----------------------------------------------------------------------------------------------------*/
static void _Audio_update_sample_rate(uint32_t sr)
{
  uint16_t mod = (uint16_t)(BSP_SYSTEM_CLOCK / sr);
  if (mod < AUDIO_MIN_MOD) mod = AUDIO_MIN_MOD;

  TPM_Init_for_audio(AUDIO_PLAYER_TIMER, mod);
}


/*-----------------------------------------------------------------------------------------------------
  Узнаем c какой половиной аудиобуфера в данный момент рабаотает DMA   
  
  \param bnk  
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Audio_get_current_DMA_buf(void)
{
  DMA_MemMapPtr DMA  = DMA_BASE_PTR;
  uint32_t      cnt;
  uint8_t       n;

  // Определим позицию и индекс буфера
  cnt = DMA->TCD[DMA_AUDIO_MF_CH].CITER_ELINKNO; // Учесть что читаемый счетчик CITER не всегда успевает обновиться до актуальной величины после возникновения прерывания и может отставать на 8 более
  //ITM_EVENT32(4,cnt);
  if (cnt <= (DMA_ITER_COUNT / 2))
  {
    n = 1;
  }
  else
  {
    n = 0;
  }

  return  n;
}


/*------------------------------------------------------------------------------
  Конфигурирование DMA для передачи данных в DAC
  Запрос DMA вызывается таймером TPM по переполнению таймера
  Пересылается два байта отсчета после каждого переполнения таймера
 
  Прерывания от DMA возникают когда пройден полный цикл пересылки и половина цикла пересылки
  За один цикл пересылки отправляется содержимое всего двумерного массива audio_buf 
 
  По каждому прерыванию отправляется сигнал на заполнение следующими данными очередной половины массива
 
  Пока DMA явно не запрещен циклическая пересылка продолжается непрерывно
 ------------------------------------------------------------------------------*/
static void _Audio_config_DMA(void)
{
  DAC_MemMapPtr DAC0 = DAC0_BASE_PTR;
  DMA_MemMapPtr DMA  = DMA_BASE_PTR;


  DMA->CERQ = DMA_AUDIO_MF_CH;                    // Запрещаем работу канала
  DMA->TCD[DMA_AUDIO_MF_CH].CSR = 0;              // Выключаем канал DMA
  while (DMA->TCD[DMA_AUDIO_MF_CH].CSR & BIT(6)); // Ждем прекращения активности канала
  DMA->CDNE = DMA_AUDIO_MF_CH;                    // Сбрасываем бит DONE

  audio_cbl.minor_tranf_sz = DMA_2BYTE_MINOR_TRANSFER;


  DMA->TCD[DMA_AUDIO_MF_CH].SADDR       = (uint32_t)&audio_buf[0][0];     // Источник - буфер с данными
  DMA->TCD[DMA_AUDIO_MF_CH].SOFF        = DMA_2BYTE_MINOR_TRANSFER;       // Адрес источника смещаем  после каждой передачи
  DMA->TCD[DMA_AUDIO_MF_CH].SLAST       = (uint32_t)(-DMA_ITER_COUNT * 2);        // Корректируем адрес источника после завершения всего цикла DMA (окончания мажорного цикла)
  DMA->TCD[DMA_AUDIO_MF_CH].DADDR       = (uint32_t)&(DAC0->DAT[0].DATL); // Адрес приемника - регистр PUSHR SPI
  DMA->TCD[DMA_AUDIO_MF_CH].DOFF        = 0;                              // После  записи указатель приемника не смещаем
  DMA->TCD[DMA_AUDIO_MF_CH].DLAST_SGA   = 0;                              // Цепочки дескрипторов не применяем
  DMA->TCD[DMA_AUDIO_MF_CH].NBYTES_MLNO = DMA_2BYTE_MINOR_TRANSFER;       // Количество байт пересылаемых за один запрос DMA (в минорном цикле)

  DMA->TCD[DMA_AUDIO_MF_CH].BITER_ELINKNO = 0       // TCD Beginning Minor Loop Link, Major Loop Count
                                           + LSHIFT(0, 15)                // ELINK  | 0 - The channel-to-channel linking is disabled
                                           + LSHIFT(0, 9)                 // LINKCH |
                                           + LSHIFT(DMA_ITER_COUNT, 0)        // BITER  | Устанавливаем количество мажорных пересылок DMA. Здесь количестов 2-х байтных слов в обоиз буферах вместе взятых.
  ;

  DMA->TCD[DMA_AUDIO_MF_CH].CITER_ELINKNO = 0       // TCD Current Minor Loop Link, Major Loop Count
                                           + LSHIFT(0, 15)                 // ELINK  | 0 - The channel-to-channel linking is disabled
                                           + LSHIFT(0, 9)                  // LINKCH |
                                           + LSHIFT(DMA_ITER_COUNT, 0)         // BITER  | Устанавливаем количество мажорных пересылок DMA. Здесь количестов 2-х байтных слов в обоиз буферах вместе взятых.
  ;

  DMA->TCD[DMA_AUDIO_MF_CH].ATTR = 0
                                  + LSHIFT(0, 11)                                           // SMOD  | Модуль адреса источника не используем
                                  + LSHIFT(DMA_Get_attr_size(DMA_2BYTE_MINOR_TRANSFER), 8)  // SSIZE | Задаем количество бит передаваемых за одну операцию чтения источника
                                  + LSHIFT(0, 3)                                            // DMOD  | Модуль адреса приемника
                                  + LSHIFT(DMA_Get_attr_size(DMA_2BYTE_MINOR_TRANSFER), 0)  // DSIZE | Задаем количество бит передаваемых за одну операцию записи в приемник
  ;

  DMA->TCD[DMA_AUDIO_MF_CH].CSR = 0
                                 + LSHIFT(0, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
                                 + LSHIFT(0, 8)  // MAJORLINKCH | Линковку не применяем
                                 + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
                                 + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
                                 + LSHIFT(0, 5)  // MAJORELINK  | Линковку не применяем
                                 + LSHIFT(0, 4)  // ESG         | Цепочки дескрипторов не применяем
                                 + LSHIFT(0, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
                                 + LSHIFT(1, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
                                 + LSHIFT(1, 1)  // INTMAJOR    | Устанавливаем прерывание по окнчании пересылки DMA
                                 + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;

  audio_cbl.dma_tx_channel  = DMA_AUDIO_MF_CH;
  DMA_AUDIO_DMUX_PTR->CHCFG[DMA_AUDIO_MF_CH] = DMA_AUDIO_DMUX_SRC + BIT(7); // Через мультиплексор связываем сигнал от внешней периферии с входом выбранного канала DMA


  _int_install_isr(DMA_AUDIO_INT_NUM, _Audio_interrupt_handler, 0);
  _bsp_int_init(DMA_AUDIO_INT_NUM, AUDIO_DMA_PRIO, 0, TRUE);
  DMA->SERQ = DMA_AUDIO_MF_CH;
}


/*-----------------------------------------------------------------------------------------------------
  
  
  \param void  
-----------------------------------------------------------------------------------------------------*/
static void _Audio_start_DMA(void)
{
  DMA_MemMapPtr DMA  = DMA_BASE_PTR;
  DMA->TCD[DMA_AUDIO_MF_CH].CSR |= BIT(0);
}

/*-----------------------------------------------------------------------------------------------------
  
  
  \param void  
-----------------------------------------------------------------------------------------------------*/
static void _Audio_init_player_periphery(void)
{
  _Audio_config_DMA();                                        // Инициализируем DMA пересылающий 16-и битные отсчеты из памяти в DAC
  TPM_Init_for_audio(AUDIO_PLAYER_TIMER, AUDIO_DEFAULT_MOD); // Инициализируем таймер генерирующий сигналы запроса DMA
  _Audio_start_DMA();
}




/*------------------------------------------------------------------------------
   Заполнение аудиосэмплами тишины
 ------------------------------------------------------------------------------*/
static void _Audio_fill_for_silience(int16_t *dest, uint32_t samples_num)
{
  uint32_t i;
  for (i = 0; i < samples_num; i++)
  {
    *dest = DAC_MID_LEVEL;
    dest++;
  }
}


/*------------------------------------------------------------------------------
   Преобразование адиосэмплов файла в формат кодека
 
   Преобразование 512 отсчетов (1024 байта) длится 57 мкс = 18 мегабайт в сек
 ------------------------------------------------------------------------------*/
static void _Audio_convert_samlpes(int16_t *src, int16_t *dest, uint32_t samples_num)
{
  uint32_t i;
  int16_t s;

  for (i = 0; i < samples_num; i++)
  {
    s =*src;
    *dest =  s / 16 +  DAC_MID_LEVEL;
    dest++;
    src++;
  }
}


/*------------------------------------------------------------------------------
  Остановка воспроизведения очереди запросов
 ------------------------------------------------------------------------------*/
static void _Audio_stop_playing(void)
{
  MUTE_ON;
  _Audio_fill_for_silience(audio_buf[0], AUDIO_BUF_SAMPLES_NUM * 2);
  ply.mode = PLAYER_IDLE;
  ply.queue_sz = 0;
  ply.queue_tail = 0;
  ply.queue_head = 0;
}

/*------------------------------------------------------------------------------
   Извлечение из очереди идентификаторов воспроизводимых файлов данных очередного файла
 
   Процедура поиска, открытия файла и чтения блока данных занимает не менее 5.8 мс
 ------------------------------------------------------------------------------*/
static int _Audio_init_play_file(void)
{
  int             file_num;
  T_WAV_pars_err  res;

  //ITM_EVENT32(3,0);
  if (ply.queue_sz == 0) goto err_exit;

  // Получаем номер записи в массиве с параметрами файла
  file_num = ply.play_queue[ply.queue_tail];

  // Формируем путь к файлу
  strcpy(ply.file_name, VAnnouncer_get_files_dir());
  strncat(ply.file_name, sound_files_map[file_num].file_name, MAX_FILE_PATH_LEN - strlen(ply.file_name));
  // Открываем файл
  if (ply.fp != NULL)
  {
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Close previous file");
    _io_fclose(ply.fp);
  }
  ply.fp = _io_fopen(ply.file_name, "r");
  if (ply.fp == NULL)
  {
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "File %s opening error.", ply.file_name);
    goto err_exit;
  }
  else
  {
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "File %s opened Ok.", ply.file_name);
  }

  if (Read_wav_header_from_file(ply.fp) == 0)
  {
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Read file %s header error.", ply.file_name);
    goto err_exit;
  }
  res  = Wavefile_header_parsing();
  if (res != Valid_WAVE_File)
  {
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Parsing file %s error %d.", ply.file_name, res);
    goto err_exit;
  }
  _Audio_update_sample_rate(Get_wave_file_sample_rate());

  // Читаем во временный буфер данные из файла
  ply.block_sz = _io_read(ply.fp, ply.tmp_audio_buf, AUDIO_BUF_SAMPLES_NUM * SAMPLE_SIZE) / 2;
  if (ply.block_sz <= 0)
  {
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Reading file %s error.", ply.file_name);
    goto err_exit;
  }

  // Передвигаем указатель конца очереди номеров файлов на следующий номер
  ply.queue_tail++;
  if (ply.queue_tail >= PLAY_QUEUE_SZ)
  {
    ply.queue_tail = 0;
  }
  ply.queue_sz--;

  //ITM_EVENT32(3,1);
  return 1;
err_exit:

  // Передвигаем указатель конца очереди номеров файлов на следующий номер
  ply.queue_tail++;
  if (ply.queue_tail >= PLAY_QUEUE_SZ)
  {
    ply.queue_tail = 0;
  }
  ply.queue_sz--;

  //ITM_EVENT32(3,100);
  return 0;
}

/*------------------------------------------------------------------------------
   Заполнение аудио-буффера abuf данными из файла

   sz- количество 2-х байтных слов в буфере
 
   Процедура длится не менее 1110 мкс
 ------------------------------------------------------------------------------*/
static void _Audio_fill_buf_from_file(uint8_t bank, uint32_t position, int samples_num)
{
  int16_t   *buf_ptr;

  //ITM_EVENT32(3,2);


  buf_ptr = audio_buf[ply.bank] + position;

  if (samples_num > ply.block_sz)
  {
    // Достигнут конец файла
    _Audio_convert_samlpes(ply.tmp_audio_buf, buf_ptr, ply.block_sz); // Длительность 57 мкс для 512 отсчетов
    MUTE_OFF;
    _Audio_fill_for_silience(buf_ptr + ply.block_sz, samples_num - ply.block_sz);
    ply.block_sz = 0;
    ply.file_end = 1;
  }
  else
  {
    _Audio_convert_samlpes(ply.tmp_audio_buf, buf_ptr, samples_num); // Длительность 57 мкс для 512 отсчетов
    MUTE_OFF;
    ply.file_end = 0;
    if (samples_num == ply.block_sz)
    {
      // Воспроизвели столько же сколько прочитали из файла
      // Читаем следующий блок
      ply.block_sz = _io_read(ply.fp, ply.tmp_audio_buf, samples_num * SAMPLE_SIZE) / 2;
      if (ply.block_sz <= 0)
      {
        ply.file_end = 1;
        ply.block_sz = 0;
      }
    }
    else
    {
      // Мы воспроизвели меньше чем прочитали из файла на предыдущем этапе
      // Это прооисходит в начале воспроизведения файла, когда для аудиобуфера нужно меньше данных чем полный размер буфера
      // Здесь часть данных из временного буфера переносим в его начало, и остальное читаем из файла
      memcpy(ply.tmp_audio_buf, ply.tmp_audio_buf + samples_num,(AUDIO_BUF_SAMPLES_NUM - samples_num) * SAMPLE_SIZE);
      // Читаем недостающие данные во временный буфер
      ply.block_sz = _io_read(ply.fp, ply.tmp_audio_buf, samples_num * SAMPLE_SIZE) / 2;
      ply.block_sz += AUDIO_BUF_SAMPLES_NUM - samples_num;
    }
  }
  //ITM_EVENT32(3,3);

}

/*-----------------------------------------------------------------------------------------------------
  
  
  \param bank  
  \param position  
  \param samples_num  
-----------------------------------------------------------------------------------------------------*/
static void _Audio_fill_buf_w_sound(uint8_t bank, uint32_t position, int samples_num)
{
  int16_t   *buf_ptr;

  buf_ptr = audio_buf[ply.bank] + position;
  Generate_sound_to_buf(buf_ptr, samples_num);
}

/*-----------------------------------------------------------------------------------------------------
  Ожидание события 
-----------------------------------------------------------------------------------------------------*/
static uint32_t _Audio_wait_event(uint32_t ticks)
{
  // Ожидаем все возможные события
  if (_lwevent_wait_ticks(&play_lwev, 0xFFFFFFFF, FALSE, ticks) == MQX_OK)
  {
    uint32_t evt_flags = _lwevent_get_signalled();
    if (evt_flags &  EVT_DELAYED_STOP)
    {
      f_delayed_stop = 1;
      _time_get(&delayed_stop_start);
    }
    return evt_flags;
  }
  return 0;
}


/*-----------------------------------------------------------------------------------------------------
  
  
  \param v  
-----------------------------------------------------------------------------------------------------*/
uint32_t Audio_set_attenuation(uint32_t v)
{
  uint8_t buf[2];

  buf[0] = (uint8_t)(v & 0x3F);
  if (I2C_WriteRead(I2C2_INTF, I2C_7BIT_ADDR, 0x4B, 1, buf, 0, 0, 10) != MQX_OK)
  {
    ply.audio_ic_errors++;
    return RES_ERROR;
  }
  return RES_OK;
}


/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
void Play_sound(uint32_t tone_freq)
{
  ply.tone_freq = tone_freq;
  _lwevent_set(&play_lwev, EVT_PLAY_SOUND);
}

/*------------------------------------------------------------------------------
   Воспроизвести файл из постоянной памяти
   file - номер файла в массиве файлов
 ------------------------------------------------------------------------------*/
void Enqueue_file(int file)
{
  __disable_interrupt();
  if (ply.queue_sz < PLAY_QUEUE_SZ)
  {
    ply.play_queue[ply.queue_head] = file;
    ply.queue_head++;
    if (ply.queue_head >= PLAY_QUEUE_SZ)
    {
      ply.queue_head = 0;
    }
    ply.queue_sz++;
  }
  __enable_interrupt();

  _Audio_set_evt_to_play_file();
}

/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
void Stop_play(void)
{
  _lwevent_set(&play_lwev, EVT_STOP_PLAY);
}

/*------------------------------------------------------------------------------

 ------------------------------------------------------------------------------*/
void Delayed_Stop_play(void)
{
  _lwevent_set(&play_lwev, EVT_DELAYED_STOP);
}


/*------------------------------------------------------------------------------
 
 ------------------------------------------------------------------------------*/
static void Task_sound_player(uint32_t initial_data)
{
  uint32_t    evt_flags;
  uint8_t     bnk;

  _lwevent_create(&play_lwev, LWEVENT_AUTO_CLEAR); // Все события автоматически сбрасываемые

  _Audio_init_player_periphery();

  ply.mode = PLAYER_IDLE;

  do
  {
    evt_flags =  _Audio_wait_event(10);

    // Ожидаем сообщения о проигрывании файла

    if (evt_flags != 0)
    {
      // От возникновения прерывания устанавливающего флаг до выполения этого места проходит 6.58 мкс
      //ITM_EVENT32(2,evt_flags);

      // Если получили сигнал воспроизвести файл когда воспроизводился тон, то отменить воспроизведение тона
      if ((ply.mode == PLAYING_SOUND) && ((evt_flags & EVT_PLAY_FILE) != 0))
      {
        _Audio_stop_playing();
      }
      // Если получили сигнал воспроизвести тона когда воспроизводился файл, то отменить воспроизведение файла
      if ((ply.mode == PLAYING_FILE) && ((evt_flags & EVT_PLAY_SOUND) != 0))
      {
        _Audio_stop_playing();
      }

      // Приоритет отдаем воспроизведению тона
      if ((evt_flags & EVT_PLAY_SOUND) && (ply.mode == PLAYER_IDLE))
      {
        // Начинаем воспроизведение звука из состояния тишины

        // Ожидаем начало цикла DMA, чтобы было достаточно времени заплнить доступный на запись буфер
        do
        {
          evt_flags =  _Audio_wait_event(10);
        } while ((evt_flags & EVT_READ_NEXT) == 0);


        bnk = _Audio_get_current_DMA_buf();    // Определяем из какогор буфера в данный момент идет воспроизведение
        ply.mode = PLAYING_SOUND;
        ply.bank = bnk ^ 1;                    // Устанавливаемся на буфер готовый к перезаписи данных
        Tone_gen_start(ply.tone_freq);
        _Audio_fill_buf_w_sound(ply.bank, 0 , AUDIO_BUF_SAMPLES_NUM);
        MUTE_OFF;
        //_Audio_set_evt_to_read_next_block(); // Сразу сгенерировать событие на чтение следующего блока данных если файл продолжается
      }
      else if ((evt_flags & EVT_PLAY_FILE) && (ply.mode == PLAYER_IDLE))
      {
        // Начинаем воспроизведение файла из состояния тишины

        // Ожидаем начало цикла DMA, чтобы было достаточно времени заплнить доступный на запись буфер
        do
        {
          evt_flags =  _Audio_wait_event(10);
        } while ((evt_flags & EVT_READ_NEXT) == 0);

        if (ply.queue_sz > 0)  // Извлечь из очереди номер файла и инициализировать воспроизведение
        {
          if (_Audio_init_play_file() != 0)
          {
            bnk = _Audio_get_current_DMA_buf();  // Определяем из какогор буфера в данный момент идет воспроизведение.
            ply.mode = PLAYING_FILE;
            ply.bank = bnk ^ 1;                     // Устанавливаемся на буфер готовый к перезаписи данных
            _Audio_fill_buf_from_file(ply.bank, 0, AUDIO_BUF_SAMPLES_NUM);
            //_Audio_set_evt_to_read_next_block();    // Сразу сгенерировать событие на чтение следующего блока данных если файл продолжается
          }
        }
      }
      else if (evt_flags & EVT_READ_NEXT)
      {
        // Обрабатываем событие заполнения следующего аудиобуфера

        bnk = _Audio_get_current_DMA_buf();     // Определяем из какогор буфера в данный момент идет воспроизведение
        ply.bank = bnk ^ 1;
        ;                    // Устанавливаемся на буфер готовый к перезаписи данных

        if (ply.mode == PLAYING_SOUND)
        {
          // Если воспроизводим простой звук, то заполняем им следующий аудиобуффер
          _Audio_fill_buf_w_sound(ply.bank, 0, AUDIO_BUF_SAMPLES_NUM);
        }
        else if (ply.mode == PLAYING_FILE)
        {
          if (ply.block_sz == 0)
          {
            if (ply.queue_sz != 0)
            {
              // Начать воспроизводить новый файл из очереди
              if (_Audio_init_play_file() != 0)
              {
                _Audio_fill_buf_from_file(ply.bank,0, AUDIO_BUF_SAMPLES_NUM);
              }
              else
              {
                _Audio_stop_playing();
              }
            }
            else
            {
              if (ply.file_end == 1)
              {
                _Audio_stop_playing();
              }
            }
          }
          else
          {
            _Audio_fill_buf_from_file(ply.bank, 0 , AUDIO_BUF_SAMPLES_NUM);
          }
        }
      }
      else if (evt_flags & EVT_STOP_PLAY)
      {
        _Audio_stop_playing();
      }
    }

    if (f_delayed_stop != 0)
    {
      if (Time_elapsed_ms(&delayed_stop_start) > DEALAYED_STOP_TIMEOUT_MS)
      {
        f_delayed_stop = 0;
        _Audio_stop_playing();
      }
    }

  }while (1);
}


/*-----------------------------------------------------------------------------------------------------
  
  
-----------------------------------------------------------------------------------------------------*/
void  Player_task_create(void)
{
  _mqx_uint res;

  TASK_TEMPLATE_STRUCT  task_template = {0};
  task_template.TASK_NAME          = "Player";
  task_template.TASK_PRIORITY      = PLAYER_PRIO;
  task_template.TASK_STACKSIZE     = 2048;
  task_template.TASK_ADDRESS       = Task_sound_player;
  task_template.TASK_ATTRIBUTES    = MQX_FLOATING_POINT_TASK;
  task_template.CREATION_PARAMETER = 0;
  player_task_id =  _task_create(0, 0, (uint32_t)&task_template);
}

