#ifndef APP_COMMUNICATIONSERVICE_H
#define APP_COMMUNICATIONSERVICE_H


uint8_t  Write_to_value_k66wrdata(deviceId_t peer_device_id, uint16_t attr_handle, uint8_t *value, uint16_t len);
uint8_t  Write_to_value_k66rddata(deviceId_t peer_device_id, uint16_t attr_handle, uint8_t *value, uint16_t len);
void     Read_from_value_k66rddata(void);
int32_t  Notif_ready_k66rddata(uint8_t *dbuf, uint16_t len);

void     Test_vuart_receiving(void);


#endif // APP_COMMUNICATIONSERVICE_H



