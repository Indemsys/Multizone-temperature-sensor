#ifndef K66BLEZ1_FTM_H
  #define K66BLEZ1_FTM_H

#define FTM_SYSCLK                  (60000000ul)

#define FTM2_BASE_PTR_AIPS1         ((FTM_MemMapPtr)0x400B8000u)


#define FTM_PRESC_1    0
#define FTM_PRESC_2    1
#define FTM_PRESC_4    2
#define FTM_PRESC_8    3
#define FTM_PRESC_16   4
#define FTM_PRESC_32   5
#define FTM_PRESC_64   6
#define FTM_PRESC_128  7

#define FTM_CH_0    0
#define FTM_CH_1    1
#define FTM_CH_2    2
#define FTM_CH_3    3
#define FTM_CH_4    4
#define FTM_CH_5    5
#define FTM_CH_6    6
#define FTM_CH_7    7



#define PH_A_H_SW   BIT(0) 
#define PH_A_L_SW   BIT(1) 
#define PH_B_H_SW   BIT(2) 
#define PH_B_L_SW   BIT(3) 
#define PH_C_H_SW   BIT(4) 
#define PH_C_L_SW   BIT(5) 

// Устновка скважности ШИМ импульсов. 
// Значение меньше 3 не таймером отрабатыватся не успевают на частоте 60 МГц с предделителем установленным в 0 
#define FTM_WS2812B_MOD 75 // 1.25 мкс
#define FTM_WS2812B_1   48 // 0.8 мкс
#define FTM_WS2812B_0   24 // 0.4 мкс 

#define QDEC_CNT_MOD 4


typedef  void (*T_qdec_isr)(uint32_t dir);



void                   FTM1_init_QDEC(T_qdec_isr isr);
void                   FTM2_init_QDEC(T_qdec_isr isr);
void                   FTM_init_WS2812_PWM_DMA(FTM_MemMapPtr FTM);
void                   FTM_init_BLDC_PWM(FTM_MemMapPtr FTM, uint32_t mod);
void                   FTM_init_PWM(FTM_MemMapPtr FTM, uint32_t mod);
void                   FTM1_FTM2_init_hall_sens_meas(uint8_t presc, uint32_t mod);
void                   FTM_set_presc_mod(FTM_MemMapPtr FTM, uint8_t presc, uint16_t mod);
void                   FTM_set_CnV(FTM_MemMapPtr FTM, uint32_t val);
void                   FTM_start(FTM_MemMapPtr FTM, uint8_t en_int);
void                   FTM_stop(FTM_MemMapPtr FTM);
void                   FTM_set_PWM(FTM_MemMapPtr FTM, uint8_t ch, uint32_t percent);
#endif // K66BLEZ1_FTM_H



