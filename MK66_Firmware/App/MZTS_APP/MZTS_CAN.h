#ifndef __MZTS_CAN_H
  #define __MZTS_CAN_H


#define  CAN_LOST 1
#define  CAN_OK   0

uint32_t CAN_connection_state(void);
void     CAN_app_task_tx(void);
void     CAN_app_task_rx(void);
uint32_t CAN_check_connection_timeout(void);

#endif // RB150V2MC_CAN_H



