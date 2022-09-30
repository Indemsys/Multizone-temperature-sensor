#ifndef __K66BLEZ_H
  #define __K66BLEZ_H

#include "K66BLEZ1_PINS.h"
#include "K66BLEZ1_INIT_SYS.h"
#include "K66BLEZ1_ADC.h"
#include "K66BLEZ1_DAC.h"
#include "K66BLEZ1_DMA.h"
#include "K66BLEZ1_PIT.h"
#include "K66BLEZ1_SPI.h"
#include "K66BLEZ1_FTM.h"
#include "K66BLEZ1_CAN.h"
#include "K66BLEZ1_VBAT_RAM.h"
#include "K66BLEZ1_I2C.h"
#include "K66BLEZ1_UART.h"
#include "K66BLEZ1_TPM.h"
#include "K66BLEZ1_MKW40_Channel.h"

#define I2C0_INTF   0
#define I2C1_INTF   1
#define I2C2_INTF   2
#define I2C3_INTF   3

// -----------------------------------------------------------------------------
//  Статическое конфигурирование каналов DMA и DMA MUX для работы с ADC

#define DMA_ADC0_RES_CH          20                     // Занятый канал DMA для обслуживания ADC
#define DMA_ADC1_RES_CH          21                     // Занятый канал DMA для обслуживания ADC
#define DMA_ADC0_CFG_CH          16                     // Занятый канал DMA для обслуживания ADC
#define DMA_ADC1_CFG_CH          17                     // Занятый канал DMA для обслуживания ADC
#define DMA_ADC_DMUX_PTR         DMAMUX_BASE_PTR        // Указатель на модуль DMUX который используется для передачи сигналов от ADC к DMA
#define DMA_ADC0_DMUX_SRC        DMUX_SRC_ADC0          // Входы DMUX используемые для выбранного ADC
#define DMA_ADC1_DMUX_SRC        DMUX_SRC_ADC1          // Входы DMUX используемые для выбранного ADC
#define DMA_ADC_INT_NUM          INT_DMA4_DMA20         // Номер вектора прерывания используемый в DMA для обслуживания ADC

// -----------------------------------------------------------------------------
//  Статическое конфигурирование каналов DMA и DMA MUX для работы с интерфейсом SPI чипа MKW40

#define MKW40_SPI                SPI2              // Номер SPI модуля используемый для коммуникации с модулем MKW40
#define DMA_MKW40_FM_CH          0                 // Канал DMA для обслуживания приема по   SPI. FIFO->Память.
#define DMA_MKW40_MF_CH          1                 // Канал DMA для обслуживания передачи по SPI. Память->FIFO.
#define MKW40_SPI_CS             0                 // Номер аппаратного внешнего сигнала CS для интерфейса MKW40 SPI
#define DMA_MKW40_DMUX_PTR       DMAMUX_BASE_PTR   // Указатель на модуль DMUX который используется для передачи сигналов от контроллера SPI к DMA
#define DMA_MKW40_DMUX_TX_SRC    DMUX_SRC_FTM3_CH7_SPI2_TX // Вход DMUX используемый для вызова передачи по DMA
#define DMA_MKW40_DMUX_RX_SRC    DMUX_SRC_FTM3_CH6_SPI2_RX // Вход DMUX используемый для вызова приема по DMA

#define DMA_MKW40_RX_INT_NUM     INT_DMA0_DMA16    // Номер вектора прерывания используемый в DMA для обслуживания приема по SPI
#define DMA_MKW40_ISR            DMA0_DMA16_IRQHandler

// -----------------------------------------------------------------------------
//  Статическое конфигурирование каналов DMA и DMA MUX для работы с на вывод в LED ленту
//  Прерывания DMA не используются

#define DMA_LEDSTRIP_CH          6                 // Канал DMA для обслуживания передачи в LED ленту
#define DMA_LEDSTRIP_DMUX_PTR    DMAMUX_BASE_PTR   // Указатель на модуль DMUX который используется для передачи сигналов от контроллера SPI к DMA
#define DMA_LEDSTRIP_DMUX_SRC    DMUX_SRC_FTM0_CH6 // Вход DMUX используемый для вызова передачи по DMA
#define DMA_LEDSTRIP_INT_NUM     INT_DMA6_DMA22


// -----------------------------------------------------------------------------
//  Статическое конфигурирование каналов DMA и DMA MUX для работы с интерфейсом SPI дисплея и акселерометра
//
//  Даже если используется только запись по SPI, канал чтения необходим для формирования прерывания по окончании передачи.
//  Поскольку только завершение чтения говорит о том что процесс передачи из сдвигового регистра SPI польностью завершен!
//
#define BUS_SPI0                SPI0              // Номер SPI модуля используемый для коммуникации с дисплеем и акселерометром
#define DMA_LCD_FM_CH           6
#define DMA_LCD_MF_CH           7                 // Канал DMA для обслуживания передачи по SPI. Память->FIFO. Должен иметь более высокий приоритет чем канал FIFO->Память
#define LCD_SPI_CS              SPI_CS0           // Номер аппаратного внешнего сигнала CS для дисплея
#define DMA_LCD_DMUX_PTR        DMAMUX_BASE_PTR   // Указатель на модуль DMUX который используется для передачи сигналов от контроллера SPI к DMA
#define DMA_LCD_DMUX_TX_SRC     DMUX_SRC_SPI0_TX  // Вход DMUX используемый для вызова передачи по DMA
#define DMA_LCD_DMUX_RX_SRC     DMUX_SRC_SPI0_RX  // Вход DMUX используемый для вызова приема по DMA

#define DMA_LCD_RX_INT_NUM      INT_DMA6_DMA22    // Номер вектора прерывания используемый в DMA для обслуживания приема по SPI
#define DMA_LCD_ISR             DMA6_DMA22_IRQHandler


// -----------------------------------------------------------------------------
//  Статическое конфигурирование каналов DMA и DMA MUX для работы с интерфейсом UART MODBUS master
#define DMA_UART_MF_CH           2                 // Канал DMA для обслуживания передачи по UART. Память->FIFO.
#define DMA_UART_DMUX_PTR        DMAMUX_BASE_PTR   // Указатель на модуль DMUX который используется для передачи сигналов от контроллера UART к DMA
#define DMA_UART_TX_INT_NUM      INT_DMA2_DMA18    // Номер вектора прерывания используемый в DMA для обслуживания приема по UART


// -----------------------------------------------------------------------------
//  Статическое конфигурирование каналов DMA и DMA MUX для для воспроизведления звука
#define DMA_AUDIO_MF_CH            3                 // Канал DMA для обслуживания передачи в DAC. Память->периферия.
#define DMA_AUDIO_DMUX_PTR         DMAMUX_BASE_PTR   // Указатель на модуль DMUX который используется для передачи
#define DMA_AUDIO_DMUX_SRC         DMUX_SRC_IEEE1588_TIMER1_OVF // Запрос от флага переполнения таймера TPM1
#define DMA_AUDIO_INT_NUM          INT_DMA3_DMA19    // Номер вектора прерывания используемый в DMA для обслуживания передачи


#endif
