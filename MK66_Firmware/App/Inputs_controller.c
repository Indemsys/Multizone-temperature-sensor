// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.07.05
// 09:27:50
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

#define FLAG_ADC_DONE               BIT( 0 ) // Флаг собьтия окончания цикла измерений многоканального АЦП
#define FLAG_INPUTS_CHANGED         BIT( 1 ) // Флаг события изменения состояния входов

extern void FMSTR_Recorder(void);

LWEVENT_STRUCT inputs_lwev;

/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
void Inputs_create_sync_obj(void)
{
  // Создаем объект синхронизации
  _lwevent_create(&inputs_lwev, LWEVENT_AUTO_CLEAR);
}

/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
void Inputs_set_ADC_evt(void)
{
  _lwevent_set(&inputs_lwev, FLAG_ADC_DONE);
}

/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
void Inputs_set_changed_evt(void)
{
  _lwevent_set(&inputs_lwev, FLAG_INPUTS_CHANGED);
}
/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
_mqx_uint Inputs_wait_for_ADC_event(void)
{
  return _lwevent_wait_for(&inputs_lwev, FLAG_ADC_DONE, TRUE, NULL);
}

/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
_mqx_uint Inputs_wait_for_changed_event(uint32_t ticks)
{
  return _lwevent_wait_ticks(&inputs_lwev, FLAG_INPUTS_CHANGED, TRUE, ticks);
}


/*------------------------------------------------------------------------------
  Процедура определения состояния сигнала

  Возвращает 1 если значение сигнала изменилось, иначе 0
 ------------------------------------------------------------------------------*/
uint32_t Inputs_do_processing(T_input_cbl *scbl)
{
  uint8_t upper_sig, lower_sig;

  if (scbl->itype == GEN_SW)
  {
    // Обрабатываем сигнал с бистабильного датчика
    if (*scbl->p_smpl1 > scbl->lbound) scbl->pbncf.curr = 1;
    else scbl->pbncf.curr = 0;
  }
  else if (scbl->itype == ESC_SW)
  {
    // Обрабатываем сигнал с датчика обладающего неопределенным состоянием (контакты в цепи безопасности)
    if (*scbl->p_smpl1 > scbl->lbound) upper_sig = 1;
    else upper_sig = 0;

    if (*scbl->p_smpl2 > scbl->lbound) lower_sig = 1;
    else lower_sig = 0;

    if ((upper_sig) && (lower_sig))
    {
      scbl->pbncf.curr = 0;
    }
    else if ((upper_sig) && (!lower_sig))
    {
      scbl->pbncf.curr = 1;
    }
    else
    {
      scbl->pbncf.curr = -1;
    }

  }
  else return 0;


  if (scbl->pbncf.init == 0)
  {
    scbl->pbncf.init = 1;
    scbl->pbncf.prev = scbl->pbncf.curr;
    *scbl->val  = scbl->pbncf.curr;
    *scbl->val_prev = *scbl->val;
    return 0;
  }

  if (scbl->pbncf.prev != scbl->pbncf.curr)
  {
    scbl->pbncf.cnt = 0;
    scbl->pbncf.prev = scbl->pbncf.curr;
  }

  if ((scbl->pbncf.curr == 0) && (scbl->pbncf.cnt == scbl->l0_time))
  {
    *scbl->val_prev = *scbl->val;
    *scbl->val = scbl->pbncf.curr;
    *scbl->flag = 1;
    scbl->pbncf.cnt++;
    return 1;
  }
  else if ((scbl->pbncf.curr == 1) && (scbl->pbncf.cnt == scbl->l1_time))
  {
    *scbl->val_prev = *scbl->val;
    *scbl->val = scbl->pbncf.curr;
    *scbl->flag = 1;
    scbl->pbncf.cnt++;
    return 1;
  }
  else if ((scbl->pbncf.curr == -1) && (scbl->pbncf.cnt == scbl->lu_time))
  {
    *scbl->val_prev = *scbl->val;
    *scbl->val = scbl->pbncf.curr;
    *scbl->flag = 1;
    scbl->pbncf.cnt++;
    return 1;
  }
  else
  {
    if (scbl->pbncf.cnt < MAX_UINT_32)
    {
      scbl->pbncf.cnt++;
    }
  }

  return 0;

}


/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
void Inputs_processing_task(void)
{



}


