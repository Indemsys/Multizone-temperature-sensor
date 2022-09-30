#ifndef MZTS_SPI_BUS_H
  #define MZTS_SPI_BUS_H


void      SPI0_bus_lock_for_TFT(void);
void      SPI0_bus_lock_for_LSM6DSO32(void);
void      SPI0_bus_unlock(void);
void      SPI0_Init_Bus(void);



#endif // MZTS_SPI_BUS_H



