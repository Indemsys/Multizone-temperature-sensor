#ifndef _MZTS_TFT_DISPLAY_CONTROL_H
  #define _MZTS_TFT_DISPLAY_CONTROL_H

#define HMI_SCREEN_NUM  2

void     TFT_init(void);

void     TFT_Set_coordinates(uint32_t x, uint32_t y);
void     TFT_Set_rect(int x0, int y0, int x1, int y1);
void     TFT_Set_x(uint32_t x);
void     TFT_Set_y(uint32_t y);

void     TFT_wr_cmd(uint16_t data);
void     TFT_wr_data(uint16_t data);
uint32_t TFT_wr_data_buf(uint8_t *buf, uint32_t buf_sz);
uint32_t TFT_wr_cmd_buf(uint8_t *buf, uint32_t buf_sz);
uint32_t TFT_fill_by_pixel(uint16_t *w, uint32_t data_sz);

void     GUI_task_create(void);

#endif // MZTS_LCD_H



