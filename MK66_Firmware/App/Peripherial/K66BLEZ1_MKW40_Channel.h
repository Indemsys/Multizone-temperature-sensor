#ifndef K66BLEZ1_MKW40_CHANNEL_H
  #define K66BLEZ1_MKW40_CHANNEL_H


typedef struct
{
  uint8_t       tx_ch;            // Номер дескриптора DMA на который указывает mem_ch
  uint8_t       rx_ch;            // Номер дескриптора DMA предлназначенного для приема
  uint8_t       minor_tranf_sz;   // Количество пересылаемых байт в минорном цикле

} T_DMA_SPI_cbl;


typedef struct
{
  uint8_t             ch;         // номер канала DMA пересылающий данные из FIFO SPI в буффер с данными
  uint8_t             *databuf;   // указатель на исходный буфер с данными
  uint32_t            datasz;     // размер исходного буфера данных
  uint32_t            spi_popr;   // адрес регистра POPR SPI
  DMAMUX_MemMapPtr    DMAMUX;
  uint32_t            dmux_src;   // номер входа периферии для выбранного мультиплексора DMAMUX для передачи на DMA. Например DMUX0_SRC_SPI0_RX
  uint8_t             minor_tranf_sz; // Количество пересылаемых байт в минорном цикле

} T_DMA_SPI_RX_config;

typedef struct
{
  uint8_t             ch;         // номер канала DMA пересылающий данные из буффер с данными в FIFO SPI
  uint8_t             *databuf;   // указатель на исходный буфер с данными
  uint32_t            datasz;     // размер исходного буфера данных
  uint32_t            spi_pushr;  // адрес регистра PUSHR SPI
  DMAMUX_MemMapPtr    DMAMUX;
  uint32_t            dmux_src;   // номер входа периферии для выбранного мультиплексора DMAMUX для передачи на DMA. Например DMUX0_SRC_SPI0_TX
  uint8_t             minor_tranf_sz; // Количество пересылаемых байт в минорном цикле

} T_DMA_SPI_TX_config;



void Config_DMA_for_SPI_TX(T_DMA_SPI_TX_config *cfg, T_DMA_SPI_cbl *pDS_cbl);
void Config_DMA_for_SPI_RX(T_DMA_SPI_RX_config *cfg, T_DMA_SPI_cbl *pDS_cbl);

void Init_MKW40_channel(void);
_mqx_uint MKW40_SPI_send_buf(const uint8_t *buff, uint32_t sz);
_mqx_uint MKW40_SPI_read_buf(const uint8_t *buff, uint32_t sz);
_mqx_uint MKW40_SPI_write_read_buf(const uint8_t *wbuff, uint32_t wsz, const uint8_t *rbuff, uint32_t rsz);


_mqx_uint MKW40_SPI_slave_read_buf(const uint8_t *buff, uint32_t sz);
_mqx_uint MKW40_SPI_slave_read_write_buf(const uint8_t *outbuff, uint8_t *inbuff, uint32_t sz);

#endif // K66BLEZ1_MKW40_CHANNEL_H



