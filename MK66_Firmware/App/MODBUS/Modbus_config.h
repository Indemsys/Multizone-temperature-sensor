

#define  MODBUS_PRIO                      6      // Приоритет задачи MODBUS один из самых высоких
#define  MODBUS_STACK                     3000   


#define  MODBUS_CFG_SLAVE_EN              0       // Разрешаем режима слэйва
#define  MODBUS_SLAVE_RX_PRIO             10
#define  MODBUS_SLAVE_RX_STACK            512


#define  MODBUS_CFG_MASTER_EN             1       // Разрешение режима мастера
                                                  
                                                  
#define  MODBUS_CFG_ASCII_EN              1       // Разрешение формата ASCII
#define  MODBUS_CFG_RTU_EN                1       // Разрешение формата RTU
                                                  
                                                  
#define  MODBUS_CFG_MAX_CH                1       // Максимальное количество каналов MODBUS

#define  MODBUS_CFG_BUF_SIZE              255     // Максимальна длина пакетов MODBUS на прием и на передачу


#define  MODBUS_CFG_FP_EN                 1       // Разрешение поддержки плавающей запятой

#define  MODBUS_ANS_LATENTION_STAT        1       // Разрешение ведения статистики времени задержки ответа


// Разрешения функций
#define  MODBUS_CFG_FC01_EN               1   
#define  MODBUS_CFG_FC02_EN               1
#define  MODBUS_CFG_FC03_EN               1
#define  MODBUS_CFG_FC04_EN               1
#define  MODBUS_CFG_FC05_EN               1
#define  MODBUS_CFG_FC06_EN               1
#define  MODBUS_CFG_FC08_EN               1
#define  MODBUS_CFG_FC15_EN               1
#define  MODBUS_CFG_FC16_EN               1
#define  MODBUS_CFG_FC20_EN               0
#define  MODBUS_CFG_FC21_EN               0

