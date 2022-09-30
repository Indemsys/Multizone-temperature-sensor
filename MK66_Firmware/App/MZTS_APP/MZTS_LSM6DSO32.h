#ifndef SB200M2PC_LSM6DSO32_H
  #define SB200M2PC_LSM6DSO32_H
                                                //       default state
#define LSM_FUNC_CFG_ACCESS              0x01   //  RW   00000000
#define LSM_PIN_CTRL                     0x02   //  RW   00111111
#define LSM_FIFO_CTRL1                   0x07   //  RW   00000000
#define LSM_FIFO_CTRL2                   0x08   //  RW   00000000
#define LSM_FIFO_CTRL3                   0x09   //  RW   00000000
#define LSM_FIFO_CTRL4                   0x0A   //  RW   00000000
#define LSM_COUNTER_BDR_REG1             0x0B   //  RW   00000000
#define LSM_COUNTER_BDR_REG2             0x0C   //  RW   00000000
#define LSM_INT1_CTRL                    0x0D   //  RW   00000000
#define LSM_INT2_CTRL                    0x0E   //  RW   00000000
#define LSM_WHO_AM_I                     0x0F   //   R   01101100
#define LSM_CTRL1_XL                     0x10   //  RW   00000000
#define LSM_CTRL2_G                      0x11   //  RW   00000000
#define LSM_CTRL3_C                      0x12   //  RW   00000100
#define LSM_CTRL4_C                      0x13   //  RW   00000000
#define LSM_CTRL5_C                      0x14   //  RW   00000000
#define LSM_CTRL6_C                      0x15   //  RW   00000000
#define LSM_CTRL7_G                      0x16   //  RW   00000000
#define LSM_CTRL8_XL                     0x17   //  RW  1 00000000
#define LSM_CTRL9_XL                     0x18   //  RW   11100000
#define LSM_CTRL10_C                     0x19   //  RW   00000000
#define LSM_ALL_INT_SRC                  0x1A   //   R   output
#define LSM_WAKE_UP_SRC                  0x1B   //   R   output
#define LSM_TAP_SRC                      0x1C   //   R   output
#define LSM_D6D_SRC                      0x1D   //   R   output
#define LSM_STATUS_REG                   0x1E   //   R   output
#define LSM_OUT_TEMP_L                   0x20   //   R   output
#define LSM_OUT_TEMP_H                   0x21   //   R   output
#define LSM_OUTX_L_G                     0x22   //   R   output
#define LSM_OUTX_H_G                     0x23   //   R   output
#define LSM_OUTY_L_G                     0x24   //   R   output
#define LSM_OUTY_H_G                     0x25   //   R   output
#define LSM_OUTZ_L_G                     0x26   //   R   output
#define LSM_OUTZ_H_G                     0x27   //   R   output
#define LSM_OUTX_L_A                     0x28   //   R   output
#define LSM_OUTX_H_A                     0x29   //   R   output
#define LSM_OUTY_L_A                     0x2A   //   R   output
#define LSM_OUTY_H_A                     0x2B   //   R   output
#define LSM_OUTZ_L_A                     0x2C   //   R   output
#define LSM_OUTZ_H_A                     0x2D   //   R   output
#define LSM_EMB_FUNC_STATUS_MAINPAGE     0x35   //   R   output
#define LSM_FSM_STATUS_A_MAINPAGE        0x36   //   R   output
#define LSM_FSM_STATUS_B_MAINPAGE        0x37   //   R   output
#define LSM_STATUS_MASTER_MAINPAGE       0x39   //   R   output
#define LSM_FIFO_STATUS1                 0x3A   //   R   output
#define LSM_FIFO_STATUS2                 0x3B   //   R   output
#define LSM_TIMESTAMP0                   0x40   //   R   output
#define LSM_TIMESTAMP1                   0x41   //   R   output
#define LSM_TIMESTAMP2                   0x42   //   R   output
#define LSM_TIMESTAMP3                   0x43   //   R   output
#define LSM_TAP_CFG0                     0x56   //  RW   00000000
#define LSM_TAP_CFG1                     0x57   //  RW   00000000
#define LSM_TAP_CFG2                     0x58   //  RW   00000000
#define LSM_TAP_THS_6D                   0x59   //  RW   00000000
#define LSM_NT_DUR2                      0x5A   //  RW   00000000
#define LSM_WAKE_UP_THS                  0x5B   //  RW   00000000
#define LSM_WAKE_UP_DUR                  0x5C   //  RW   00000000
#define LSM_FREE_FALL                    0x5D   //  RW   00000000
#define LSM_MD1_CFG                      0x5E   //  RW   00000000
#define LSM_MD2_CFG                      0x5F   //  RW   00000000
#define LSM_I3C_BUS_AVB                  0x62   //  RW   00000000
#define LSM_INTERNAL_FREQ_FINE           0x63   //   R   output
#define LSM_X_OFS_USR                    0x73   //  RW   00000000
#define LSM_Y_OFS_USR                    0x74   //  RW   00000000
#define LSM_Z_OFS_USR                    0x75   //  RW   00000000
#define LSM_FIFO_DATA_OUT_TAG            0x78   //   R   output
#define LSM_FIFO_DATA_OUT_X_L            0x79   //   R   output
#define LSM_FIFO_DATA_OUT_X_H            0x7A   //   R   output
#define LSM_FIFO_DATA_OUT_Y_L            0x7B   //   R   output
#define LSM_FIFO_DATA_OUT_Y_H            0x7C   //   R   output
#define LSM_FIFO_DATA_OUT_Z_L            0x7D   //   R   output
#define LSM_FIFO_DATA_OUT_Z_H            0x7E   //   R   output

typedef struct
{
  int16_t temp;
  int16_t x_g;
  int16_t y_g;
  int16_t z_g;
  int16_t x_a;
  int16_t y_a;
  int16_t z_a;
  float   temperature;
  int32_t x_a_ndc;       // Текущее значение амплитуды сигнала по оси X акселерометра после удаления постоянной составляющей
  int32_t y_a_ndc;       // Текущее значение амплитуды сигнала по оси Y акселерометра после удаления постоянной составляющей
  int32_t z_a_ndc;       // Текущее значение амплитуды сигнала по оси Z акселерометра после удаления постоянной составляющей
  int32_t max_x_a_ndc;   // Максимальное значение абсолютное значение амплитуды сигнала по оси X акселерометра после удаления постоянной составляющей после последнего сброса этой величины
  int32_t max_y_a_ndc;   // Максимальное значение абсолютное значение амплитуды сигнала по оси Y акселерометра после удаления постоянной составляющей после последнего сброса этой величины
  int32_t max_z_a_ndc;   // Максимальное значение абсолютное значение амплитуды сигнала по оси Z акселерометра после удаления постоянной составляющей после последнего сброса этой величины
  int32_t aver_accel_rms;
  int32_t max_aver_accel_rms;
} T_lsm6dso32_data;


extern T_lsm6dso32_data   lsm6dso32_data;

void      LSM6DSO32_init(void);
uint32_t  LSM6DSO32_wr_rd_buf(uint8_t *wbuff, uint8_t *rbuff, uint32_t sz);
uint32_t  LSM6DSO32_read_reg(uint8_t addr, uint8_t *b);
uint32_t  LSM6DSO32_write_reg(uint8_t addr, uint8_t b);
void      LSM6DSO32_processing(void);
void      LSM6DSO32_reset_max(void);

#endif // SB200M2PC_LSM6DSO32_H


