#ifndef MZTS_DS1WIRE_INTF_H
  #define MZTS_DS1WIRE_INTF_H


#define ONEWIRE_UART_PORT 4

void     OneWire_UART_config(void);
void     OneWire_UART_init(uint32_t baud_rate);
void     OneWire_UART_send_byte(uint8_t val);
uint32_t OneWire_UART_wait_transmit_complete(uint32_t timeout);
void     OneWire_UART_assign_rx_data_wait(uint8_t *buf, uint32_t sz);
uint32_t OneWire_UART_wait_data(uint32_t timeout);
uint32_t OneWire_wait_presence_pulse(void);

#endif // MZTS_DS1WIRE_INTF_H



