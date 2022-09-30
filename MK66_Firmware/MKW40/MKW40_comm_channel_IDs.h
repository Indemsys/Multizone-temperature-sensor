#ifndef APP_COMM_CHANNEL_IDS_H
#define APP_COMM_CHANNEL_IDS_H


// �������������� ����������� ���������� ������� �� ������ ����� MKW40
// ��� �� �������������� �������� ��������� � ������� ��������
#define MKW40_CH_VUART       1     // ������������� ������ ��� ������������ UART-�
#define MKW40_CH_FILEMAN     2     // ������������� ������ ��� ��������� ���������
#define MKW40_CH_CMDMAN      3     // ������������� ������ ��� ��������� ������
#define MKW40_CH_SETT_DONE   4     // ������� ����������� BLE �����
#define MKW40_CH_SETT_WRITE  5     // ������ ������ �������������� BLE �����
#define MKW40_CH_RESET       6     // ������� ������ ���� MKW40
#define MKW40_CH_ACK         0x55  // ������������� ��������� ������
#define MKW40_CH_NACK        0xAA  // ��������� �� ������ ��������� ������ 


#define BLE_PARAM_POS        5
#define BLE_PARAM_MASK       0x07
#define BLE_PARAM_SZ_MASK    0x1F

#define PAR_PIN_CODE         0
#define PAR_ADV_DEV_NAME     1
#define PAR_SOFT_REV         2



#define MKW_SUBSCR_NAX_CNT   3 // ����������� ��������� � ������� �������� �� ����� ������� �� ������ ���� � MKW40



#endif // APP_COMM_CHANNEL_IDS_H



