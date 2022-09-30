// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2022-04-07
// 13:05:50
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"


uint8_t               LSM_id;
uint8_t               lsm6dso32_active;
T_lsm6dso32_data      lsm6dso32_data;
uint8_t               lsm6dso32_buf[16];
T_dc_remover_filter   dc_remove_x;
T_dc_remover_filter   dc_remove_y;
T_dc_remover_filter   dc_remove_z;

#define ACCEL_AVER_FLTR_LEN 100
T_run_average_int32_N       accel_rms_aver_flt;
int32_t                     accel_flt_arr[ACCEL_AVER_FLTR_LEN];

static   TIME_STRUCT  lsm6dso32_init_time;
static   uint8_t      lsm6dso32_dc_filter_stable;


/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void LSM6DSO32_init(void)
{
  uint8_t b;
  LSM6DSO32_read_reg(LSM_WHO_AM_I,&LSM_id);
  if (LSM_id != 0x6C)
  {
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "LSM6DSO32 Not detected.");
    return;
  }


  // Сброс устройства
  b = 0
     + LSHIFT(0, 7) // BOOT      | Reboots memory content. Default value: 0 (0: normal mode; 1: reboot memory content) This bit is automatically cleared.
     + LSHIFT(0, 6) // BDU       | Block Data Update. Default value: 0 (0: continuous update; 1: output registers are not updated until MSB and LSB have been read)
     + LSHIFT(0, 5) // H_LACTIVE | Interrupt activation level. Default value: 0 (0: interrupt output pins active high; 1: interrupt output pins active low)
     + LSHIFT(0, 4) // PP_OD     | Push-pull/open-drain selection on INT1 and INT2 pins. Default value: 0 (0: push-pull mode; 1: open-drain mode)
     + LSHIFT(0, 3) // SIM       | SPI Serial Interface Mode selection. Default value: 0 (0: 4-wire interface; 1: 3-wire interface)
     + LSHIFT(1, 2) // IF_INC    | Register address automatically incremented during a multiple byte access with a serial interface (I²C or SPI). Default value: 1 (0: disabled; 1: enabled)
     + LSHIFT(0, 1) //           |
     + LSHIFT(1, 0) // SW_RESET  | Software reset. Default value: 0 (0: normal mode; 1: reset device) This bit is automatically cleared.
  ;
  LSM6DSO32_write_reg(LSM_CTRL3_C, b);
  b &= ~BIT(0);
  LSM6DSO32_write_reg(LSM_CTRL3_C, b);


  // Запускаем работу акселерометра 104 Гц
  b = 0
     + LSHIFT(4, 4) // ODR_XL[3:0] Accelerometer ODR selection
                    //  0000   Power-down Power-down
                    //  1011   1.6 Hz (low power only)     12.5 Hz (high performance)
                    //  0001   12.5 Hz (low power)         12.5 Hz (high performance)
                    //  0010   26 Hz (low power)           26 Hz (high performance)
                    //  0011   52 Hz (low power)           52 Hz (high performance)
                    //  0100   104 Hz (normal mode)        104 Hz (high performance)
                    //  0101   208 Hz (normal mode)        208 Hz (high performance)
                    //  0110   416 Hz (high performance)   416 Hz (high performance)
                    //  0111   833 Hz (high performance)   833 Hz (high performance)
                    //  1000   1.66 kHz (high performance) 1.66 kHz (high performance)
                    //  1001   3.33 kHz (high performance) 3.33 kHz (high performance)
                    //  1010   6.66 kHz (high performance) 6.66 kHz (high performance)
                    //  11xx   Not allowed Not allowed
     + LSHIFT(0, 2) // FS[1:0]_XL Accelerometer full-scale selection
                    //  00 ±4 g
                    //  01 ±32 g
                    //  10 ±8  g
                    //  11 ±16 g
     + LSHIFT(0, 1) // LPF2_XL_EN Accelerometer high-resolution selection (0: output from first stage digital filtering selected (default); 1: output from LPF2 second filtering stage selected)
     + LSHIFT(0, 0) //
  ;

  LSM6DSO32_write_reg(LSM_CTRL1_XL, b);

  // Запускаем работу гироскопа 104 Гц
  b = 0
     + LSHIFT(4, 4) // ODR_G[3:0] Gyroscope output data rate selection
                    // 0000 Power down Power down
                    // 0001 12.5 Hz (low power)         12.5 Hz (high performance)
                    // 0010 26 Hz (low power)           26 Hz (high performance)
                    // 0011 52 Hz (low power)           52 Hz (high performance)
                    // 0100 104 Hz (normal mode)        104 Hz (high performance)
                    // 0101 208 Hz (normal mode)        208 Hz (high performance)
                    // 0110 416 Hz (high performance)   416 Hz (high performance)
                    // 0111 833 Hz (high performance)   833 Hz (high performance)
                    // 1000 1.66 kHz (high performance) 1.66 kHz (high performance)
                    // 1001 3.33 kHz (high performance  3.33 kHz (high performance)
                    // 1010 6.66 kHz (high performance  6.66 kHz (high performance)
                    // 1011 Not available Not available
     + LSHIFT(0, 2) // FS[1:0]_G  Gyroscope chain full-scale selection
                    //  00 ±250 dps
                    //  01  ±500 dps
                    //  10 ±1000 dps
                    //  11 ±2000 dps
     + LSHIFT(0, 1) // FS_125 Selects gyro chain full-scale ±125 dps (0: FS selected through bits FS[1:0]_G; 1: FS set to ±125 dps)
     + LSHIFT(0, 0) //
  ;
  LSM6DSO32_write_reg(LSM_CTRL2_G, b);


  accel_rms_aver_flt.len = ACCEL_AVER_FLTR_LEN;
  accel_rms_aver_flt.en  = 0;
  accel_rms_aver_flt.arr = accel_flt_arr;
  lsm6dso32_active = 1;


  _time_get(&lsm6dso32_init_time);
  LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "LSM6DSO32 initialized.");

}

/*-----------------------------------------------------------------------------------------------------
  Считывание данных из чипа LSM6DSO32

  \param void
-----------------------------------------------------------------------------------------------------*/
void LSM6DSO32_processing(void)
{
  uint32_t res;
  // Читаем регистры с данными
  // LSM_OUT_TEMP_L
  // LSM_OUT_TEMP_H
  // LSM_OUTX_L_G
  // LSM_OUTX_H_G
  // LSM_OUTY_L_G
  // LSM_OUTY_H_G
  // LSM_OUTZ_L_G
  // LSM_OUTZ_H_G
  // LSM_OUTX_L_A
  // LSM_OUTX_H_A
  // LSM_OUTY_L_A
  // LSM_OUTY_H_A
  // LSM_OUTZ_L_A
  // LSM_OUTZ_H_A
  if (lsm6dso32_active == 0) return;

  memset(lsm6dso32_buf, 0xFF, 15);
  lsm6dso32_buf[0] = LSM_OUT_TEMP_L | 0x80;
  res = LSM6DSO32_wr_rd_buf(lsm6dso32_buf,lsm6dso32_buf, 15);
  if (res == MQX_OK)
  {
    memcpy(&lsm6dso32_data,&lsm6dso32_buf[1], 14);
  }
  lsm6dso32_data.temperature =(lsm6dso32_data.temp / 256.0f)+ 25.0f;

  lsm6dso32_data.x_a_ndc = DCremover_filter(lsm6dso32_data.x_a,&dc_remove_x);
  lsm6dso32_data.y_a_ndc = DCremover_filter(lsm6dso32_data.y_a,&dc_remove_y);
  lsm6dso32_data.z_a_ndc = DCremover_filter(lsm6dso32_data.z_a,&dc_remove_z);

  int32_t xp2 = abs(lsm6dso32_data.x_a_ndc);
  int32_t yp2 = abs(lsm6dso32_data.y_a_ndc);
  int32_t zp2 = abs(lsm6dso32_data.z_a_ndc);

  if (xp2 > lsm6dso32_data.max_x_a_ndc) lsm6dso32_data.max_x_a_ndc = xp2;
  if (yp2 > lsm6dso32_data.max_y_a_ndc) lsm6dso32_data.max_y_a_ndc = yp2;
  if (zp2 > lsm6dso32_data.max_z_a_ndc) lsm6dso32_data.max_z_a_ndc = zp2;

  float val = xp2*xp2 + yp2*yp2 + zp2*zp2;

  int32_t ival = (int32_t)sqrtf(val);

  lsm6dso32_data.aver_accel_rms =  RunAverageFilter_int32_N(ival,&accel_rms_aver_flt);

  if (lsm6dso32_data.aver_accel_rms > lsm6dso32_data.max_aver_accel_rms)
  {
    lsm6dso32_data.max_aver_accel_rms = lsm6dso32_data.aver_accel_rms;
  }

  if (lsm6dso32_dc_filter_stable == 0)
  {
    if (Time_elapsed_ms(&lsm6dso32_init_time) > 2000)
    {
      lsm6dso32_dc_filter_stable = 1;
      LSM6DSO32_reset_max();
    }
  }
}

/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
void LSM6DSO32_reset_max(void)
{
  if (lsm6dso32_active == 0) return;

  LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Vibration metrics before reset: Max.RMS=%d Max.X=%d Max.Y=%d Max.Z=%d",lsm6dso32_data.max_aver_accel_rms, lsm6dso32_data.max_x_a_ndc, lsm6dso32_data.max_y_a_ndc, lsm6dso32_data.max_z_a_ndc);
  lsm6dso32_data.max_x_a_ndc = 0;
  lsm6dso32_data.max_y_a_ndc = 0;
  lsm6dso32_data.max_z_a_ndc = 0;
  lsm6dso32_data.max_aver_accel_rms= 0;
}


/*-----------------------------------------------------------------------------------------------------
  Прочитать регистр

  \param addr
  \param b

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t LSM6DSO32_read_reg(uint8_t addr, uint8_t *b)
{
  uint32_t res;
  uint8_t buf[2];
  buf[0] = addr | 0x80; // Добавляем флаг чтения
  buf[1] = 0;
  res = LSM6DSO32_wr_rd_buf(buf, buf, 2);

  if (res == MQX_OK) if (b) *b = buf[1];
  return res;
}


/*-----------------------------------------------------------------------------------------------------


  \param addr
  \param b

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t LSM6DSO32_write_reg(uint8_t addr, uint8_t b)
{
  uint32_t res;
  uint8_t buf[2];
  buf[0] = addr & ~0x80; // Ставим флаг чтения
  buf[1] = b;
  res = LSM6DSO32_wr_rd_buf(buf, buf, 2);

  return res;
}

/*-------------------------------------------------------------------------------------------------------------
  Передача и прием  данных по SPI с использованием DMA в контроллер дисплея LSM6DSO32

  sz    - количество посылаемых слов
  wbuff - буфер с передаваемыми словами

  размер слова определяется при инициализации DMA
-------------------------------------------------------------------------------------------------------------*/
uint32_t LSM6DSO32_wr_rd_buf(uint8_t *wbuff, uint8_t *rbuff, uint32_t sz)
{
  _mqx_uint res = MQX_OK;
  int32_t       i;
  uint32_t      n;


  while (sz > 0)
  {
    if (sz > MAX_DMA_SPI_BUFF)
    {
      n = MAX_DMA_SPI_BUFF;
    }
    else
    {
      n = sz;
    }
    SPI0_bus_lock_for_LSM6DSO32();
    Set_LSM_CS(0);
    Start_DMA_for_SPI_RXTX(&SPI0_DS_cbl, wbuff, rbuff, n);
    // Ожидаем флага окончания передачи буфера по DMA
    if (_lwevent_wait_ticks(&spi_cbl[BUS_SPI0].spi_event, SPI0_BUS_RXEND, FALSE, 10) != MQX_OK)
    {
      spi_cbl[BUS_SPI0].tx_err_cnt++;
      res = MQX_ERROR;
      Set_LSM_CS(1);
      SPI0_bus_unlock();
      break;
    }
    Set_LSM_CS(1);
    SPI0_bus_unlock();
    sz = sz - n;
    wbuff += n;
  }

  return res;
}

