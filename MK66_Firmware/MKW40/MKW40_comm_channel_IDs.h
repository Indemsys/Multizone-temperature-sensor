#ifndef APP_COMM_CHANNEL_IDS_H
#define APP_COMM_CHANNEL_IDS_H


// Идентификаторы подписчиков приемников пакетов из канала связи MKW40
// Эти же идентификаторы являются индексами в массиве подписки
#define MKW40_CH_VUART       1     // Идентификатор пакета для виртуального UART-а
#define MKW40_CH_FILEMAN     2     // Идентификатор пакета для файлового менеджера
#define MKW40_CH_CMDMAN      3     // Идентификатор пакета для менеджера команд
#define MKW40_CH_SETT_DONE   4     // Команда активизации BLE стека
#define MKW40_CH_SETT_WRITE  5     // Запись данных параметризации BLE стека
#define MKW40_CH_RESET       6     // Команда сброса чипа MKW40
#define MKW40_CH_ACK         0x55  // Подтверждение получения данных
#define MKW40_CH_NACK        0xAA  // Сообщение об ошибке получения данных 


#define BLE_PARAM_POS        5
#define BLE_PARAM_MASK       0x07
#define BLE_PARAM_SZ_MASK    0x1F

#define PAR_PIN_CODE         0
#define PAR_ADV_DEV_NAME     1
#define PAR_SOFT_REV         2



#define MKW_SUBSCR_NAX_CNT   3 // Колическтво элементов в массиве подписок на прием пакетов из канала свзя с MKW40



#endif // APP_COMM_CHANNEL_IDS_H



