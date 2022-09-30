#include "App.h"
#include "MFS_test.h"


#define COL        80   /* Maximum column size       */

#define EDSTLEN    20


static void Do_misc_test(uint8_t keycode);
static void Do_show_event_log(uint8_t keycode);
static void Do_date_time_set(uint8_t keycode);
static void Do_VBAT_RAM_diagnostic(uint8_t keycode);
static void Do_Reset(uint8_t keycode);

extern const T_VT100_Menu MENU_MAIN;
extern const T_VT100_Menu MENU_PARAMETERS;
extern const T_VT100_Menu MENU_SPEC;


static int32_t Lookup_menu_item(T_VT100_Menu_item **item, uint8_t b);



/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      Пункты имеющие свое подменю распологаются на следующем уровне вложенности
      Их функция подменяет в главном цикле обработчик нажатий по умолчанию и должна
      отдавать управление периодически в главный цикл

      Пункты не имеющие функции просто переходят на следующий уровень подменю

      Пункты не имеющие подменю полностью захватывают управление и на следующий уровень не переходят

      Пункты без подменю и функции означают возврат на предыдущий уровень
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

//-------------------------------------------------------------------------------------
const T_VT100_Menu_item MENU_MAIN_ITEMS[] =
{
  { '1', Do_Params_editor, (void *)&MENU_PARAMETERS },
  { '3', Do_date_time_set, 0 },
  { '4', Do_show_event_log, 0 },
  { '6', 0, (void *)&MENU_SPEC },
  { '7', Do_Reset, 0  },
  { 'R', 0, 0 },
  { 'M', 0, (void *)&MENU_MAIN },
  { 0 }
};

const T_VT100_Menu      MENU_MAIN             =
{
  "K66BLEZ controller",
  "\033[5C MAIN MENU \r\n"
  "\033[5C <1> - Adjustable parameters and settings\r\n"
  "\033[5C <3> - Setup date and time\r\n"
  "\033[5C <4> - Log\r\n"
  "\033[5C <6> - Special tools menu\r\n"
  "\033[5C <7> - Reset\r\n",
  MENU_MAIN_ITEMS,
};

//-------------------------------------------------------------------------------------
const T_VT100_Menu      MENU_PARAMETERS       =
{
  "",
  "",
  0
};

const T_VT100_Menu_item MENU_SPEC_ITEMS[] =
{
  { '1', Do_VBAT_RAM_diagnostic, 0 },
  { 'R', 0, 0 },
  { 'M', 0, (void *)&MENU_MAIN },
  { 0 }
};

const T_VT100_Menu      MENU_SPEC  =
{
  "MONITOR Ver.180924",
  "\033[5C SPECIAL MENU \r\n"
  "\033[5C <1> - VBAT RAM diagnostic\r\n"
  "\033[5C <R> - Display previous menu\r\n"
  "\033[5C <M> - Display main menu\r\n",
  MENU_SPEC_ITEMS,
};


const char              *_days_abbrev[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char              *_months_abbrev[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

extern uint32_t           test_attempt;


/*-------------------------------------------------------------------------------------------------------------
   Процедура ожидания корректной последовательности данных для подтверждения входа с терминала
   Используется для фильтации шумов на длинном кабеле
-------------------------------------------------------------------------------------------------------------*/
const uint8_t             entry_str[] = "12345678";

void Entry_check(void)
{
  uint32_t       indx;
  uint8_t        b;
  GET_MCBL;

  MPRINTF("Press this sequence to enter - %s:\r\n", entry_str);
  indx = 0;
  while (1)
  {
    if (WAIT_CHAR(&b, 200) == MQX_OK)
    {
      if (b != 0)
      {
        if (b == entry_str[indx])
        {
          indx++;
          if (indx == (sizeof(entry_str)- 1))
          {
            return;
          }
        }
        else
        {
          indx = 0;
        }
      }
    }
  }
}


/*-------------------------------------------------------------------------------------------------------------
  Вывод текущего времени
-------------------------------------------------------------------------------------------------------------*/
static void Print_RTC_time(uint32_t rtc_time)
{
  TIME_STRUCT     ts;
  DATE_STRUCT     tm;
  GET_MCBL;

  ts.SECONDS = rtc_time;
  ts.MILLISECONDS = 0;
  if (_time_to_date(&ts,&tm) == FALSE)
  {
    MPRINTF(VT100_CLR_LINE"Current time is      : - - - - - -");
    return;
  }
  // Вывод времени без дня недели
  MPRINTF(VT100_CLR_LINE"Current time is      : %s %2d %d %.2d:%.2d:%.2d", _months_abbrev[tm.MONTH - 1], tm.DAY, tm.YEAR, tm.HOUR, tm.MINUTE, tm.SECOND);
}

/*-------------------------------------------------------------------------------------------------------------
   Задача монитора
-------------------------------------------------------------------------------------------------------------*/
void Task_VT100(uint32_t initial_data)
{
  uint8_t    b;
  uint32_t rtc_time;

  T_monitor_cbl *mcbl;
  T_monitor_driver *drv;

  // Выделим память для управляющей структуры задачи
  mcbl = (T_monitor_cbl *)_mem_alloc_zero(sizeof(T_monitor_cbl));
  if (mcbl == NULL) return;
  _task_set_environment(_task_get_id(), mcbl);

  // Определим указатели функций ввода-вывода
  drv = (T_monitor_driver *)_task_get_parameter();
  mcbl->pdrv = drv;
  MPRINTF    = drv->_printf;
  WAIT_CHAR = drv->_wait_char;
  mcbl->_send_buf  = drv->_send_buf;

  if (drv->_init(&drv->pdrvcbl, drv) != MQX_OK) return; // Выходим из задачи если не удалось инициализировать драйвер.


  // Очищаем экран
  MPRINTF(VT100_CLEAR_AND_HOME);


  //Entry_check();
  Goto_main_menu();

  do
  {
    if (WAIT_CHAR(&b, 200) == MQX_OK)
    {
      if (b != 0)
      {
        if (b == 0x1B)
        {
          // При вводе символа ESC  переходим в главное меню за исключением случая когда находимс я режиме редактирования параметра
          if (mcbl->Monitor_func != Edit_func)
          {
            MPRINTF(VT100_CLEAR_AND_HOME);
            Goto_main_menu();
          }
        }
        else if (b < 0x80)
        {
          // Если назначена функция для исполнения, то передаем управление ей
          if (mcbl->Monitor_func)
          {
            mcbl->Monitor_func(b);  // Обработчик нажатий главного цикла
          }
        }
      }
    }


    if (mcbl->menu_trace[mcbl->menu_nesting] == &MENU_MAIN)
    {
      unsigned int uid[4];

      VT100_set_cursor_pos(22, 0);
      _bsp_get_unique_identificator(uid);
      MPRINTF("UID                  : %08X %08X %08X %08X", uid[0], uid[1], uid[2], uid[3]);
      VT100_set_cursor_pos(23, 0);
      MPRINTF("Firmware compile time: %s", fw_version);
      VT100_set_cursor_pos(24, 0);
      _rtc_get_time(&rtc_time);
      Print_RTC_time(rtc_time);

    }


  }while (1);

}


/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
void Set_monitor_func(void (*func)(unsigned char))
{
  GET_MCBL;

  mcbl->Monitor_func = func;
}



/*-------------------------------------------------------------------------------------------------------------
  Вывести на экран текущее меню
  1. Вывод заголовока меню
  2. Вывод содержания меню
-------------------------------------------------------------------------------------------------------------*/
void Display_menu(void)
{
  GET_MCBL;

  MPRINTF(VT100_CLEAR_AND_HOME);

  if (mcbl->menu_trace[mcbl->menu_nesting] == 0) return;

  VT100_send_str_to_pos((uint8_t *)mcbl->menu_trace[mcbl->menu_nesting]->menu_header, 1, Find_str_center((uint8_t *)mcbl->menu_trace[mcbl->menu_nesting]->menu_header));
  VT100_send_str_to_pos(DASH_LINE, 2, 0);
  VT100_send_str_to_pos((uint8_t *)mcbl->menu_trace[mcbl->menu_nesting]->menu_body, 3, 0);
  MPRINTF("\r\n");
  MPRINTF(DASH_LINE);

}
/*-------------------------------------------------------------------------------------------------------------
  Поиск в текущем меню пункта вызываемого передаваемым кодом
  Параметры:
    b - код команды вазывающей пункт меню
  Возвращает:
    Указатель на соответствующий пункт в текущем меню
-------------------------------------------------------------------------------------------------------------*/
int32_t Lookup_menu_item(T_VT100_Menu_item **item, uint8_t b)
{
  int16_t           i;
  GET_MCBL;

  if (isalpha(b) != 0) b = toupper(b);

  i = 0;
  do
  {
    *item = (T_VT100_Menu_item *)mcbl->menu_trace[mcbl->menu_nesting]->menu_items + i;
    if ((*item)->but == b) return (1);
    if ((*item)->but == 0) break;
    i++;
  }while (1);

  return (0);
}


/*----------------------------------------------------------------------------
 *      Line Editor
 *---------------------------------------------------------------------------*/
static int Get_string(char *lp, int n)
{
  int  cnt = 0;
  char c;
  GET_MCBL;

  do
  {
    if (WAIT_CHAR((unsigned char *)&c, 200) == MQX_OK)
    {
      switch (c)
      {
      case VT100_CNTLQ:                          /* ignore Control S/Q             */
      case VT100_CNTLS:
        break;

      case VT100_BCKSP:
      case VT100_DEL:
        if (cnt == 0)
        {
          break;
        }
        cnt--;                             /* decrement count                */
        lp--;                              /* and line VOID*               */
        /* echo backspace                 */
        MPRINTF("\x008 \x008");
        break;
      case VT100_ESC:
        *lp = 0;                           /* ESC - stop editing line        */
        return (MQX_ERROR);
      default:
        MPRINTF("*");
        *lp = c;                           /* echo and store character       */
        lp++;                              /* increment line VOID*         */
        cnt++;                             /* and count                      */
        break;
      }
    }
  }while (cnt < n - 1 && c != 0x0d);      /* check limit and line feed      */
  *lp = 0;                                 /* mark end of string             */
  return (MQX_OK);
}

/*-------------------------------------------------------------------------------------------------------------
  Ввод кода для доступа к специальному меню
-------------------------------------------------------------------------------------------------------------*/
int Enter_special_code(void)
{
  char str[32];
  GET_MCBL;

  if (mcbl->g_access_to_spec_menu != 0)
  {
    return MQX_OK;
  }
  MPRINTF(VT100_CLEAR_AND_HOME"Enter access code>");
  if (Get_string(str, 31) == MQX_OK)
  {
    if (strcmp(str, "4321\r") == 0)
    {
      mcbl->g_access_to_spec_menu = 1;
      return MQX_OK;
    }
  }

  return MQX_ERROR;
}
/*-------------------------------------------------------------------------------------------------------------
  Поиск пункта меню по коду вызова (в текущем меню)
  и выполнение соответствующей ему функции
  Параметры:
    b - код команды вазывающей пункт меню

-------------------------------------------------------------------------------------------------------------*/
void Execute_menu_func(uint8_t b)
{
  T_VT100_Menu_item *menu_item;
  GET_MCBL;

  if (Lookup_menu_item(&menu_item, b) != 0)
  {
    // Нашли соответствующий пункт меню
    if (menu_item->psubmenu != 0)
    {
      // Если присутствует субменю, то вывести его

      if ((T_VT100_Menu *)menu_item->psubmenu == &MENU_SPEC)
      {
        if (Enter_special_code() != MQX_OK)
        {
          Display_menu();
          return;
        }
      }

      mcbl->menu_nesting++;
      mcbl->menu_trace[mcbl->menu_nesting] = (T_VT100_Menu *)menu_item->psubmenu;

      Display_menu();
      // Если есть функция у пункта меню, то передать ей обработчик нажатий в главном цикле и выполнить функцию.
      if (menu_item->func != 0)
      {
        mcbl->Monitor_func = (T_menu_func)(menu_item->func); // Установить обработчик нажатий главного цикла на функцию из пункта меню
        menu_item->func(0);                // Выполнить саму функцию меню
      }
    }
    else
    {
      if (menu_item->func == 0)
      {
        // Если нет ни субменю ни функции, значит это пункт возврата в предыдущее меню
        // Управление остается в главном цикле у обработчика по умолчанию
        Return_to_prev_menu();
        Display_menu();
      }
      else
      {
        // Если у пункта нет своего меню, то перейти очистить экран и перейти к выполению  функции выбранного пункта
        MPRINTF(VT100_CLEAR_AND_HOME);
        menu_item->func(0);
        // Управление возвращается в главный цикл обработчику по умолчанию
        Display_menu();
      }
    }

  }
}


/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
void Goto_main_menu(void)
{
  GET_MCBL;

  mcbl->menu_nesting = 0;
  mcbl->menu_trace[mcbl->menu_nesting] = (T_VT100_Menu *)&MENU_MAIN;
  Display_menu();
  mcbl->Monitor_func = Execute_menu_func; // Назначение функции
}

/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
void Return_to_prev_menu(void)
{
  GET_MCBL;

  if (mcbl->menu_nesting > 0)
  {
    mcbl->menu_trace[mcbl->menu_nesting] = 0;
    mcbl->menu_nesting--;
  }
  mcbl->Monitor_func = Execute_menu_func; // Назначение функции
}



/*-------------------------------------------------------------------------------------------------------------
  Очистка экрана монитора
-------------------------------------------------------------------------------------------------------------*/
void VT100_clr_screen(void)
{
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME);
}

/*-------------------------------------------------------------------------------------------------------------
     Установка курсора в заданную позицию
-------------------------------------------------------------------------------------------------------------*/
void VT100_set_cursor_pos(uint8_t row, uint8_t col)
{
  GET_MCBL;
  MPRINTF("\033[%.2d;%.2dH", row, col);
}

/*-------------------------------------------------------------------------------------------------------------
     Вывод строки в заданную позицию
-------------------------------------------------------------------------------------------------------------*/
void VT100_send_str_to_pos(uint8_t *str, uint8_t row, uint8_t col)
{
  GET_MCBL;
  MPRINTF("\033[%.2d;%.2dH", row, col);
  mcbl->_send_buf(str, strlen((char *)str));
}

/*-------------------------------------------------------------------------------------------------------------
    Находим позицию начала строки для расположения ее по центру экрана
-------------------------------------------------------------------------------------------------------------*/
uint8_t Find_str_center(uint8_t *str)
{
  int16_t l = 0;
  while (*(str + l) != 0) l++; // Находим длину строки
  return (COLCOUNT - l) / 2;
}


/*-----------------------------------------------------------------------------------------------------
  Ввод строки

  \param buf      - буффер для вводимых символов
  \param buf_len  - размер буфера c учетом нулевого байта
  \param row      - строка в которой будет производится ввод
  \param instr    - строка с начальным значением

  \return int32_t - MQX_OK если ввод состоялся
-----------------------------------------------------------------------------------------------------*/
int32_t Mon_input_line(char *buf, int buf_len, int row, char *instr)
{

  int   i;
  uint8_t b;
  int   res;
  uint8_t bs_seq[] = { VT100_BCKSP, ' ', VT100_BCKSP, 0 };
  GET_MCBL;

  i = 0;
  VT100_set_cursor_pos(row, 0);
  MPRINTF(VT100_CLL_FM_CRSR);
  MPRINTF(">");

  if (instr != 0)
  {
    i = strlen(instr);
    if (i < buf_len)
    {
      MPRINTF(instr);

    }
    else i = 0;
  }

  do
  {
    if (WAIT_CHAR(&b, 20000) != MQX_OK)
    {
      res = MQX_ERROR;
      goto exit_;
    };

    if (b == VT100_ESC)
    {
      res = MQX_ERROR;
      goto exit_;
    }
    if (b == VT100_BCKSP)
    {
      if (i > 0)
      {
        i--;
        mcbl->_send_buf(bs_seq, sizeof(bs_seq));
      }
    }
    else if (b != VT100_CR && b != VT100_LF && b != 0)
    {
      mcbl->_send_buf(&b, 1);
      buf[i] = b;           /* String[i] value set to alpha */
      i++;
      if (i >= buf_len)
      {
        res = MQX_ERROR;
        goto exit_;
      };
    }
  }while ((b != VT100_CR) && (i < COL));

  res = MQX_OK;
  buf[i] = 0;                     /* End of string set to NUL */
exit_:

  VT100_set_cursor_pos(row, 0);
  MPRINTF(VT100_CLL_FM_CRSR);

  return (res);
}

/*------------------------------------------------------------------------------
 Вывод дампа области памяти


 \param addr       - выводимый начальный адрес дампа
 \param buf        - указатель на память
 \param buf_len    - количество байт
 \param sym_in_str - количество выводимых байт в строке дампа

 \return int32_t
 ------------------------------------------------------------------------------*/
void Mon_print_dump(uint32_t addr, void *buf, uint32_t buf_len, uint8_t sym_in_str)
{

  uint32_t   i;
  uint32_t   scnt;
  uint8_t    *pbuf;
  GET_MCBL;

  pbuf = (uint8_t *)buf;
  scnt = 0;
  for (i = 0; i < buf_len; i++)
  {
    if (scnt == 0)
    {
      MPRINTF("%08X: ", addr);
    }

    MPRINTF("%02X ", pbuf[ i ]);

    addr++;
    scnt++;
    if (scnt >= sym_in_str)
    {
      scnt = 0;
      MPRINTF("\r\n");
    }
  }

  if (scnt != 0)
  {
    MPRINTF("\r\n");
  }
}


/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void Edit_uinteger_val(uint32_t row, uint32_t *value, uint32_t minv, uint32_t maxv)
{
  char   str[32];
  char   buf[32];
  uint32_t tmpv;
  sprintf(str, "%d",*value);
  if (Mon_input_line(buf, 31, row, str) == MQX_OK)
  {
    if (sscanf(buf, "%d",&tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void Edit_integer_val(uint32_t row, int32_t *value, int32_t minv, int32_t maxv)
{
  char   str[32];
  char   buf[32];
  int32_t tmpv;
  sprintf(str, "%d",*value);
  if (Mon_input_line(buf, 31, row, str) == MQX_OK)
  {
    if (sscanf(buf, "%d",&tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}
/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void Edit_float_val(uint32_t row, float *value, float minv, float maxv)
{
  char   str[32];
  char   buf[32];
  float tmpv;
  sprintf(str, "%f",*value);
  if (Mon_input_line(buf, 31, row, str) == MQX_OK)
  {
    if (sscanf(buf, "%f",&tmpv) == 1)
    {
      if (tmpv > maxv) tmpv = maxv;
      if (tmpv < minv) tmpv = minv;
      *value = tmpv;
    }
  }
}

/*-------------------------------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------------------------------*/
static uint8_t BCD2ToBYTE(uint8_t value)
{
  uint32_t tmp;
  tmp =((value & 0xF0)>> 4) * 10;
  return(uint8_t)(tmp +(value & 0x0F));
}
/*-----------------------------------------------------------------------------------------------------
  Установка даты и времени
-----------------------------------------------------------------------------------------------------*/
static void Do_date_time_set(uint8_t keycode)
{
  unsigned char      i, k, b;
  uint8_t              buf[EDSTLEN];
  TIME_STRUCT        mqx_time;
  DATE_STRUCT        date_time;
  uint32_t           rtc_time;
  GET_MCBL;

  MPRINTF(VT100_CLEAR_AND_HOME);

  VT100_send_str_to_pos("SYSTEM TIME SETTING", 1, 30);
  VT100_send_str_to_pos("\033[5C <M> - Display main menu, <Esc> - Exit \r\n", 2, 10);
  VT100_send_str_to_pos("Print in form [YY.MM.DD dd HH.MM.SS]:  .  .        :  :  ", SCR_ITEMS_VERT_OFFS, 1);

  mcbl->beg_pos = 38;
  k = 0;
  mcbl->curr_pos = mcbl->beg_pos;
  VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->curr_pos);
  MPRINTF((char *)VT100_CURSOR_ON);

  for (i = 0; i < EDSTLEN; i++) buf[i] = 0;

  do
  {
    if (WAIT_CHAR(&b, 200) == MQX_OK)
    {
      switch (b)
      {
      case VT100_BCKSP:  // Back Space
        if (mcbl->curr_pos > mcbl->beg_pos)
        {
          mcbl->curr_pos--;
          k--;
          switch (k)
          {
          case 2:
          case 5:
          case 8:
          case 11:
          case 14:
          case 17:
            k--;
            mcbl->curr_pos--;
            break;
          }

          VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->curr_pos);
          MPRINTF((char *)" ");
          VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->curr_pos);
          buf[k] = 0;
        }
        break;
      case VT100_DEL:  // DEL
        mcbl->curr_pos = mcbl->beg_pos;
        k = 0;
        for (i = 0; i < EDSTLEN; i++) buf[i] = 0;
        VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->beg_pos);
        MPRINTF((char *)"  .  .        :  :  ");
        VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->beg_pos);
        break;
      case VT100_ESC:  // ESC
        MPRINTF((char *)VT100_CURSOR_OFF);
        return;
      case 'M':  //
      case 'm':  //
        MPRINTF((char *)VT100_CURSOR_OFF);
        return;

      case VT100_CR:  // Enter
        MPRINTF((char *)VT100_CURSOR_OFF);

        date_time.YEAR     = BCD2ToBYTE((buf[0] << 4)+ buf[1])+ 2000;
        date_time.MONTH    = BCD2ToBYTE((buf[3] << 4)+ buf[4]);
        date_time.DAY      = BCD2ToBYTE((buf[6] << 4)+ buf[7]);
        date_time.HOUR     = BCD2ToBYTE((buf[12] << 4)+ buf[13]);
        date_time.MINUTE   = BCD2ToBYTE((buf[15] << 4)+ buf[16]);
        date_time.SECOND   = BCD2ToBYTE((buf[18] << 4)+ buf[19]);
        date_time.MILLISEC = 0;

        if (_time_from_date(&date_time,&mqx_time) == FALSE)
        {
          MPRINTF("\r\nCannot convert date_time!");
          _time_delay(3000);
          return;
        }
        rtc_time = mqx_time.SECONDS;
        _time_set(&mqx_time);      // Установка времени RTOS
        _rtc_set_time(rtc_time);   // Установка времени RTC
        g_RTC_state = 1;
        return;
      default:
        if (isdigit(b))
        {
          if (k < EDSTLEN)
          {
            uint8_t str[2];
            str[0] = b;
            str[1] = 0;
            MPRINTF((char *)str);
            buf[k] = b - 0x30;
            mcbl->curr_pos++;
            k++;
            switch (k)
            {
            case 2:
            case 5:
            case 8:
            case 11:
            case 14:
            case 17:
              k++;
              mcbl->curr_pos++;
              break;
            }
            VT100_set_cursor_pos(SCR_ITEMS_VERT_OFFS, mcbl->curr_pos);
          }
        }
        break;

      } // switch
    }
  }while (1);
}



/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
static void Do_show_event_log(uint8_t keycode)
{
  GET_MCBL;
  AppLogg_monitor_output(mcbl);
}



/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
static void Do_VBAT_RAM_diagnostic(uint8_t keycode)
{
  uint8_t  b;
  GET_MCBL;

  do
  {
    MPRINTF(VT100_CLEAR_AND_HOME);
    MPRINTF("VBAT RAM diagnostic.\n\r");
    MPRINTF("Press 'R' to exit. \n\r");
    MPRINTF(DASH_LINE);
    Mon_print_dump((uint32_t)VBAT_RAM_ptr, VBAT_RAM_ptr, sizeof(T_VBAT_RAM), 8);

    if (WAIT_CHAR(&b, 2000) == MQX_OK)
    {
      switch (b)
      {
      case 'S':
      case 's':
        K66BLEZ1_VBAT_RAM_sign_data();
        break;
      case 'V':
      case 'v':
        if (K66BLEZ1_VBAT_RAM_validation() == MQX_OK)
        {
          MPRINTF("Data Ok.\n\r");
        }
        else
        {
          MPRINTF("Data incorrect!.\n\r");
        }
        _time_delay(1000);
        break;
      case 'R':
      case 'r':
      case VT100_ESC:
        return;
      }
    }
  }while (1);
}


/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void Do_misc_test(uint8_t keycode)
{
  uint8_t  b;
  GET_MCBL;
  MPRINTF(VT100_CLEAR_AND_HOME"Misc. test.\n\r");
  MPRINTF("Press 'R' to exit. \n\r");

//   Тестирование кодирования и декодирования Base64
//
//   {
//   int i, n;
//   char plainbuf[16];
//   char encbuf[32];
//   char encbuf2[48];
//
//
//   mprintf("Plain buf: \n\r");
//   for (i = 0; i < sizeof(plainbuf); i++)
//   {
//     plainbuf[i] = rand();
//     mprintf("%02X ", plainbuf[i]);
//   }
//   mprintf("\r\nEncoded buf len: %d\n\r", Base64encode_len(sizeof(plainbuf)));
//
//   n = Base64encode(encbuf, plainbuf, sizeof(plainbuf));
//   mprintf("Encoded: %d\n\r",n);
//   mprintf("\r\nEncoded buf: %s\n\r", encbuf);
//   Base64encode(encbuf2, encbuf, n-1);
//   mprintf("\r\nSecond encoded buf: %s\n\r", encbuf2);
//
//   n = Base64decode_len(encbuf);
//   mprintf("\r\n\r\nDecoded buf len: %d\n\r", n);
//   n = Base64decode(plainbuf, encbuf);
//   mprintf("Decoded %d\n\r",i);
//   mprintf("Decoded buf: \n\r");
//   for (i = 0; i < n; i++)
//   {
//     mprintf("%02X ", plainbuf[i]);
//   }
//   mprintf("\r\n");
// }


// Тестирование работы watchdog
  {
    unsigned int i;
    _int_disable();
    for (i = 0; i < 1000000000ul; i++)
    {
      __no_operation();
    }
    _int_enable();
  }


  do
  {
    if (WAIT_CHAR(&b, 200) == MQX_OK)
    {
      switch (b)
      {
      case 'R':
      case 'r':
      case VT100_ESC:

        return;
      }
    }
  }while (1);
}


/*------------------------------------------------------------------------------
 Сброс системы
 ------------------------------------------------------------------------------*/
static void Do_Reset(uint8_t keycode)
{
  Reset_system();
}


