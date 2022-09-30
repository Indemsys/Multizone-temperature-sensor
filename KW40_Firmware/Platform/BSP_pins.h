#ifndef BSP_PINS_H
  #define BSP_PINS_H


void      BSP_Init_Pins(void);
void      Set_SPI_CS_state(int state);
void      Set_PTC7_state(int state);
uint32_t  Get_PTC7_state(void);
void      Set_PTC6_state(int state);
uint32_t  Get_PTC6_state(void);

#endif // BSP_PINS_H



