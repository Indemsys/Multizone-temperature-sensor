// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 2016.07.07
// 15:07:12
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include   "App.h"

extern volatile CAN_MemMapPtr        CAN_PTR;
extern LWEVENT_STRUCT                can_rx_event;
extern LWEVENT_STRUCT                can_tx_event;
extern uint32_t                      can_events_mask;
extern T_can_statistic               can_stat;
extern _queue_id                     can_tx_queue_id;
extern const T_can_rx_config         rx_mboxes[RX_MBOX_CNT];

static   TIME_STRUCT                 last_packet_timestup;


static  uint32_t                     g_can_connection_state = CAN_LOST;

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t CAN_check_connection_timeout(void)
{
  if (Time_elapsed_ms(&last_packet_timestup) > 1000)
  {
    if (g_can_connection_state == CAN_OK)
    {
      LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT, "Connection with main controller fault");
    }
    g_can_connection_state=  CAN_LOST;
  }

  return g_can_connection_state;
}

/*-----------------------------------------------------------------------------------------------------


  \param void

  \return uint32_t
-----------------------------------------------------------------------------------------------------*/
uint32_t CAN_connection_state(void)
{
  return g_can_connection_state;
}
/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
void CAN_app_task_tx(void)
{
  T_can_tx_message *cmpt;



  while (1)
  {
    // Извлекаем сообщения из очереди
    cmpt = _msgq_receive(can_tx_queue_id, 0);
    if (cmpt != NULL)
    {
      CAN_send_packet(cmpt->canid, cmpt->data, cmpt->len, cmpt->rtr);
      if (_lwevent_wait_ticks(&can_tx_event, BIT(CAN_TX_MB1), FALSE, 100) != MQX_OK)
      {
        can_stat.tx_err_cnt++;
      }
      _msg_free(cmpt);
    }
    else _time_delay_ticks(2); // Некоторая пауза если ошибки очереди начнут идти непрерывно
  }
}


/*------------------------------------------------------------------------------


 ------------------------------------------------------------------------------*/
void CAN_app_task_rx(void)
{
  T_can_rx               rx;
  _mqx_uint              events;
  uint32_t               n;
  uint32_t               wticks = ms_to_ticks(CAN_RX_WIAT_MS);

  _time_get(&last_packet_timestup);

  while (1)
  {

    if (_lwevent_wait_ticks(&can_rx_event, can_events_mask, FALSE, wticks) == MQX_OK)
    {
      events = _lwevent_get_signalled();
      _lwevent_clear(&can_rx_event, events);

      for (n = 0; n < RX_MBOX_CNT; n++)
      {
        if (events & (1 << rx_mboxes[n].mb_num))
        {
          _time_get(&last_packet_timestup);

          CAN_read_rx_mbox(CAN_PTR, rx_mboxes[n].mb_num,&rx);

#ifdef ENABLE_CAN_LOG
          CAN_push_log_rec(&rx);
#endif

        }
      }
    }
  }
}




