#ifndef K66BLE_COMMANDSERVICE_H
  #define K66BLE_COMMANDSERVICE_H


uint8_t    CommandService_write(deviceId_t peer_device_id, uint16_t handle, uint8_t *value, uint16_t len);
void       CommandService_read(void);
uint32_t   CommandService_write_attribute(uint8_t *data, uint32_t sz);
#endif // K66BLE_COMMANDSERVICE_H



