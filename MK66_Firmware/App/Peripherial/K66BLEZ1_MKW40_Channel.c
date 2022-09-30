// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.07.29
// 13:50:42
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

extern const T_SPI_modules spi_mods[3];
extern T_SPI_cbls  spi_cbl[3];

#define  MKW40_RXEND   BIT(1)  // ���� ������������ �� ISR �� ��������� ������ �� SPI

static T_DMA_SPI_cbl DS_cbl;

static volatile uint8_t   dummy_tx = 0xFF;
static volatile uint8_t   dummy_rx;
/*-----------------------------------------------------------------------------------------------------

 \param rt_size

 \return uint8_t
-----------------------------------------------------------------------------------------------------*/
static uint8_t _Get_attr_size(uint8_t rt_size)
{
  switch (rt_size)
  {
  case 1:
    return 0;
  case 2:
    return 1;
  case 4:
    return 2;
  default:
    return 0;
  }
}
/*-------------------------------------------------------------------------------------------------------------
  �������������� DMA ��� �������� ������ �� SPI
  ��������� �������� �������� �������� � ��������� cfg
  ��������� pDS_cbl ����������� ��� ������������ ������������� ��������� ��������
-------------------------------------------------------------------------------------------------------------*/
void Config_DMA_for_SPI_TX(T_DMA_SPI_TX_config *cfg, T_DMA_SPI_cbl *pDS_cbl)
{
  uint8_t   k;
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  pDS_cbl->minor_tranf_sz = cfg->minor_tranf_sz;


  DMA->TCD[cfg->ch].SADDR = 0;                         // �������� - ����� � ������� (�� ���� ������������ �� �������������!!!)
  DMA->TCD[cfg->ch].SOFF = cfg->minor_tranf_sz;        // ����� ��������� �������  ����� ������ ��������
  DMA->TCD[cfg->ch].SLAST = 0;                         // �� ������������ ����� ��������� ����� ���������� ����� ����� DMA (��������� ��������� �����)
  DMA->TCD[cfg->ch].DADDR = (uint32_t)cfg->spi_pushr;  // ����� ��������� - ������� PUSHR SPI
  DMA->TCD[cfg->ch].DOFF = 0;                          // �����  ������ ��������� ��������� �� �������
  DMA->TCD[cfg->ch].DLAST_SGA = 0;                     // ������� ������������ �� ���������
  DMA->TCD[cfg->ch].NBYTES_MLNO = cfg->minor_tranf_sz; // ���������� ���� ������������ �� ���� ������ DMA (� �������� �����)
  DMA->TCD[cfg->ch].BITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | �������� �� ���������
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  | (�� ���� ������������ �� �������������!!!)
  ;
  DMA->TCD[cfg->ch].CITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | �������� �� ���������
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  | (�� ���� ������������ �� �������������!!!)
  ;
  DMA->TCD[cfg->ch].ATTR = 0
    + LSHIFT(0, 11) // SMOD  | ������ ������ ��������� �� ����������
    + LSHIFT(_Get_attr_size(cfg->minor_tranf_sz), 8)  // SSIZE | ������ ���������� ��� ������������ �� ���� �������� ������ ���������
    + LSHIFT(0, 3)  // DMOD  | ������ ������ ���������
    + LSHIFT(_Get_attr_size(cfg->minor_tranf_sz), 0)  // DSIZE | ������ ���������� ��� ������������ �� ���� �������� ������ � ��������
  ;
  DMA->TCD[cfg->ch].CSR = 0
    + LSHIFT(0, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
    + LSHIFT(0, 8)  // MAJORLINKCH | �������� �� ���������
    + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
    + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
    + LSHIFT(0, 5)  // MAJORELINK  | �������� �� ���������
    + LSHIFT(0, 4)  // ESG         | ������� ������������ �� ���������
    + LSHIFT(1, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
    + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
    + LSHIFT(0, 1)  // INTMAJOR    | �� ���������� ���������� �� �������� ��������� DMA
    + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;

  pDS_cbl->tx_ch  = cfg->ch;
  {
    uint8_t indx = cfg->ch;
    cfg->DMAMUX->CHCFG[indx] = cfg->dmux_src + BIT(7); // ����� ������������� ��������� ������ �� ������� ��������� (����� �� ������ SPI) � ������ ���������� ������ DMA
  }

}

/*-------------------------------------------------------------------------------------------------------------
  �������������� DMA ��� ������ ������ �� SPI
  ��������� �������� �������� �������� � ��������� cfg
  ��������� pDS_cbl ����������� ��� ������������ ������������� ��������� ��������
-------------------------------------------------------------------------------------------------------------*/
void Config_DMA_for_SPI_RX(T_DMA_SPI_RX_config *cfg, T_DMA_SPI_cbl *pDS_cbl)
{
  uint8_t   k;
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  pDS_cbl->minor_tranf_sz = cfg->minor_tranf_sz;


  DMA->TCD[cfg->ch].SADDR = (uint32_t)cfg->spi_popr;   // �������� - FIFO ��������� SPI
  DMA->TCD[cfg->ch].SOFF = 0;                          // ����� ��������� �� ��������
  DMA->TCD[cfg->ch].SLAST = 0;                         // �� ������������ ����� ��������� ����� ���������� ����� ����� DMA (��������� ��������� �����)
  DMA->TCD[cfg->ch].DADDR = 0;                         // ����� ��������� - ����� ������. �� ���� ������������ �� �������������
  DMA->TCD[cfg->ch].DOFF = cfg->minor_tranf_sz;        // �����  ������ ��������� ��������� �������
  DMA->TCD[cfg->ch].DLAST_SGA = 0;                     // ������� ������������ �� ���������
  DMA->TCD[cfg->ch].NBYTES_MLNO = cfg->minor_tranf_sz; // ���������� ���� ������������ �� ���� ������ DMA (� �������� �����)
  DMA->TCD[cfg->ch].BITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | �������� �� ���������
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  |
  ;
  DMA->TCD[cfg->ch].CITER_ELINKNO = 0
    + LSHIFT(0, 15)                                 // ELINK  | �������� �� ���������
    + LSHIFT(0, 9)                                  // LINKCH |
    + LSHIFT(0, 0)                                  // BITER  |
  ;
  DMA->TCD[cfg->ch].ATTR = 0
    + LSHIFT(0, 11) // SMOD  | ������ ������ ��������� �� ����������
    + LSHIFT(_Get_attr_size(cfg->minor_tranf_sz), 8)  // SSIZE | | ������ ���������� ��� ������������ �� ���� �������� ������ ���������
    + LSHIFT(0, 3)  // DMOD  | ������ ������ ���������
    + LSHIFT(_Get_attr_size(cfg->minor_tranf_sz), 0)  // DSIZE | | ������ ���������� ��� ������������ �� ���� �������� ������ � ��������
  ;
  DMA->TCD[cfg->ch].CSR = 0
    + LSHIFT(0, 14) // BWC         | Bandwidth Control. 00 No eDMA engine stalls
    + LSHIFT(0, 8)  // MAJORLINKCH | �������� �� ���������
    + LSHIFT(0, 7)  // DONE        | This flag indicates the eDMA has completed the major loop.
    + LSHIFT(0, 6)  // ACTIVE      | This flag signals the channel is currently in execution
    + LSHIFT(0, 5)  // MAJORELINK  | �������� �� ���������
    + LSHIFT(0, 4)  // ESG         | ������� ������������ �� ���������
    + LSHIFT(1, 3)  // DREQ        | Disable Request. If this flag is set, the eDMA hardware automatically clears the corresponding ERQ bit when the current major iteration count reaches zero.
    + LSHIFT(0, 2)  // INTHALF     | Enable an interrupt when major counter is half complete
    + LSHIFT(1, 1)  // INTMAJOR    | ���������� ���������� �� �������� ��������� DMA
    + LSHIFT(0, 0)  // START       | Channel Start. If this flag is set, the channel is requesting service.
  ;

  pDS_cbl->rx_ch  = cfg->ch;
  {
    uint8_t indx = cfg->ch;
    cfg->DMAMUX->CHCFG[indx] = cfg->dmux_src + BIT(7); // ����� ������������� ��������� ������ �� ������� ��������� (����� �� ������ SPI) � ������ ���������� ������ DMA
  }

}

/*-------------------------------------------------------------------------------------------------------------
  �������������� DMA ��� �������� ������ �� SPI

  ������ ���������� 1-�, 2-� ��� 4-� ������� �������
  sz - ���������� ������������ ���� (��� �� ������ ������ � ������!)

  ����� ���� �������� �� ����� 2048 ���� (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
static void _Start_DMA_for_SPI_TX(T_DMA_SPI_cbl *pDS_cbl, void *buf, uint32_t sz)
{
  DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

  T_DMA_TCD  volatile *pTDCmf;
  T_DMA_TCD  volatile *pTDCfm;

  // ���������� ����������� DMA ���������������� ��� ������� ������ �� SPI

  // ���������� ��� DONE
  DMA->CDNE = BIT(pDS_cbl->rx_ch);

  // ������������� ���������� ������� ������
  pTDCfm = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->rx_ch];
  pTDCfm->DADDR         = (uint32_t)&dummy_rx;
  pTDCfm->DOFF          = 0;
  pTDCfm->BITER_ELINKNO = (pTDCfm->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCfm->CITER_ELINKNO = pTDCfm->BITER_ELINKNO;

  // ���������� ����������� DMA ��������������� ��� ������ ������ � SPI

  // ���������� ��� DONE
  DMA->CDNE = BIT(pDS_cbl->tx_ch);

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->tx_ch];
  pTDCmf->SADDR         = (uint32_t)buf;
  pTDCmf->SOFF          = pDS_cbl->minor_tranf_sz;
  pTDCmf->BITER_ELINKNO = (pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;

  DMA->SERQ = pDS_cbl->rx_ch;
  DMA->SERQ = pDS_cbl->tx_ch;
}

/*-------------------------------------------------------------------------------------------------------------
  �������������� DMA ��� ������ ������ �� SPI
  ������ ���������� 1-�, 2-� ��� 4-� ������� �������
  sz - ���������� ������������ ���� (��� �� ������ ������ � ������!)

  ����� ���� �������� �� ����� 2048 ���� (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
static void _Start_DMA_for_SPI_RX(T_DMA_SPI_cbl *pDS_cbl, void *buf, uint32_t sz)
{
  DMA_MemMapPtr        DMA     = DMA_BASE_PTR;
  T_DMA_TCD  volatile *pTDCmf;
  T_DMA_TCD  volatile *pTDCfm;

  // ���������� ����������� DMA ���������������� ��� ������ �� SPI

  // ���������� ��� DONE
  DMA->CDNE = BIT(pDS_cbl->rx_ch);

  // ������������� ����������� � ����������� ������ ������� � �������
  pTDCfm = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->rx_ch];
  pTDCfm->DADDR         = (uint32_t)buf;
  pTDCfm->DOFF          = pDS_cbl->minor_tranf_sz;
  pTDCfm->BITER_ELINKNO = (pTDCfm->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCfm->CITER_ELINKNO = pTDCfm->BITER_ELINKNO;


  // ���������� ����������� DMA ��������������� ��� ������ ������ � SPI

  // ���������� ��� DONE
  DMA->CDNE = BIT(pDS_cbl->tx_ch);

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->tx_ch];
  pTDCmf->SADDR         = (uint32_t)&dummy_tx;
  pTDCmf->SOFF          = 0;
  pTDCmf->BITER_ELINKNO = (pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;


  DMA->SERQ = pDS_cbl->rx_ch;
  DMA->SERQ = pDS_cbl->tx_ch;
}

/*-------------------------------------------------------------------------------------------------------------
  �������������� DMA ��� �������� � ������ ������ �� SPI
  ������ ���������� 1-�, 2-� ��� 4-� ������� �������
  sz - ���������� ������������ ���� (��� �� ������ ������ � ������!)

  ����� ���� �������� �� ����� 2048 ���� (MAX_DMA_SPI_BUFF) !!!
-------------------------------------------------------------------------------------------------------------*/
void _Start_DMA_for_SPI_RXTX(T_DMA_SPI_cbl *pDS_cbl, void *outbuf, void *inbuf, uint32_t sz)
{
  DMA_MemMapPtr        DMA     = DMA_BASE_PTR;
  T_DMA_TCD  volatile *pTDCmf;
  T_DMA_TCD  volatile *pTDCfm;

  // ���������� ����������� DMA ���������������� ��� ������ �� SPI

  // ���������� ��� DONE
  DMA->CDNE = BIT(pDS_cbl->rx_ch);

  // ������������� ����������  ������
  pTDCfm = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->rx_ch];
  pTDCfm->DADDR         = (uint32_t)inbuf;
  pTDCfm->DOFF          = pDS_cbl->minor_tranf_sz;
  pTDCfm->BITER_ELINKNO = (pTDCfm->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCfm->CITER_ELINKNO = pTDCfm->BITER_ELINKNO;

  // ���������� ����������� DMA ��������������� ��� ������ ������ � SPI

  // ���������� ��� DONE
  DMA->CDNE = BIT(pDS_cbl->tx_ch);

  pTDCmf = (T_DMA_TCD *)&DMA->TCD[pDS_cbl->tx_ch];
  pTDCmf->SADDR         = (uint32_t)outbuf;
  pTDCmf->SOFF          = pDS_cbl->minor_tranf_sz;
  pTDCmf->BITER_ELINKNO = (pTDCmf->BITER_ELINKNO & ~0x7FF) |  sz;
  pTDCmf->CITER_ELINKNO = pTDCmf->BITER_ELINKNO;

  DMA->SERQ = pDS_cbl->rx_ch;
  DMA->SERQ = pDS_cbl->tx_ch;
}

/*------------------------------------------------------------------------------
  ���������� ���������� �� ������ DMA �� ���������� ������ ������ �� SPI ���������� � MKW40

 \param user_isr_ptr
 ------------------------------------------------------------------------------*/
static void DMA_SPI_MKW40_rx_isr(void *user_isr_ptr)
{
   DMA_MemMapPtr    DMA     = DMA_BASE_PTR;

   DMA->INT = BIT(DMA_MKW40_FM_CH); // ���������� ���� ����������  ������

   // ������� FIFO ��������� � �����������
   spi_mods[MKW40_SPI].spi->MCR  |= BIT(CLR_RXF) + BIT(CLR_TXF);
   // ������� ��� ����� � SPI
   spi_mods[MKW40_SPI].spi->SR =  spi_mods[MKW40_SPI].spi->SR;

   _lwevent_set(&spi_cbl[MKW40_SPI].spi_event, MKW40_RXEND); // ���������� �� ��������� ��������� �� DMA �� SPI � ������
}

/*------------------------------------------------------------------------------
  ���������������� 2-� ������� DMA �� ����� � �� �������� ��� ������ � ������� SPI


 ------------------------------------------------------------------------------*/
void Init_MKW40_SPI_DMA(void)
{
   T_DMA_SPI_RX_config   rx_cfg;
   T_DMA_SPI_TX_config   tx_cfg;

   tx_cfg.ch       = DMA_MKW40_MF_CH;                           // ����� ������ DMA
   tx_cfg.spi_pushr = (uint32_t)&(spi_mods[MKW40_SPI].spi->PUSHR); // ����� �������� PUSHR SPI
   tx_cfg.DMAMUX   = DMA_MKW40_DMUX_PTR;                        // ��������� �� ������������� ������� �������� ��� DMA
   tx_cfg.dmux_src = DMA_MKW40_DMUX_TX_SRC;                     // ����� ����� ��������� ��� ���������� �������������� DMAMUX ��� �������� �� DMA.
   tx_cfg.minor_tranf_sz = DMA_1BYTE_MINOR_TRANSFER;
   Config_DMA_for_SPI_TX(&tx_cfg, &DS_cbl);


   rx_cfg.ch        = DMA_MKW40_FM_CH;                          // ����� ������ DMA
   rx_cfg.spi_popr  = (uint32_t)&(spi_mods[MKW40_SPI].spi->POPR); // ����� �������� POPR SPI
   rx_cfg.DMAMUX    = DMA_MKW40_DMUX_PTR;                       // ����� ����� ��������� ��� ���������� �������������� DMAMUX ��� �������� �� DMA.
   rx_cfg.dmux_src  = DMA_MKW40_DMUX_RX_SRC;
   rx_cfg.minor_tranf_sz = DMA_1BYTE_MINOR_TRANSFER;
   Config_DMA_for_SPI_RX(&rx_cfg, &DS_cbl);
   Install_and_enable_isr(DMA_MKW40_RX_INT_NUM, spi_mods[MKW40_SPI].prio, DMA_SPI_MKW40_rx_isr); // ���������� �� ���������� ������ �� DMA
}



/*-------------------------------------------------------------------------------------------------------------
  �������� ������ �� SPI � �������������� DMA
-------------------------------------------------------------------------------------------------------------*/
_mqx_uint MKW40_SPI_send_buf(const uint8_t *buff, uint32_t sz)
{
   _mqx_uint    res = MQX_OK;
   uint32_t       s;
   int          i;

   Set_MKW40_CS_state(0);
   while (sz > 0)
   {
      if (sz >= MAX_DMA_SPI_BUFF) s = MAX_DMA_SPI_BUFF-1;
      else s = sz;

      _Start_DMA_for_SPI_TX(&DS_cbl, (void*)buff, s);
      // ������� ����� ��������� �������� ������ �� DMA
      if (_lwevent_wait_ticks(&spi_cbl[MKW40_SPI].spi_event, MKW40_RXEND, FALSE, 10) != MQX_OK)
      {
         spi_cbl[MKW40_SPI].tx_err_cnt++;
         res = MQX_ERROR;
      }
      buff = buff + s;
      sz -= s;
   }
   Set_MKW40_CS_state(1);
   return res;
}

/*-------------------------------------------------------------------------------------------------------------
  ����� ������ �� SPI � �������������� DMA
-------------------------------------------------------------------------------------------------------------*/
_mqx_uint MKW40_SPI_read_buf(const uint8_t *buff, uint32_t sz)
{
   _mqx_uint   res = MQX_OK;
   uint32_t      s;
   int         i;

   Set_MKW40_CS_state(0);
   while (sz > 0)
   {
      if (sz >= MAX_DMA_SPI_BUFF) s = MAX_DMA_SPI_BUFF-1;
      else s = sz;

      _Start_DMA_for_SPI_RX(&DS_cbl,  (void*)buff, s);
      // ������� ����� ��������� �������� �������
      if (_lwevent_wait_ticks(&spi_cbl[MKW40_SPI].spi_event, MKW40_RXEND, FALSE, 10) != MQX_OK)
      {
         spi_cbl[MKW40_SPI].rx_err_cnt++;
         res = MQX_ERROR;
      }
      buff = buff + s;
      sz -= s;
   }
   Set_MKW40_CS_state(1);
   return res;
}

/*-------------------------------------------------------------------------------------------------------------
  ����� ������ �� SPI � �������������� DMA
-------------------------------------------------------------------------------------------------------------*/
_mqx_uint MKW40_SPI_write_read_buf(const uint8_t *wbuff, uint32_t wsz, const uint8_t *rbuff, uint32_t rsz)
{
   _mqx_uint   res = MQX_OK;
   uint32_t      s;
   int         i;

   Set_MKW40_CS_state(0);
   while (wsz > 0)
   {
      if (wsz > MAX_DMA_SPI_BUFF) s = MAX_DMA_SPI_BUFF;
      else s = wsz;

      _Start_DMA_for_SPI_TX(&DS_cbl,  (void*)wbuff, s);
      // ������� ����� ��������� �������� ������ �� DMA
      if (_lwevent_wait_ticks(&spi_cbl[MKW40_SPI].spi_event, MKW40_RXEND, FALSE, 10) != MQX_OK)
      {
         spi_cbl[MKW40_SPI].tx_err_cnt++;
         res = MQX_ERROR;
      }
      wbuff = wbuff + s;
      wsz -= s;
   }

   while (rsz > 0)
   {
      if (rsz > MAX_DMA_SPI_BUFF) s = MAX_DMA_SPI_BUFF;
      else s = rsz;

      _Start_DMA_for_SPI_RX(&DS_cbl,  (void*)rbuff, s);
      // ������� ����� ��������� �������� �������
      if (_lwevent_wait_ticks(&spi_cbl[MKW40_SPI].spi_event, MKW40_RXEND, FALSE, 10) != MQX_OK)
      {
         spi_cbl[MKW40_SPI].rx_err_cnt++;
         res = MQX_ERROR;
      }
      rbuff = rbuff + s;
      rsz -= s;
   }
   Set_MKW40_CS_state(1);
   return res;
}


/*-------------------------------------------------------------------------------------------------------------
  ����� ������ �� SPI � �������������� DMA
  ������� ���������� ������� �����
  ������ ������ ����� ���� �� ����� MAX_DMA_SPI_BUFF
-------------------------------------------------------------------------------------------------------------*/
_mqx_uint MKW40_SPI_slave_read_buf(const uint8_t *buff, uint32_t sz)
{
  _mqx_uint   res = MQX_OK;
  if (sz >= MAX_DMA_SPI_BUFF) return MQX_ERROR;

  _Start_DMA_for_SPI_RX(&DS_cbl, (void*)buff, sz);

  // ������� ����� ��������� �������� �������
  if (_lwevent_wait_ticks(&spi_cbl[MKW40_SPI].spi_event, MKW40_RXEND, FALSE, 10) != MQX_OK)
  {
    spi_cbl[MKW40_SPI].rx_err_cnt++;
    res = MQX_ERROR;
  }
  return res;
}


/*-------------------------------------------------------------------------------------------------------------
  ����� � �������� ������ �� SPI � �������������� DMA
  ������ ������ ����� ���� �� ����� MAX_DMA_SPI_BUFF
-------------------------------------------------------------------------------------------------------------*/
_mqx_uint MKW40_SPI_slave_read_write_buf(const uint8_t *outbuff, uint8_t *inbuff, uint32_t sz)
{
  _mqx_uint   res = MQX_OK;
  if (sz >= MAX_DMA_SPI_BUFF) return MQX_ERROR;

  SPI_clear_FIFO(MKW40_SPI);
  _Start_DMA_for_SPI_RXTX(&DS_cbl, (void*)outbuff, inbuff, sz);

  // ������� ����� ��������� �������� �������
  if (_lwevent_wait_ticks(&spi_cbl[MKW40_SPI].spi_event, MKW40_RXEND, FALSE, 10) != MQX_OK)
  {
    spi_cbl[MKW40_SPI].rx_err_cnt++;
    res = MQX_ERROR;
  }
  return res;
}



/*------------------------------------------------------------------------------
  ������������� SPI ������ � 2-� ������� DMA ��� ����

 ------------------------------------------------------------------------------*/
void Init_MKW40_channel(void)
{
  //SPI_master_init(MKW40_SPI, SPI_8_BITS, 0, 0, SPI_BAUD_20MHZ, 0);
  SPI_slave_init(MKW40_SPI, SPI_8_BITS, 0, 0, 0);
  Init_MKW40_SPI_DMA(); // ���������������� 2-� ������� DMA �� ����� � �� �������� ��� ������ � ������� SPI

}

