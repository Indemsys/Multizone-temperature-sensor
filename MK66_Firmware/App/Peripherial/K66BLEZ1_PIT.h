#ifndef __K66BLEZ_PIT
  #define __K66BLEZ_PIT

#define PIT_CHANNELS_NUM  4

typedef void (*T_PIT_callback)(void);

void     PIT_init_module(void);

void     PIT_init_interrupt(uint8_t chnl, uint32_t period, uint8_t prio, T_PIT_callback callback_func);
void     PIT_disable(uint8_t chnl);

void     PIT_init_for_ADC_trig(uint8_t chnl, uint32_t period, uint32_t scan_sz);
uint32_t PIT_get_curr_val(uint8_t chnl);
uint32_t PIT_get_load_val(uint8_t chnl);

#endif
