#ifndef __SOUND_GEN
  #define __SOUND_GEN

/*
  Частота дискретизации ШИМ = 8000 Гц
  Таблица генерируемых частот и соответствующих приращений фаз для них в 32-x битном представлении
*/
#define DAC_TIMER_CLOCK 108000000ul

#define DACT_PRESC_8000KHZ   ( DAC_TIMER_CLOCK/8000 )
#define DACT_PRESC_11025KHZ  ( DAC_TIMER_CLOCK/11025 )
#define DACT_PRESC_22050KHZ  ( DAC_TIMER_CLOCK/22050 )
#define DACT_PRESC_44100HZ   ( DAC_TIMER_CLOCK/44100 )


#define FS                    8000ul
#define PH_MAX                0x100000000ull

#define ph777    (777*(PH_MAX/FS))    //


void      Tone_gen_start(unsigned int freq);
int16_t   Get_tone_sample(void);
void      Generate_sound_to_buf(int16_t *abuf, int samples_num);
#endif
