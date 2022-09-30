// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2018.12.13
// 16:02:08
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "app.h"
#include   "GUI.h"

extern const T_SPI_modules spi_mods[3];
extern       T_SPI_cbls    spi_cbl[3];


#define                  LCD_X_SIZE 240
#define                  LCD_Y_SIZE 240
#define                  MAX_S_LEN  64

#define                  MAX_X_SZ   319
#define                  MAX_Y_SZ   239
uint32_t                 X_OFFSET;
uint32_t                 Y_OFFSET;

GUI_RECT                 screen_rect;
char                     gui_str[MAX_S_LEN+1];

static _task_id          gui_task_id;
static void              Task_GUI(uint32_t initial_data);

static T_app_snapshot    app_snapshot;

/*-------------------------------------------------------------------------------------------------------------
  Передача данных по SPI с использованием DMA в контроллер дисплея ST7789 по 4-х проводной схеме

  sz    - количество посылаемых слов
  wbuff - буфер с передаваемыми словами

  размер слова определяется при инициализации DMA
-------------------------------------------------------------------------------------------------------------*/
static uint32_t SPI_TFT_write_buf(uint8_t *wbuff, uint32_t sz)
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
    SPI0_bus_lock_for_TFT();
    Set_LCD_CS(0);
    Start_DMA_TFT_TX_w_dummyRX(&SPI0_DS_cbl, wbuff, n, 0);
    // Ожидаем флага окончания передачи буфера по DMA
    if (_lwevent_wait_ticks(&spi_cbl[BUS_SPI0].spi_event, SPI0_BUS_RXEND, FALSE, 10) != MQX_OK)
    {
      spi_cbl[BUS_SPI0].tx_err_cnt++;
      res = MQX_ERROR;
      Set_LCD_CS(1);
      SPI0_bus_unlock();
      break;
    }
    Set_LCD_CS(1);
    SPI0_bus_unlock();
    sz = sz - n;
    wbuff += n;
  }

  return res;
}

/*-------------------------------------------------------------------------------------------------------------
  Повоторяющаяся передача слова по SPI с использованием DMA в контроллер дисплея ST7789 по 4-х проводной схеме
  Используется для заполнения прямоугольных пространств на дисплее

  sz    - количество посылаемых слов
  w     - буфер с передаваемыми словами

  размер слова определяется при инициализации DMA
-------------------------------------------------------------------------------------------------------------*/
static uint32_t SPI_TFT_fill_by_word(uint16_t *w, uint32_t sz)
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
    SPI0_bus_lock_for_TFT();
    Set_LCD_CS(0);
    Start_DMA_TFT_TX_w_dummyRX(&SPI0_DS_cbl, w, n, 1);
    // Ожидаем флага окончания передачи буфера по DMA
    if (_lwevent_wait_ticks(&spi_cbl[BUS_SPI0].spi_event, SPI0_BUS_RXEND, FALSE, 10) != MQX_OK)
    {
      spi_cbl[BUS_SPI0].tx_err_cnt++;
      res = MQX_ERROR;
      Set_LCD_CS(1);
      SPI0_bus_unlock();
      break;
    }
    Set_LCD_CS(1);
    SPI0_bus_unlock();
    sz = sz - n;
  }

  return res;
}

/*-----------------------------------------------------------------------------------------------------

 \param cmdw
 \param val

 \return _mqx_uint
-----------------------------------------------------------------------------------------------------*/
_mqx_uint SPI_TFT_write_byte(uint8_t val)
{
  _mqx_uint  res;
  res = SPI_TFT_write_buf(&val, 1);
  return res;
}

/*-----------------------------------------------------------------------------------------------------


  \param v
-----------------------------------------------------------------------------------------------------*/
static void TFT_delay(uint32_t v)
{
  _time_delay_ticks(ms_to_ticks(v));
}

/*-----------------------------------------------------------------------------------------------------


  \param data
-----------------------------------------------------------------------------------------------------*/
void TFT_wr_cmd(uint16_t data)
{
  Set_LCD_DC(0);
  SPI_TFT_write_byte(data);
}

/*-----------------------------------------------------------------------------------------------------


  \param data
-----------------------------------------------------------------------------------------------------*/
void TFT_wr_data(uint16_t data)
{
  Set_LCD_DC(1);
  SPI_TFT_write_byte(data);
}

/*-----------------------------------------------------------------------------------------------------


  \param data
-----------------------------------------------------------------------------------------------------*/
uint32_t TFT_wr_data_buf(uint8_t *buf, uint32_t buf_sz)
{
  _mqx_uint  res;

  Set_LCD_DC(1);
  res = SPI_TFT_write_buf(buf, buf_sz);
  return res;
}

/*-----------------------------------------------------------------------------------------------------
  Заполнение прямоугольной области дисплея фиксированным словом


  \param data
-----------------------------------------------------------------------------------------------------*/
uint32_t TFT_fill_by_pixel(uint16_t *w, uint32_t data_sz)
{
  _mqx_uint  res;

  Set_LCD_DC(1);
  res = SPI_TFT_fill_by_word(w, data_sz);
  return res;
}

/*-----------------------------------------------------------------------------------------------------


  \param data
-----------------------------------------------------------------------------------------------------*/
uint32_t TFT_wr_cmd_buf(uint8_t *buf, uint32_t buf_sz)
{
  _mqx_uint  res;

  Set_LCD_DC(0);
  res = SPI_TFT_write_buf(buf, buf_sz);
  return res;
}

/*-----------------------------------------------------------------------------------------------------


-----------------------------------------------------------------------------------------------------*/
void TFT_init(void)
{
  uint8_t  tmp;
  uint16_t data;

  _mqx_uint res;




  Set_LCD_BLK(1);
  Set_LCD_RST(0);
  TFT_delay(10);
  Set_LCD_RST(1);

  TFT_delay(100);
  TFT_wr_cmd(0x0011); //exit SLEEP mode

  TFT_delay(100);

  TFT_wr_cmd(0x0036);

  switch (wvar.screen_rot)
  {
  case 0:
    tmp = 0
         + LSHIFT(0, 7) // MY  - Page Address Order       | 0 = Top to Bottom, 1 = Bottom to Top
         + LSHIFT(0, 6) // MX  - Column Address Order     | 0 = Left to Right, 1 = Right to Left
         + LSHIFT(0, 5) // MV  - Page/Column Order        | 0 = Normal Mode, 1 = Reverse Mode
         + LSHIFT(0, 4) // ML  - Line Address Order       | 0 = LCD Refresh Top to Bottom, 1 = LCD Refresh Bottom to Top
         + LSHIFT(0, 3) // RGB - RGB/BGR Order            | 0 = RGB, 1 = BGR
         + LSHIFT(0, 2) // MH  - Display Data Latch Order | 0 = LCD Refresh Left to Right, 1 = LCD Refresh Right to Left
    ;
    X_OFFSET = 0;
    Y_OFFSET = 0;
    break;
  case 1:
    tmp = 0
         + LSHIFT(0, 7) // MY  - Page Address Order       | 0 = Top to Bottom, 1 = Bottom to Top
         + LSHIFT(1, 6) // MX  - Column Address Order     | 0 = Left to Right, 1 = Right to Left
         + LSHIFT(1, 5) // MV  - Page/Column Order        | 0 = Normal Mode, 1 = Reverse Mode
         + LSHIFT(0, 4) // ML  - Line Address Order       | 0 = LCD Refresh Top to Bottom, 1 = LCD Refresh Bottom to Top
         + LSHIFT(0, 3) // RGB - RGB/BGR Order            | 0 = RGB, 1 = BGR
         + LSHIFT(0, 2) // MH  - Display Data Latch Order | 0 = LCD Refresh Left to Right, 1 = LCD Refresh Right to Left
    ;
    X_OFFSET = 0;
    Y_OFFSET = 0;
    break;
  case 2:
    tmp = 0
         + LSHIFT(1, 7) // MY  - Page Address Order       | 0 = Top to Bottom, 1 = Bottom to Top   // 1 не работает 0 не работает
         + LSHIFT(1, 6) // MX  - Column Address Order     | 0 = Left to Right, 1 = Right to Left   // 1             1
         + LSHIFT(0, 5) // MV  - Page/Column Order        | 0 = Normal Mode, 1 = Reverse Mode
         + LSHIFT(0, 4) // ML  - Line Address Order       | 0 = LCD Refresh Top to Bottom, 1 = LCD Refresh Bottom to Top
         + LSHIFT(0, 3) // RGB - RGB/BGR Order            | 0 = RGB, 1 = BGR
         + LSHIFT(0, 2) // MH  - Display Data Latch Order | 0 = LCD Refresh Left to Right, 1 = LCD Refresh Right to Left
    ;
    X_OFFSET = 0;
    Y_OFFSET = 0x50;
    break;
  case 3:
    tmp = 0
         + LSHIFT(1, 7) // MY  - Page Address Order       | 0 = Top to Bottom, 1 = Bottom to Top
         + LSHIFT(0, 6) // MX  - Column Address Order     | 0 = Left to Right, 1 = Right to Left
         + LSHIFT(1, 5) // MV  - Page/Column Order        | 0 = Normal Mode, 1 = Reverse Mode
         + LSHIFT(0, 4) // ML  - Line Address Order       | 0 = LCD Refresh Top to Bottom, 1 = LCD Refresh Bottom to Top
         + LSHIFT(0, 3) // RGB - RGB/BGR Order            | 0 = RGB, 1 = BGR
         + LSHIFT(0, 2) // MH  - Display Data Latch Order | 0 = LCD Refresh Left to Right, 1 = LCD Refresh Right to Left
    ;
    X_OFFSET = 0x50;
    Y_OFFSET = 0;
    break;


  }


  TFT_wr_data(tmp); //MADCTL: memory data access control

  TFT_wr_cmd(0x003A);
  TFT_wr_data(0x0055); //COLMOD: 16 bit/pixel

  TFT_wr_cmd(0x00B2);
  TFT_wr_data(0x000C);
  TFT_wr_data(0x0C);
  TFT_wr_data(0x00);
  TFT_wr_data(0x33);
  TFT_wr_data(0x33); //PORCTRK: Porch setting

  TFT_wr_cmd(0x00B7);
  TFT_wr_data(0x0035); //GCTRL: Gate Control

  TFT_wr_cmd(0x00BB);
  TFT_wr_data(0x002B); //VCOMS: VCOM setting

  TFT_wr_cmd(0x00C0);
  TFT_wr_data(0x002C); //LCMCTRL: LCM Control

  TFT_wr_cmd(0x00C2);
  TFT_wr_data(0x0001);
  TFT_wr_data(0xFF); //VDVVRHEN: VDV and VRH Command Enable

  TFT_wr_cmd(0x00C3);
  TFT_wr_data(0x0011); //VRHS: VRH Set

  TFT_wr_cmd(0x00C4);
  TFT_wr_data(0x0020); //VDVS: VDV Set

  TFT_wr_cmd(0x00C6);
  TFT_wr_data(0x000F); //FRCTRL2: Frame Rate control in normal mode

  TFT_wr_cmd(0x00D0);
  TFT_wr_data(0x00A4);
  TFT_wr_data(0xA1); //PWCTRL1: Power Control 1

  TFT_wr_cmd(0x00E0);
  TFT_wr_data(0x00D0);
  TFT_wr_data(0x0000);
  TFT_wr_data(0x0005);
  TFT_wr_data(0x000E);
  TFT_wr_data(0x0015);
  TFT_wr_data(0x000D);
  TFT_wr_data(0x0037);
  TFT_wr_data(0x0043);
  TFT_wr_data(0x0047);
  TFT_wr_data(0x0009);
  TFT_wr_data(0x0015);
  TFT_wr_data(0x0012);
  TFT_wr_data(0x0016);
  TFT_wr_data(0x0019); //PVGAMCTRL: Positive Voltage Gamma control

  TFT_wr_cmd(0x00E1);
  TFT_wr_data(0x00D0);
  TFT_wr_data(0x0000);
  TFT_wr_data(0x0005);
  TFT_wr_data(0x000D);
  TFT_wr_data(0x000C);
  TFT_wr_data(0x0006);
  TFT_wr_data(0x002D);
  TFT_wr_data(0x0044);
  TFT_wr_data(0x0040);
  TFT_wr_data(0x000E);
  TFT_wr_data(0x001C);
  TFT_wr_data(0x0018);
  TFT_wr_data(0x0016);
  TFT_wr_data(0x0019); //NVGAMCTRL: Negative Voltage Gamma control

  TFT_wr_cmd(0x002A);
  TFT_wr_data((X_OFFSET >> 8) & 0xFF);
  TFT_wr_data(X_OFFSET       & 0xFF);
  TFT_wr_data(((MAX_X_SZ)>>8) & 0xFF);
  TFT_wr_data(( MAX_X_SZ)&0xFF);

  TFT_wr_cmd(0x002B);
  TFT_wr_data((Y_OFFSET >> 8) & 0xFF);
  TFT_wr_data(Y_OFFSET       & 0xFF);
  TFT_wr_data(((MAX_Y_SZ)>>8) & 0xFF);
  TFT_wr_data(( MAX_Y_SZ)&0xFF);

  TFT_delay(5);



}

/*-----------------------------------------------------------------------------------------------------


  \param x
  \param y
-----------------------------------------------------------------------------------------------------*/
void TFT_Set_coordinates(uint32_t x, uint32_t y)
{
  uint8_t  dbuf[4];
  uint16_t xaddr;
  uint16_t yaddr;

  if (x > 239) x = 239;
  if (y > 239) y = 239;

  xaddr = X_OFFSET + x;
  yaddr = Y_OFFSET + y;

  TFT_wr_cmd(0x2A);
  dbuf[0] =(xaddr >> 8) & 0xFF;
  dbuf[1] =  xaddr & 0xFF;
  dbuf[2] =(MAX_X_SZ >> 8) & 0xFF;
  dbuf[3] = (MAX_X_SZ )&0xFF;
  TFT_wr_data_buf(dbuf, 4);

  TFT_wr_cmd(0x2B);
  dbuf[0] =(yaddr >> 8) & 0xFF;
  dbuf[1] =  yaddr & 0xFF;
  dbuf[2] =(MAX_Y_SZ >> 8) & 0xFF;
  dbuf[3] = (MAX_Y_SZ)&0xFF;
  TFT_wr_data_buf(dbuf, 4);
}

/*-----------------------------------------------------------------------------------------------------


  \param x
  \param y
-----------------------------------------------------------------------------------------------------*/
void TFT_Set_rect(int x0, int y0, int x1, int y1)
{
  uint8_t  dbuf[4];
  uint16_t xaddr0;
  uint16_t xaddr1;

  uint16_t yaddr0;
  uint16_t yaddr1;

  if (x0 > 239) x0 = 239;
  if (y0 > 239) y0 = 239;

  if (x1 > 239) x1 = 239;
  if (y1 > 239) y1 = 239;

  xaddr0 = X_OFFSET + x0;
  xaddr1 = X_OFFSET + x1;

  yaddr0 = Y_OFFSET + y0;
  yaddr1 = Y_OFFSET + y1;

  TFT_wr_cmd(0x2A);
  dbuf[0] =(xaddr0 >> 8) & 0xFF;
  dbuf[1] = xaddr0 & 0xFF;
  dbuf[2] =(xaddr1 >> 8) & 0xFF;
  dbuf[3] = xaddr1 & 0xFF;
  TFT_wr_data_buf(dbuf, 4);

  TFT_wr_cmd(0x2B);
  dbuf[0] =(yaddr0 >> 8) & 0xFF;
  dbuf[1] = yaddr0 & 0xFF;
  dbuf[2] =(yaddr1 >> 8) & 0xFF;
  dbuf[3] = yaddr1 & 0xFF;
  TFT_wr_data_buf(dbuf, 4);
}

/*-----------------------------------------------------------------------------------------------------


  \param x
-----------------------------------------------------------------------------------------------------*/
void TFT_Set_x(uint32_t x)
{
  uint8_t  dbuf[4];
  uint16_t xaddr;

  if (x > 239) x = 239;

  xaddr = X_OFFSET + x;

  TFT_wr_cmd(0x2A);
  dbuf[0] =(xaddr >> 8) & 0xFF;
  dbuf[1] = xaddr & 0xFF;
  dbuf[2] =(MAX_X_SZ >> 8) & 0xFF;
  dbuf[3] = (MAX_X_SZ )&0xFF;
  TFT_wr_data_buf(dbuf, 4);
}

/*-----------------------------------------------------------------------------------------------------


  \param x
-----------------------------------------------------------------------------------------------------*/
void TFT_Set_y(uint32_t y)
{
  uint8_t  dbuf[4];
  uint16_t yaddr;

  if (y > 239) y = 239;

  yaddr = Y_OFFSET + y;

  TFT_wr_cmd(0x2B);
  dbuf[0] =(yaddr >> 8) & 0xFF;
  dbuf[1] = yaddr & 0xFF;
  dbuf[2] =(MAX_Y_SZ >> 8) & 0xFF;
  dbuf[3] = (MAX_Y_SZ)&0xFF;
  TFT_wr_data_buf(dbuf, 4);
}



/*-----------------------------------------------------------------------------------------------------


  \param void
-----------------------------------------------------------------------------------------------------*/
static void TFT_clear_screen(void)
{
  uint16_t w = 0xFFFF;

  TFT_Set_rect(0, 0 , LCD_X_SIZE-1, LCD_Y_SIZE-1);
  TFT_wr_cmd(0x002C);
  TFT_fill_by_pixel(&w, LCD_X_SIZE * LCD_Y_SIZE * 2);
  TFT_wr_cmd(0x0029); //display ON
}


/*-----------------------------------------------------------------------------------------------------
  Прорисовка логотипа
  Битмапы в формате GUI_DRAW_BMPM565 прорисовываются медленно и требуют много памяти.
  Если пямяти не хватает, то прорисовываются с искажениями.
  Если штатным конвертером битмапов от uc/GUI делать формат GUI_DRAW_BMP565, то искажаются цвета и также требуется много памяти, иначе искажения

  Битмапы размером больше 25 кБ без искажений вывести не удается.


  \param p
-----------------------------------------------------------------------------------------------------*/
void Draw_Logo(void *p)
{
  GUI_Clear();
  //GUI_DrawBitmap(&bm_Barduva_logo, 0, 0);
}

/*-----------------------------------------------------------------------------------------------------
  Вывод экрана с моноширинными шрифтами

  \param p
-----------------------------------------------------------------------------------------------------*/
void Draw_fonts_monospace(void *p)
{
  GUI_Clear();
  GUI_SetColor(GUI_WHITE);
  GUI_SetFont(&GUI_Font4x6);
  GUI_DispString("GUI_Font4x6        \n");
  GUI_SetFont(&GUI_Font6x8);
  GUI_DispString("GUI_Font6x8        \n");
  GUI_SetFont(&GUI_Font8x8);
  GUI_DispString("GUI_Font8x8        \n");
  GUI_SetFont(&GUI_Font8x10_ASCII);
  GUI_DispString("GUI_Font8x10_ASCII \n");
  GUI_SetFont(&GUI_Font8x12_ASCII);
  GUI_DispString("GUI_Font8x12_ASCII \n");
  GUI_SetFont(&GUI_Font8x13_ASCII);
  GUI_DispString("GUI_Font8x13_ASCII \n");
  GUI_SetFont(&GUI_Font8x15B_ASCII);
  GUI_DispString("GUI_Font8x15B_ASCII\n");
  GUI_SetFont(&GUI_Font8x16);
  GUI_DispString("GUI_Font8x16       \n");
  GUI_SetFont(&GUI_Font8x16x1x2);
  GUI_DispString("GUI_Font8x16x1x2   \n");
  GUI_SetFont(&GUI_Font8x16x2x2);
  GUI_DispString("GUI_Font8x16x2x2   \n");
  GUI_SetFont(&GUI_Font8x16x3x3);
  GUI_DispString("GUI_Font8x16x3x3   \n");

}

/*-----------------------------------------------------------------------------------------------------
  Вывод экрана с пропорциональными шрифтами

  \param p
-----------------------------------------------------------------------------------------------------*/
void Draw_fonts_proportional(void *p)
{
  GUI_Clear();
  GUI_SetFont(&GUI_Font8_1);
  GUI_DispString("GUI_Font8_1   \n");
  GUI_SetFont(&GUI_Font10S_1);
  GUI_DispString("GUI_Font10S_1 \n");
  GUI_SetFont(&GUI_Font10_1);
  GUI_DispString("GUI_Font10_1  \n");
  GUI_SetFont(&GUI_Font13_1);
  GUI_DispString("GUI_Font13_1  \n");
  GUI_SetFont(&GUI_Font13B_1);
  GUI_DispString("GUI_Font13B_1 \n");
  GUI_SetFont(&GUI_Font13H_1);
  GUI_DispString("GUI_Font13H_1 \n");
  GUI_SetFont(&GUI_Font13HB_1);
  GUI_DispString("GUI_Font13HB_1\n");
  GUI_SetFont(&GUI_Font16_1);
  GUI_DispString("GUI_Font16_1  \n");
  GUI_SetFont(&GUI_Font16B_1);
  GUI_DispString("GUI_Font16B_1 \n");
  GUI_SetFont(&GUI_Font24_1);
  GUI_DispString("GUI_Font24_1  \n");
  GUI_SetFont(&GUI_Font24B_1);
  GUI_DispString("GUI_Font24B_1 \n");
  GUI_SetFont(&GUI_Font32_1);
  GUI_DispString("GUI_Font32_1  \n");
  GUI_SetFont(&GUI_Font32B_1);
  GUI_DispString("GUI_Font32B_1 \n");
}



/*-----------------------------------------------------------------------------------------------------


  \param name_str
  \param fmt
  \param val
  \param x
  \param y
-----------------------------------------------------------------------------------------------------*/
void Display_int_val_str(char *name_str, char *fmt, int32_t val,  LCD_COLOR val_color, uint32_t x, uint32_t y)
{
  GUI_SetColor(GUI_WHITE);
  GUI_SetFont(&GUI_Font8x16);

  snprintf(gui_str,MAX_S_LEN, "%s", name_str);
  GUI_GotoXY(x,y + 15);
  GUI_DispString(gui_str);

  GUI_SetColor(val_color);
  GUI_SetFont(&GUI_Font8x16x3x3);

  snprintf(gui_str,MAX_S_LEN, fmt, val);
  GUI_GotoY(y);
  GUI_DispString(gui_str);
}

/*-----------------------------------------------------------------------------------------------------


  \param name_str
  \param fmt
  \param val
  \param x
  \param y
-----------------------------------------------------------------------------------------------------*/
void Display_float_val_str(char *name_str, char *fmt, float val,  LCD_COLOR val_color, uint32_t x, uint32_t y)
{
  GUI_SetColor(GUI_WHITE);
  GUI_SetFont(&GUI_Font8x16);

  snprintf(gui_str,MAX_S_LEN, "%s", name_str);
  GUI_GotoXY(x,y);
  GUI_DispString(gui_str);

  GUI_SetColor(val_color);
  GUI_SetFont(&GUI_Font8x16);

  snprintf(gui_str,MAX_S_LEN, fmt, val);
  GUI_GotoY(y);
  GUI_DispString(gui_str);
}


/*-----------------------------------------------------------------------------------------------------


  \param p
-----------------------------------------------------------------------------------------------------*/
static void Draw_temperatures_screen(void *p)
{
  char str[32];
  uint32_t y_offs = 2;
  uint32_t x_offs = 0;
  T_app_snapshot   *p_ast = (T_app_snapshot *)p;

  GUI_Clear();

  GUI_Clear();
  GUI_SetColor(GUI_WHITE);

  for (uint32_t i=0; i < TEMP_ZONES_NUM; i++)
  {

    sprintf(str, "%d: ", i+1);
    if (p_ast->sensors_state[i] == 0)
    {
      Display_float_val_str(str, "----", 0,  GUI_RED,  x_offs , y_offs);
    }
    else
    {
      if (p_ast->current_sensor == i)
      {
        Display_float_val_str(str, "%0.1f C", p_ast->temperatures[i],  GUI_RED,  x_offs , y_offs);
      }
      else
      {
        Display_float_val_str(str, "%0.1f C", p_ast->temperatures[i],  GUI_GREEN,  x_offs , y_offs);
      }
    }
    y_offs += 17;
  }

  GUI_SetColor(GUI_WHITE);
  GUI_SetPenSize(2);
  GUI_DrawHLine(y_offs+2, 0, screen_rect.x1-1);
  GUI_DrawVLine(80, 0, y_offs+2);

  GUI_SetColor(GUI_LIGHTBLUE);
  GUI_SetFont(&GUI_Font24B_ASCII);

  y_offs = 0;
  GUI_GotoXY(90,y_offs);
  sprintf(str,"%04d.%02d.%02d", p_ast->tm.YEAR, p_ast->tm.MONTH, p_ast->tm.DAY);
  GUI_DispString(str);
  y_offs += 26;

  GUI_GotoXY(90,y_offs);
  sprintf(str,"%02d:%02d:%02d", p_ast->tm.HOUR, p_ast->tm.MINUTE, p_ast->tm.SECOND);
  GUI_DispString(str);
  y_offs += 26;

  if (p_ast->card_free_space != 0)
  {

    GUI_SetColor(GUI_WHITE);
    GUI_DrawHLine(y_offs, 80, screen_rect.x1-1);
    y_offs += 2;

    GUI_SetColor(GUI_BLUE);
    GUI_SetFont(&GUI_Font16_ASCII);
    GUI_GotoXY(90,y_offs);
    sprintf(str,"SD card space:");
    GUI_DispString(str);
    y_offs += 17;

    GUI_SetColor(GUI_CYAN);
    GUI_GotoXY(90,y_offs);
    sprintf(str,"%lld MiB", g_card_size / (1024 * 1024));
    GUI_DispString(str);
    y_offs += 17;

    GUI_SetColor(GUI_BLUE);
    GUI_GotoXY(90,y_offs);
    sprintf(str,"SD card free space:");
    GUI_DispString(str);
    y_offs += 17;

    GUI_SetColor(GUI_CYAN);
    GUI_GotoXY(90,y_offs);
    sprintf(str,"%lld MiB", p_ast->card_free_space / (1024 * 1024));
    GUI_DispString(str);
  }


  if (p_ast->onew_devices_num > 0)
  {
    GUI_SetColor(GUI_YELLOW);
    GUI_SetFont(&GUI_FontD48x64);
    GUI_GotoXY(0,155);
    sprintf(str, "%0.1f", p_ast->temperature_aver);
    GUI_DispString(str);
  }

}


/*-----------------------------------------------------------------------------------------------------


-----------------------------------------------------------------------------------------------------*/
void  GUI_task_create(void)
{
  _mqx_uint res;

  TASK_TEMPLATE_STRUCT  task_template = {0};
  task_template.TASK_NAME          = "GUI";
  task_template.TASK_PRIORITY      = GUI_TASK_PRIO;
  task_template.TASK_STACKSIZE     = 2048;
  task_template.TASK_ADDRESS       = Task_GUI;
  task_template.TASK_ATTRIBUTES    = MQX_FLOATING_POINT_TASK;
  task_template.CREATION_PARAMETER = 0;
  gui_task_id =  _task_create(0, 0, (uint32_t)&task_template);
}


/*-----------------------------------------------------------------------------------------------------


  \param initial_data
-----------------------------------------------------------------------------------------------------*/
static void Task_GUI(uint32_t initial_data)
{
  TFT_init();
  TFT_clear_screen();

  GUI_Init();

  GUI_GetClientRect(&screen_rect);
  GUI_SetBkColor(GUI_BLACK);



  for (;;)
  {
    Get_app_snapshot(&app_snapshot);
    GUI_MEMDEV_Draw(&screen_rect, Draw_temperatures_screen,&app_snapshot, 0, GUI_MEMDEV_NOTRANS);
    TFT_delay(100);
  }
}

