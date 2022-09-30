#ifndef BSP_H
  #define BSP_H

#include   <stdint.h>
#include   <string.h>

#include   "MKW40Z4.h"
#include   "BSP_clocks.h"
#include   "BSP_pins.h"
#include   "BSP_SPI.h"
#include   "BSP_PIT.h"
#include   "BSP_DMA.h"

#define BIT(n) (1u << n)
#define LSHIFT(v,n) (((unsigned int)(v) << n))




#define CLOCK_VLPR                       1U
#define CLOCK_RUN                        2U
#define CLOCK_NUMBER_OF_CONFIGURATIONS   3U

#ifndef CLOCK_INIT_CONFIG
  #define CLOCK_INIT_CONFIG              CLOCK_RUN
#endif

#if (CLOCK_INIT_CONFIG == CLOCK_RUN)
  #define CORE_CLOCK_FREQ 32000000U
#else
  #define CORE_CLOCK_FREQ 4000000U
#endif

/* Connectivity */
#ifndef APP_SERIAL_INTERFACE_TYPE
  #define APP_SERIAL_INTERFACE_TYPE (gSerialMgrLpuart_c)
#endif

#ifndef APP_SERIAL_INTERFACE_INSTANCE
  #define APP_SERIAL_INTERFACE_INSTANCE (0)
#endif

/* RTC external clock configuration. */
#define RTC_XTAL_FREQ                      32768U
#define RTC_SC2P_ENABLE_CONFIG             false
#define RTC_SC4P_ENABLE_CONFIG             false
#define RTC_SC8P_ENABLE_CONFIG             false
#define RTC_SC16P_ENABLE_CONFIG            false
#define RTC_OSC_ENABLE_CONFIG              true

#define BOARD_LTC_INSTANCE                 0


#endif // BSP_H



