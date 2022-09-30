/*! *********************************************************************************
 * \addtogroup BLE
 * @{
 ********************************************************************************** */
/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file ble_host_tasks.c
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "App.h"


task_handler_t gHost_TaskId;
osaEventId_t   gHost_TaskEvent;
msgQueue_t     gHost_TaskQueue;

task_handler_t gL2ca_TaskId;
osaEventId_t   gL2ca_TaskEvent;
msgQueue_t     gL2ca_TaskQueue;

static void Host_Task(task_param_t argument);
static void L2ca_Task(task_param_t argument);


OSA_TASK_DEFINE(HOST, HOST_TASK_STACK_SZ);
OSA_TASK_DEFINE(L2CA, HOST_TASK_STACK_SZ);


T_event_obj host_event_obj;
T_event_obj l2ca_event_obj;


osaStatus_t Ble_HostTaskInit(void)
{
  osa_status_t status;

  DEBUG_PRINT("Ble_HostTaskInit\r\n");
  /* Already initialized? */
  if (gHost_TaskId && gL2ca_TaskId)
  {
    return osaStatus_Error;
  }

  gHost_TaskEvent = &host_event_obj;
  gL2ca_TaskEvent = &l2ca_event_obj;
  if (OSA_EventCreate(&(host_event_obj.os_event), kEventAutoClear) != kStatus_OSA_Success) return osaStatus_Error; 
  if (OSA_EventCreate(&(l2ca_event_obj.os_event), kEventAutoClear) != kStatus_OSA_Success) return osaStatus_Error; 


  /* Initialization of task message queue */
  MSG_InitQueue(&gHost_TaskQueue);
  MSG_InitQueue(&gL2ca_TaskQueue);

  /* Task creation */

  status = OSA_TaskCreate(Host_Task, "HOST_Task", HOST_TASK_STACK_SZ, HOST_stack, HOST_TASK_PRIO, (task_param_t)NULL, FALSE, &gHost_TaskId);
  if (kStatus_OSA_Success != status)
  {
    Panic(0, 0, 0, 0);
    return osaStatus_Error;
  }

  status = OSA_TaskCreate(L2ca_Task, "L2CA_Task", L2CA_TASK_STACK_SZ, L2CA_stack, L2CA_TASK_PRIO, (task_param_t)NULL, FALSE, &gL2ca_TaskId);
  if (kStatus_OSA_Success != status)
  {
    Panic(0, 0, 0, 0);
    return osaStatus_Error;
  }

  return osaStatus_Success;
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

static void Host_Task(task_param_t argument)
{
  Host_TaskHandler((void *)NULL);
}

static void L2ca_Task(task_param_t argument)
{
  L2ca_TaskHandler((void *)NULL);
}

/*! *********************************************************************************
* @}
********************************************************************************** */
