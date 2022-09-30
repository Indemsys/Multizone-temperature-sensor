#ifndef APP_DEBUG_PRINT_H
  #define APP_DEBUG_PRINT_H

  #include "SEGGER_RTT.h"

//#define TRACE_BLE           // Макрос разрешающий вывод отладочныхх сообщений при вызове основных функций стека BLE
//#define LOG_PERFOMANCE_TEST // Трассировка теста производительности
//#define LOG_CMDSERV_SERV    // Трассировка обращений к аттрибутам сервиса комманд
//#define LOG_VUART_SERV      // Трассировка сервиса виртуального UART-а
//#define TRACE_CALLBACKS     // Трассировка вызовов callback функций
//#define TRACE_MEM_MAN       // Трассировка работы менеджера пулов памяти блоков фиксированной длинны

  #ifdef TRACE_BLE
    #define DEBUG_PRINT( X )           SEGGER_RTT_printf( 0, X )
    #define DEBUG_PRINT_ARG( X, ... )  SEGGER_RTT_printf( 0, X, __VA_ARGS__)
  #else
    #define DEBUG_PRINT( X )
    #define DEBUG_PRINT_ARG( X, ... )
  #endif

  #ifdef TRACE_CALLBACKS
    #define DEBUG_CALLBACK_PRINT( X )           SEGGER_RTT_printf( 0, X )
    #define DEBUG_CALLBACK_PRINT_ARG( X, ... )  SEGGER_RTT_printf( 0, X, __VA_ARGS__)
  #else
    #define DEBUG_CALLBACK_PRINT( X )
    #define DEBUG_CALLBACK_PRINT_ARG( X, ... )
  #endif

  #ifdef TRACE_MEM_MAN
    #define DEBUG_MEM_MAN_PRINT( X )           SEGGER_RTT_printf( 0, X )
    #define DEBUG_MEM_MAN_PRINT_ARG( X, ... )  SEGGER_RTT_printf( 0, X, __VA_ARGS__)
  #else
    #define DEBUG_MEM_MAN_PRINT( X )
    #define DEBUG_MEM_MAN_PRINT_ARG( X, ... )
  #endif


  #ifdef LOG_PERFOMANCE_TEST
    #define  DEBUG_PERF_TEST_PRINT_ARG(X, ...) SEGGER_RTT_printf(0, X, __VA_ARGS__)
  #else
    #define  DEBUG_PERF_TEST_PRINT_ARG(X, ...)
  #endif


  #ifdef LOG_CMDSERV_SERV
    #define  DEBUG_CMDSERV_PRINT_ARG(X, ...) SEGGER_RTT_printf(0, X, __VA_ARGS__)
  #else
    #define  DEBUG_CMDSERV_PRINT_ARG(X, ...)
  #endif

  #ifdef LOG_VUART_SERV
    #define  DEBUG_VUARTSERV_PRINT_ARG(X, ...) SEGGER_RTT_printf(0, X, __VA_ARGS__)
  #else
    #define  DEBUG_VUARTSERV_PRINT_ARG(X, ...)
  #endif

#endif // DEBUG_PRINT_H



