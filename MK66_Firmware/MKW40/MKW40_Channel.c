// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.07.29
// 15:08:39
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

#define MKW_BUF_SZ        22 // ����������� ����� ������
#define MKW_DATA_SZ       20 // ���������� �������� ������ � ������
static uint8_t            mkw_rx_buf[MKW_BUF_SZ]; // ����� ������ ������ �� BLE ����
static uint8_t            mkw_tx_buf[MKW_BUF_SZ]; // ����� �������� ������ BLE ����
static MUTEX_STRUCT       mutex;

static uint8_t            mkw40_activated;  // ���� ����������������� ���� MKW40

static T_MKW40_recv_subsc MKW40_subsribers[MKW_SUBSCR_NAX_CNT];  // ������ � ���������� �� ����� �� ����� ����� MKW40
static int32_t MKW40_setup_params(void);

/*------------------------------------------------------------------------------
  ������ ��������� ������ ����� � ����� ������������ ����� MKW40 �������������� �������� Bluetooth LE (BLE)

 \param parameter
 ------------------------------------------------------------------------------*/
void Task_MKW40(uint32_t parameter)
{
  if (Create_mutex_P_inhr_P_queue(&mutex) != MQX_OK) return;

  _mutex_lock(&mutex);

  mkw40_activated = 0;

  Init_MKW40_channel();

  // ������ � ���� ������������� ���������� � ������� BLE �� ���� MKW40
  do
  {
    if (MKW40_setup_params() == RES_OK) break;
    OS_Time_delay(10); // ���� ��������� ���������� ���� �� ������� ���������������� ���������
  }
  while (1);

  LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "BLE device activated.");
  mkw40_activated = 1;

  memset(mkw_tx_buf, 0, MKW_BUF_SZ); // ������� ����� ������������ ������
  for (;;)
  {
    // ��������� � ���������� ������ ���� MKW40 � ������ ������
    // ������ MKW40 �������� ������ �������� MKW_BUF_SZ ������ 2 ��
    // ��������� ������:
    // ����� �����   ����������
    // � ������
    // 0     [llid]  - ������������� ����������� ������
    // 1     [LEN]   - ����������� ������������ ������
    // 2     [data0] - ������ ���� ������
    // ...
    // LEN+2 [dataN] - ��������� ���� ������



    if (MKW40_SPI_slave_read_write_buf(mkw_tx_buf, mkw_rx_buf, MKW_BUF_SZ) == MQX_OK)
    {
      uint8_t  llid = mkw_rx_buf[0] - 1;
      // ���������� ���������� ������ � ������� � �������������� ���

      if (llid < MKW_SUBSCR_NAX_CNT)
      {
        if (MKW40_subsribers[llid].receiv_func != 0)
        {
          MKW40_subsribers[llid].receiv_func(&mkw_rx_buf[2], mkw_rx_buf[1], MKW40_subsribers[llid].pcbl);
        }
      }

      // �������� ����� ������ ��� ��������
      memset(mkw_tx_buf, 0, MKW_BUF_SZ); // �������������� �������� ����� ����� �� ���� ������� ������ ������
      _mutex_unlock(&mutex);
      // ����� ������ ��������� �������� �������� ������ � ���������� ������ mkw_tx_buf
      _mutex_lock(&mutex);
    }

  }
}

/*-----------------------------------------------------------------------------------------------------
  ��������� ���������� ����� ������������ ����� BLE � ���� MKW40

 \param void
-----------------------------------------------------------------------------------------------------*/
static int32_t MKW40_setup_params(void)
{
  // ���������� ���
  mkw_tx_buf[0] = MKW40_CH_RESET;
  if (MKW40_SPI_slave_read_write_buf(mkw_tx_buf, mkw_rx_buf, MKW_BUF_SZ) != MQX_OK) return RES_ERROR;

  OS_Time_delay(100);
  // ������ ������� ��� ������� ������ �� ������ ����� ������
//  mkw_tx_buf[0] = MKW40_CH_NACK;
//  if (MKW40_SPI_slave_read_write_buf(mkw_tx_buf, mkw_rx_buf, MKW_BUF_SZ) != MQX_OK) return RES_ERROR;

//  mkw_tx_buf[0] = MKW40_CH_NACK;
//  if (MKW40_SPI_slave_read_write_buf(mkw_tx_buf, mkw_rx_buf, MKW_BUF_SZ) != MQX_OK) return RES_ERROR;

  // ���������� PIN ���
  mkw_tx_buf[0] = MKW40_CH_SETT_WRITE;
  mkw_tx_buf[1] = ((PAR_PIN_CODE & BLE_PARAM_MASK) << BLE_PARAM_POS) | (sizeof(wvar.pin_code) & BLE_PARAM_SZ_MASK);
  memcpy(&mkw_tx_buf[2], &wvar.pin_code, sizeof(wvar.pin_code));
  if (MKW40_SPI_slave_read_write_buf(mkw_tx_buf, mkw_rx_buf, MKW_BUF_SZ) != MQX_OK) return RES_ERROR;

  // ���������� ��� ���������� ��� �������������
  mkw_tx_buf[0] = MKW40_CH_SETT_WRITE;
  mkw_tx_buf[1] = ((PAR_ADV_DEV_NAME & BLE_PARAM_MASK) << BLE_PARAM_POS) | (sizeof(wvar.adv_dev_name) & BLE_PARAM_SZ_MASK);
  memcpy(&mkw_tx_buf[2], wvar.adv_dev_name, sizeof(wvar.adv_dev_name));
  if (MKW40_SPI_slave_read_write_buf(mkw_tx_buf, mkw_rx_buf, MKW_BUF_SZ) != MQX_OK) return RES_ERROR;

  // ���������� ������ ������������ ����������� ��� ������ � ��������������� ��������
  mkw_tx_buf[0] = MKW40_CH_SETT_WRITE;
  mkw_tx_buf[1] = ((PAR_SOFT_REV & BLE_PARAM_MASK) << BLE_PARAM_POS) | (sizeof(wvar.ble_ver) & BLE_PARAM_SZ_MASK);
  memcpy(&mkw_tx_buf[2], wvar.ble_ver, sizeof(wvar.ble_ver));
  if (MKW40_SPI_slave_read_write_buf(mkw_tx_buf, mkw_rx_buf, MKW_BUF_SZ) != MQX_OK) return RES_ERROR;

  // ��������� ������ BLE �����
  memset(mkw_tx_buf, 0, MKW_BUF_SZ); //
  mkw_tx_buf[0] = MKW40_CH_SETT_DONE;
  if (MKW40_SPI_slave_read_write_buf(mkw_tx_buf, mkw_rx_buf, MKW_BUF_SZ) != MQX_OK) return RES_ERROR;

  return RES_OK;
}


/*------------------------------------------------------------------------------
 ��������� ��������� ������ � ������� �����������

 \param llid          - ������������� ����������� ������ ��������, �� �� ������ � ������� �����������
 \param receiv_func   - ��������� �� ������� ������
 \param pcbl          - ��������� �� ��������������� ��������� ������������ ��� ������ ������� ������

 \return _mqx_uint
 ------------------------------------------------------------------------------*/
_mqx_uint MKW40_subscibe(uint8_t llid, T_MKW40_receiver receiv_func, void *pcbl)
{
  llid -= 1;
  if (llid < MKW_SUBSCR_NAX_CNT)
  {
    MKW40_subsribers[llid].receiv_func = receiv_func;
    MKW40_subsribers[llid].pcbl = pcbl;
    return MQX_OK;
  }
  return MQX_ERROR;

}
/*------------------------------------------------------------------------------
 ��������� �������� ������ � ������ ����� MKW40


 \param llid - ������������� ����������� ������ ��������
 \param data
 \param sz
 ------------------------------------------------------------------------------*/
_mqx_uint MKW40_send_buf(uint8_t llid, uint8_t *data, uint32_t sz)
{
  _mqx_uint res;
  uint32_t  len = 0;

  while (sz > 0)
  {
    res = _mutex_lock(&mutex);
    if (res != MQX_OK) return res;

    mkw_tx_buf[0] = llid;
    if (sz > MKW_DATA_SZ) len = MKW_DATA_SZ;
    else len = sz;
    mkw_tx_buf[1] = (uint8_t)len;
    memcpy(&mkw_tx_buf[2], data, len);
    sz -= len;
    data += len;
    _mutex_unlock(&mutex);
  }

  return MQX_OK;
}

/*-----------------------------------------------------------------------------------------------------

 \param void

 \return uint8_t
-----------------------------------------------------------------------------------------------------*/
uint8_t Is_BLE_activated(void)
{
  return mkw40_activated;
}
