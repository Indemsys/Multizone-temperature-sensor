/*!
* Copyright (c) 2014, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file ApplMain.c
* This is a source file for the main application.
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

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
/* Drv */
//#include "LED.h"
//#include "Keyboard.h"
#include "KW4xXcvrDrv.h"

/* Fwk */
#include "MemManager.h"
#include "TimersManager.h"
#include "RNG_Interface.h"
#include "Messaging.h"
#include "Flash_Adapter.h"
#include "SecLib.h"
#include "Panic.h"

#if gFsciIncluded_c
  #include "FsciInterface.h"
  #include "FsciCommands.h"
#endif

/* KSDK */
#include "fsl_device_registers.h"

/* Bluetooth Low Energy */
#include "gatt_interface.h"
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gap_interface.h"
#include "l2ca_cb_interface.h"
#include "l2ca_interface.h"

#ifdef USE_POWER_DOWN_MODE
  #if (USE_POWER_DOWN_MODE)
    #include "PWR_Interface.h"
  #endif
#endif
#include "BLE_common_app.h"
#include "ble_controller_task_config.h"
#include "controller_interface.h"

#include "App.h"



/************************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
************************************************************************************/
#if (USE_POWER_DOWN_MODE)

static void App_Idle_Task(task_param_t argument);
#endif



static void App_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t *pConnectionEvent);
static void App_AdvertisingCallback(gapAdvertisingEvent_t *pAdvertisingEvent);
static void App_ScanningCallback(gapScanningEvent_t *pAdvertisingEvent);
static void App_GattServerCallback(deviceId_t peerDeviceId, gattServerEvent_t *pServerEvent);
static void App_GattClientProcedureCallback
(
  deviceId_t              deviceId,
  gattProcedureType_t     procedureType,
  gattProcedureResult_t   procedureResult,
  bleResult_t             error
  );
static void App_GattClientNotificationCallback
(
  deviceId_t      deviceId,
  uint16_t        characteristicValueHandle,
  uint8_t *aValue,
  uint16_t        valueLength
  );
static void App_GattClientIndicationCallback
(
  deviceId_t      deviceId,
  uint16_t        characteristicValueHandle,
  uint8_t *aValue,
  uint16_t        valueLength
  );

static void App_L2caLeDataCallback
(
  deviceId_t deviceId,
  uint16_t   lePsm,
  uint8_t *pPacket,
  uint16_t packetLength
  );

static void App_L2caLeControlCallback
(
  l2capControlMessageType_t  messageType,
  void *pMessage
  );


/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/
#if USE_POWER_DOWN_MODE
OSA_TASK_DEFINE(IDLE, IDLE_TASK_STACK_SZ);
task_handler_t                          gAppIdleTaskId            = 0;
#endif  /* cPWR_UsePowerDownMode */

extern event_t                                 app_event_struct;
extern anchor_t                                main_task_queue;



static gapAdvertisingCallback_t         pfAdvCallback             = NULL;
static gapScanningCallback_t            pfScanCallback            = NULL;
static gapConnectionCallback_t          pfConnCallback            = NULL;
static gattServerCallback_t             pfGattServerCallback      = NULL;
static gattClientProcedureCallback_t    pfGattClientProcCallback  = NULL;
static gattClientNotificationCallback_t pfGattClientNotifCallback = NULL;
static gattClientNotificationCallback_t pfGattClientIndCallback   = NULL;
static l2caLeCbDataCallback_t           pfL2caLeCbDataCallback    = NULL;
static l2caLeCbControlCallback_t        pfL2caLeCbControlCallback = NULL;


#if gRNG_HWSupport_d == gRNG_NoHWSupport_d
extern uint32_t mRandomNumber;
#endif


#if gUseHciTransportUpward_d
  #define BleApp_GenericCallback(param)
#else
extern void BleApp_GenericCallback(gapGenericEvent_t *pGenericEvent);
#endif

extern osaStatus_t Ble_HostTaskInit(void);
/*-----------------------------------------------------------------------------------------------------

 \param gapGenericCallback

 \return bleResult_t
-----------------------------------------------------------------------------------------------------*/
bleResult_t Ble_Initialize(gapGenericCallback_t gapGenericCallback)
{
  bleResult_t        res;
  #if (gUseHciTransportDownward_d == 1)

  /* Configure HCI Transport */
  hcitConfigStruct_t hcitConfigStruct =
  {
    .interfaceType = gHcitInterfaceType_d,
    .interfaceChannel = gHcitInterfaceNumber_d,
    .interfaceBaudrate = gHcitInterfaceSpeed_d,
    .transportInterface =  Ble_HciRecv
  };

  /* HCI Transport Init */
  if (gHciSuccess_c != Hcit_Init(&hcitConfigStruct)) return gHciTransportError_c;

  /* BLE Host Tasks Init */
  if (osaStatus_Success != Ble_HostTaskInit()) return gBleOsError_c;

  res = Ble_HostInitialize(gapGenericCallback, (hciHostToControllerInterface_t)Hcit_SendPacket);

  return res;

  #elif (gUseHciTransportUpward_d == 1)

  if (osaStatus_Success != Controller_TaskInit()) return gBleOsError_c;
  /* BLE Controller Init */
  if (osaStatus_Success != Controller_Init((gHostRecvCallback_t)Hcit_SendPacket)) return gBleOsError_c;
  /* Configure HCI Transport */
  hcitConfigStruct_t hcitConfigStruct =
  {
    .interfaceType = gHcitInterfaceType_d,
    .interfaceChannel = gHcitInterfaceNumber_d,
    .interfaceBaudrate = gHcitInterfaceSpeed_d,
    .transportInterface =  Controller_RecvPacket
  };

  return Hcit_Init(&hcitConfigStruct);

  #else
  /* BLE Controller Task Init */
  if (osaStatus_Success != Controller_TaskInit()) return gBleOsError_c;

  /* BLE Controller Init */
  if (osaStatus_Success != Controller_Init(Ble_HciRecv)) return gBleOsError_c;

  /* BLE Host Tasks Init */
  if (osaStatus_Success != Ble_HostTaskInit()) return gBleOsError_c;

  /* BLE Host Stack Init */
  res = Ble_HostInitialize(gapGenericCallback, (hciHostToControllerInterface_t)Controller_RecvPacket);
  return res;

  #endif
}





/*------------------------------------------------------------------------------



 \param advertisingCallback
 \param connectionCallback
 ------------------------------------------------------------------------------*/
bleResult_t App_StartAdvertising(gapAdvertisingCallback_t    advertisingCallback, gapConnectionCallback_t     connectionCallback)
{
  pfAdvCallback = advertisingCallback;
  pfConnCallback = connectionCallback;

  return Gap_StartAdvertising(App_AdvertisingCallback, App_ConnectionCallback);
}

/*------------------------------------------------------------------------------



 \param pScanningParameters
 \param scanningCallback
 ------------------------------------------------------------------------------*/
bleResult_t App_StartScanning(gapScanningParameters_t *pScanningParameters, gapScanningCallback_t       scanningCallback)
{
  pfScanCallback = scanningCallback;

  return Gap_StartScanning(pScanningParameters, App_ScanningCallback);
}

/*------------------------------------------------------------------------------



 \param serverCallback
 ------------------------------------------------------------------------------*/
bleResult_t App_RegisterGattServerCallback(gattServerCallback_t  serverCallback)
{
  pfGattServerCallback = serverCallback;

  return GattServer_RegisterCallback(App_GattServerCallback);
}

/*------------------------------------------------------------------------------



 \param callback
 ------------------------------------------------------------------------------*/
bleResult_t App_RegisterGattClientProcedureCallback(gattClientProcedureCallback_t  callback)
{
  pfGattClientProcCallback = callback;

  return GattClient_RegisterProcedureCallback(App_GattClientProcedureCallback);
}

/*------------------------------------------------------------------------------



 \param callback
 ------------------------------------------------------------------------------*/
bleResult_t App_RegisterGattClientNotificationCallback(gattClientNotificationCallback_t  callback)
{
  pfGattClientNotifCallback = callback;

  return GattClient_RegisterNotificationCallback(App_GattClientNotificationCallback);
}

/*------------------------------------------------------------------------------



 \param callback
 ------------------------------------------------------------------------------*/
bleResult_t App_RegisterGattClientIndicationCallback(gattClientIndicationCallback_t  callback)
{
  pfGattClientIndCallback = callback;

  return GattClient_RegisterIndicationCallback(App_GattClientIndicationCallback);
}

/*------------------------------------------------------------------------------



 \param lePsm
 \param pCallback
 \param pCtrlCallback
 \param lePsmMtu
 ------------------------------------------------------------------------------*/
bleResult_t App_RegisterLePsm(uint16_t lePsm, l2caLeCbDataCallback_t pCallback, l2caLeCbControlCallback_t pCtrlCallback, uint16_t lePsmMtu)
{
  pfL2caLeCbDataCallback = pCallback;
  pfL2caLeCbControlCallback = pCtrlCallback;
  return L2ca_RegisterLePsm(lePsm, App_L2caLeDataCallback, App_L2caLeControlCallback, lePsmMtu);
}

/*------------------------------------------------------------------------------



 \param pCallback
 ------------------------------------------------------------------------------*/
bleResult_t App_RegisterL2caControlCallback(l2caControlCallback_t pCallback)
{
  pfL2caLeCbControlCallback = pCallback;
  return L2ca_RegisterControlCallback(App_L2caLeControlCallback);
}



/*****************************************************************************
* Handles all messages received from the host task.
* Interface assumptions: None
* Return value: None
*****************************************************************************/
void App_HandleHostMessageInput(appMsgFromHost_t *pMsg)
{

  switch (pMsg->msgType)
  {
  case gAppGapGenericMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppGapGenericMsg_c\r\n");

      BleApp_GenericCallback(&pMsg->msgData.genericMsg);
      break;
    }
  case gAppGapAdvertisementMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppGapAdvertisementMsg_c\r\n");

      if (pfAdvCallback) pfAdvCallback(&pMsg->msgData.advMsg);
      break;
    }
  case gAppGapScanMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppGapScanMsg_c\r\n");

      if (pfScanCallback) pfScanCallback(&pMsg->msgData.scanMsg);
      break;
    }
  case gAppGapConnectionMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppGapConnectionMsg_c\r\n");

      if (pfConnCallback) pfConnCallback(pMsg->msgData.connMsg.deviceId, &pMsg->msgData.connMsg.connEvent);
      break;
    }
  case gAppGattServerMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppGattServerMsg_c\r\n");

      if (pfGattServerCallback) pfGattServerCallback(pMsg->msgData.gattServerMsg.deviceId, &pMsg->msgData.gattServerMsg.serverEvent);
      break;
    }
  case gAppGattClientProcedureMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppGattClientProcedureMsg_c\r\n");

      if (pfGattClientProcCallback) pfGattClientProcCallback(pMsg->msgData.gattClientProcMsg.deviceId,
                                                             pMsg->msgData.gattClientProcMsg.procedureType,
                                                             pMsg->msgData.gattClientProcMsg.procedureResult,
                                                             pMsg->msgData.gattClientProcMsg.error);
      break;
    }
  case gAppGattClientNotificationMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppGattClientNotificationMsg_c\r\n");

      if (pfGattClientNotifCallback) pfGattClientNotifCallback(
          pMsg->msgData.gattClientNotifIndMsg.deviceId,
          pMsg->msgData.gattClientNotifIndMsg.characteristicValueHandle,
          pMsg->msgData.gattClientNotifIndMsg.aValue,
          pMsg->msgData.gattClientNotifIndMsg.valueLength);
      break;
    }
  case gAppGattClientIndicationMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppGattClientIndicationMsg_c\r\n");

      if (pfGattClientIndCallback) pfGattClientIndCallback(
          pMsg->msgData.gattClientNotifIndMsg.deviceId,
          pMsg->msgData.gattClientNotifIndMsg.characteristicValueHandle,
          pMsg->msgData.gattClientNotifIndMsg.aValue,
          pMsg->msgData.gattClientNotifIndMsg.valueLength);
      break;
    }
  case gAppL2caLeDataMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppL2caLeDataMsg_c\r\n");

      if (pfL2caLeCbDataCallback) pfL2caLeCbDataCallback(
          pMsg->msgData.l2caLeCbDataMsg.deviceId,
          pMsg->msgData.l2caLeCbDataMsg.lePsm,
          pMsg->msgData.l2caLeCbDataMsg.aPacket,
          pMsg->msgData.l2caLeCbDataMsg.packetLength);
      break;
    }
  case gAppL2caLeControlMsg_c:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - gAppL2caLeControlMsg_c\r\n");

      if (pfL2caLeCbControlCallback) pfL2caLeCbControlCallback(
          pMsg->msgData.l2caLeCbControlMsg.messageType,
          pMsg->msgData.l2caLeCbControlMsg.aMessage);
      break;
    }
  default:
    {
      DEBUG_PRINT(".. -> App_HandleHostMessageInput - default\r\n");
      break;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------

 \param eventType

 \return char*
-----------------------------------------------------------------------------------------------------*/
static const char* Print_GenericEventType(gapGenericEventType_t  eventType)
{
  switch (eventType)
  {
  case gInitializationComplete_c:
    return "gInitializationComplete_c";
  case gInternalError_c:
    return "gInternalError_c";
  case gAdvertisingSetupFailed_c:
    return "gAdvertisingSetupFailed_c";
  case gAdvertisingParametersSetupComplete_c:
    return "gAdvertisingParametersSetupComplete_c";
  case gAdvertisingDataSetupComplete_c:
    return "gAdvertisingDataSetupComplete_c";
  case gWhiteListSizeRead_c:
    return "gWhiteListSizeRead_c";
  case gDeviceAddedToWhiteList_c:
    return "gDeviceAddedToWhiteList_c";
  case gDeviceRemovedFromWhiteList_c:
    return "gDeviceRemovedFromWhiteList_c";
  case gWhiteListCleared_c:
    return "gWhiteListCleared_c";
  case gRandomAddressReady_c:
    return "gRandomAddressReady_c";
  case gCreateConnectionCanceled_c:
    return "gCreateConnectionCanceled_c";
  case gPublicAddressRead_c:
    return "gPublicAddressRead_c";
  case gAdvTxPowerLevelRead_c:
    return "gAdvTxPowerLevelRead_c";
  case gPrivateResolvableAddressVerified_c:
    return "gPrivateResolvableAddressVerified_c";
  case gRandomAddressSet_c:
    return "gRandomAddressSet_c";
  }
  return "???";
}

/*------------------------------------------------------------------------------



 \param pGenericEvent
 ------------------------------------------------------------------------------*/
void App_GenericCallback(gapGenericEvent_t *pGenericEvent)
{
  appMsgFromHost_t *pMsgIn = NULL;

  DEBUG_CALLBACK_PRINT_ARG("~~~~~ BLE Host generic event: %s\r\n", Print_GenericEventType(pGenericEvent->eventType));

  pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gapGenericEvent_t));

  if (!pMsgIn) return;

  pMsgIn->msgType = gAppGapGenericMsg_c;
  FLib_MemCpy(&pMsgIn->msgData.genericMsg, pGenericEvent, sizeof(gapGenericEvent_t));

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param peerDeviceId
 \param pConnectionEvent
 ------------------------------------------------------------------------------*/
static void App_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t *pConnectionEvent)
{
  appMsgFromHost_t *pMsgIn = NULL;
  uint8_t          msgLen  = GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gapConnectionEvent_t);

  DEBUG_CALLBACK_PRINT_ARG("~~~~~ Connection callback from dev %d. Event=%d\r\n", peerDeviceId, pConnectionEvent->eventType);


  if (pConnectionEvent->eventType == gConnEvtKeysReceived_c)
  {
    gapSmpKeys_t    *pKeys = pConnectionEvent->eventData.keysReceivedEvent.pKeys;

    /* Take into account alignment */
    msgLen = GetRelAddr(appMsgFromHost_t, msgData) + GetRelAddr(connectionMsg_t, connEvent) +
             GetRelAddr(gapConnectionEvent_t, eventData) + sizeof(gapKeysReceivedEvent_t) + sizeof(gapSmpKeys_t);

    if (pKeys->aLtk != NULL)
    {
      msgLen += 2 * sizeof(uint8_t) + pKeys->cLtkSize + pKeys->cRandSize;
    }

    msgLen += (pKeys->aIrk != NULL) ? (gcSmpIrkSize_c + gcBleDeviceAddressSize_c) : 0;
    msgLen += (pKeys->aCsrk != NULL) ? gcSmpCsrkSize_c : 0;
  }

  pMsgIn = MSG_Alloc(msgLen);

  if (!pMsgIn) return;

  pMsgIn->msgType = gAppGapConnectionMsg_c;
  pMsgIn->msgData.connMsg.deviceId = peerDeviceId;

  if (pConnectionEvent->eventType == gConnEvtKeysReceived_c)
  {
    gapSmpKeys_t *pKeys   = pConnectionEvent->eventData.keysReceivedEvent.pKeys;
    uint8_t      *pCursor = (uint8_t *)&pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys;

    pMsgIn->msgData.connMsg.connEvent.eventType = gConnEvtKeysReceived_c;
    pCursor += sizeof(void *);
    pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys = (gapSmpKeys_t *)pCursor;

    /* Copy SMP Keys structure */
    FLib_MemCpy(pCursor, pConnectionEvent->eventData.keysReceivedEvent.pKeys, sizeof(gapSmpKeys_t));
    pCursor += sizeof(gapSmpKeys_t);

    if (pKeys->aLtk != NULL)
    {
      /* Copy LTK */
      pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->cLtkSize = pKeys->cLtkSize;
      pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aLtk = pCursor;
      FLib_MemCpy(pCursor, pKeys->aLtk, pKeys->cLtkSize);
      pCursor += pKeys->cLtkSize;

      /* Copy RAND */
      pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->cRandSize = pKeys->cRandSize;
      pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aRand = pCursor;
      FLib_MemCpy(pCursor, pKeys->aRand, pKeys->cRandSize);
      pCursor += pKeys->cRandSize;
    }
    else if (pKeys->aIrk != NULL)
    {
      /* Copy IRK */
      pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aIrk = pCursor;
      FLib_MemCpy(pCursor, pKeys->aIrk, gcSmpIrkSize_c);
      pCursor += gcSmpIrkSize_c;

      /* Copy Address*/
      pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->addressType = pKeys->addressType;
      pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aAddress = pCursor;
      FLib_MemCpy(pCursor, pKeys->aAddress, gcBleDeviceAddressSize_c);
      pCursor += gcBleDeviceAddressSize_c;
    }

    if (pKeys->aCsrk != NULL)
    {
      /* Copy CSRK */
      pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aCsrk = pCursor;
      FLib_MemCpy(pCursor, pKeys->aCsrk, gcSmpCsrkSize_c);
    }
  }
  else
  {
    FLib_MemCpy(&pMsgIn->msgData.connMsg.connEvent, pConnectionEvent, sizeof(gapConnectionEvent_t));
  }

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param pAdvertisingEvent
 ------------------------------------------------------------------------------*/
static void App_AdvertisingCallback(gapAdvertisingEvent_t *pAdvertisingEvent)
{
  appMsgFromHost_t *pMsgIn = NULL;

  DEBUG_CALLBACK_PRINT("~~~~~ Advertising Callback\r\n");


  pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gapAdvertisingEvent_t));

  if (!pMsgIn) return;

  pMsgIn->msgType = gAppGapAdvertisementMsg_c;
  pMsgIn->msgData.advMsg.eventType = pAdvertisingEvent->eventType;
  pMsgIn->msgData.advMsg.eventData = pAdvertisingEvent->eventData;

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param pScanningEvent
 ------------------------------------------------------------------------------*/
static void App_ScanningCallback(gapScanningEvent_t *pScanningEvent)
{
  appMsgFromHost_t *pMsgIn = NULL;

  uint8_t          msgLen  = GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gapScanningEvent_t);

  DEBUG_CALLBACK_PRINT("~~~~~ Scanning Callback\r\n");


  if (pScanningEvent->eventType == gDeviceScanned_c)
  {
    msgLen += pScanningEvent->eventData.scannedDevice.dataLength;
  }

  pMsgIn = MSG_Alloc(msgLen);

  if (!pMsgIn) return;

  pMsgIn->msgType = gAppGapScanMsg_c;
  pMsgIn->msgData.scanMsg.eventType = pScanningEvent->eventType;

  if (pScanningEvent->eventType == gScanCommandFailed_c)
  {
    pMsgIn->msgData.scanMsg.eventData.failReason = pScanningEvent->eventData.failReason;
  }

  if (pScanningEvent->eventType == gDeviceScanned_c)
  {
    FLib_MemCpy(&pMsgIn->msgData.scanMsg.eventData.scannedDevice,
                &pScanningEvent->eventData.scannedDevice,
                sizeof(gapScanningEvent_t));

    /* Copy data after the gapScanningEvent_t structure and update the data pointer*/
    pMsgIn->msgData.scanMsg.eventData.scannedDevice.data = (uint8_t *)&pMsgIn->msgData + sizeof(gapScanningEvent_t);
    FLib_MemCpy(pMsgIn->msgData.scanMsg.eventData.scannedDevice.data,
                pScanningEvent->eventData.scannedDevice.data,
                pScanningEvent->eventData.scannedDevice.dataLength);
  }

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param deviceId
 \param pServerEvent
 ------------------------------------------------------------------------------*/
static void App_GattServerCallback(deviceId_t deviceId, gattServerEvent_t *pServerEvent)
{
  appMsgFromHost_t *pMsgIn = NULL;
  uint8_t          msgLen  = GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gattServerMsg_t);

  DEBUG_CALLBACK_PRINT_ARG("~~~~~ GATT Server Callback from devise %d. Event=%d\r\n",deviceId,pServerEvent->eventType);


  if (pServerEvent->eventType == gEvtAttributeWritten_c || pServerEvent->eventType == gEvtAttributeWrittenWithoutResponse_c)
  {
    msgLen += pServerEvent->eventData.attributeWrittenEvent.cValueLength;
  }

  pMsgIn = MSG_Alloc(msgLen);

  if (!pMsgIn) return;

  pMsgIn->msgType = gAppGattServerMsg_c;
  pMsgIn->msgData.gattServerMsg.deviceId = deviceId;
  FLib_MemCpy(&pMsgIn->msgData.gattServerMsg.serverEvent, pServerEvent, sizeof(gattServerEvent_t));

  if ((pMsgIn->msgData.gattServerMsg.serverEvent.eventType == gEvtAttributeWritten_c) || (pMsgIn->msgData.gattServerMsg.serverEvent.eventType == gEvtAttributeWrittenWithoutResponse_c))
  {
    /* Copy value after the gattServerEvent_t structure and update the aValue pointer*/
    pMsgIn->msgData.gattServerMsg.serverEvent.eventData.attributeWrittenEvent.aValue = (uint8_t *)&pMsgIn->msgData.gattServerMsg.serverEvent.eventData.attributeWrittenEvent.aValue + sizeof(uint8_t *);
    FLib_MemCpy(pMsgIn->msgData.gattServerMsg.serverEvent.eventData.attributeWrittenEvent.aValue, pServerEvent->eventData.attributeWrittenEvent.aValue, pServerEvent->eventData.attributeWrittenEvent.cValueLength);

  }

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param deviceId
 \param procedureType
 \param procedureResult
 \param error
 ------------------------------------------------------------------------------*/
static void App_GattClientProcedureCallback(deviceId_t deviceId, gattProcedureType_t procedureType, gattProcedureResult_t   procedureResult, bleResult_t error)
{
  appMsgFromHost_t *pMsgIn = NULL;

  DEBUG_CALLBACK_PRINT_ARG("~~~~~ GATT Client Procedure Callback from device %d\r\n",deviceId);


  pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gattClientProcMsg_t));

  if (!pMsgIn) return;

  pMsgIn->msgType = gAppGattClientProcedureMsg_c;
  pMsgIn->msgData.gattClientProcMsg.deviceId = deviceId;
  pMsgIn->msgData.gattClientProcMsg.procedureType = procedureType;
  pMsgIn->msgData.gattClientProcMsg.error = error;
  pMsgIn->msgData.gattClientProcMsg.procedureResult = procedureResult;

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param deviceId
 \param characteristicValueHandle
 \param aValue
 \param valueLength
 ------------------------------------------------------------------------------*/
static void App_GattClientNotificationCallback(deviceId_t deviceId, uint16_t characteristicValueHandle, uint8_t *aValue, uint16_t valueLength)
{
  appMsgFromHost_t *pMsgIn = NULL;

  DEBUG_CALLBACK_PRINT_ARG("~~~~~ GATT Client Notification Callback from device %d\r\n", deviceId);

  /* Allocate a buffer with enough space to store also the notified value*/
  pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gattClientNotifIndMsg_t)
                     + valueLength);

  if (!pMsgIn) return;

  pMsgIn->msgType = gAppGattClientNotificationMsg_c;
  pMsgIn->msgData.gattClientNotifIndMsg.deviceId = deviceId;
  pMsgIn->msgData.gattClientNotifIndMsg.characteristicValueHandle = characteristicValueHandle;
  pMsgIn->msgData.gattClientNotifIndMsg.valueLength = valueLength;

  /* Copy value after the gattClientNotifIndMsg_t structure and update the aValue pointer*/
  pMsgIn->msgData.gattClientNotifIndMsg.aValue = (uint8_t *)&pMsgIn->msgData + sizeof(gattClientNotifIndMsg_t);
  FLib_MemCpy(pMsgIn->msgData.gattClientNotifIndMsg.aValue, aValue, valueLength);

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param deviceId
 \param characteristicValueHandle
 \param aValue
 \param valueLength
 ------------------------------------------------------------------------------*/
static void App_GattClientIndicationCallback(deviceId_t deviceId, uint16_t characteristicValueHandle, uint8_t *aValue, uint16_t valueLength)
{
  appMsgFromHost_t *pMsgIn = NULL;

  DEBUG_CALLBACK_PRINT_ARG("~~~~~ GATT Client Indication Callback from device %d\r\n",deviceId);


  /* Allocate a buffer with enough space to store also the notified value*/
  pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gattClientNotifIndMsg_t)
                     + valueLength);

  if (!pMsgIn) return;

  pMsgIn->msgType = gAppGattClientIndicationMsg_c;
  pMsgIn->msgData.gattClientNotifIndMsg.deviceId = deviceId;
  pMsgIn->msgData.gattClientNotifIndMsg.characteristicValueHandle = characteristicValueHandle;
  pMsgIn->msgData.gattClientNotifIndMsg.valueLength = valueLength;

  /* Copy value after the gattClientIndIndMsg_t structure and update the aValue pointer*/
  pMsgIn->msgData.gattClientNotifIndMsg.aValue = (uint8_t *)&pMsgIn->msgData + sizeof(gattClientNotifIndMsg_t);
  FLib_MemCpy(pMsgIn->msgData.gattClientNotifIndMsg.aValue, aValue, valueLength);

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param deviceId
 \param lePsm
 \param pPacket
 \param packetLength
 ------------------------------------------------------------------------------*/
static void App_L2caLeDataCallback(deviceId_t deviceId, uint16_t lePsm, uint8_t *pPacket, uint16_t packetLength)
{
  appMsgFromHost_t *pMsgIn = NULL;

  DEBUG_CALLBACK_PRINT_ARG("~~~~~ L2ca Le Data Callback from device %d\r\n",deviceId);

  /* Allocate a buffer with enough space to store the packet */
  pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(l2caLeCbDataMsg_t) + packetLength);

  if (!pMsgIn)
  {
    return;
  }

  pMsgIn->msgType = gAppL2caLeDataMsg_c;
  pMsgIn->msgData.l2caLeCbDataMsg.deviceId = deviceId;
  pMsgIn->msgData.l2caLeCbDataMsg.lePsm = lePsm;
  pMsgIn->msgData.l2caLeCbDataMsg.packetLength = packetLength;

  FLib_MemCpy(pMsgIn->msgData.l2caLeCbDataMsg.aPacket, pPacket, packetLength);

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}

/*------------------------------------------------------------------------------



 \param messageType
 \param pMessage
 ------------------------------------------------------------------------------*/
static void App_L2caLeControlCallback(l2capControlMessageType_t messageType, void *pMessage)
{
  appMsgFromHost_t *pMsgIn       = NULL;
  uint8_t          messageLength = 0;

  DEBUG_CALLBACK_PRINT("~~~~~ L2ca Le Control Callback\r\n");

  switch (messageType)
  {
  case gL2ca_LePsmConnectRequest_c:
    {
      messageLength = sizeof(l2caLeCbConnectionRequest_t);
      break;
    }
  case gL2ca_LePsmConnectionComplete_c:
    {
      messageLength = sizeof(l2caLeCbConnectionComplete_t);
      break;
    }
  case gL2ca_LePsmDisconnectNotification_c:
    {
      messageLength = sizeof(l2caLeCbDisconnection_t);
      break;
    }
  case gL2ca_ConnectionParameterUpdateRequest_c:
    {
      messageLength = sizeof(l2caLeCbConnectionParameterUpdateRequest_t);
      break;
    }
  case gL2ca_ConnectionParameterUpdateComplete_c:
    {
      messageLength = sizeof(l2caLeCbConnectionParameterUpdateComplete_t);
      break;
    }
  case gL2ca_NoPeerCredits_c:
    {
      messageLength = sizeof(l2caLeCbNoPeerCredits_t);
      break;
    }
  case gL2ca_LocalCreditsNotification_c:
    {
      messageLength = sizeof(l2caLeCbLocalCreditsNotification_t);
      break;
    }
  default:
    return;
  }


  /* Allocate a buffer with enough space to store the biggest packet */
  pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(l2caLeCbConnectionParameterUpdateRequest_t));

  if (!pMsgIn)
  {
    return;
  }

  pMsgIn->msgType = gAppL2caLeControlMsg_c;
  pMsgIn->msgData.l2caLeCbControlMsg.messageType = messageType;

  FLib_MemCpy(pMsgIn->msgData.l2caLeCbControlMsg.aMessage, pMessage, messageLength);

  /* Put message in the Host Stack to App queue */
  MSG_Queue(&main_task_queue, pMsgIn);

  /* Signal application */
  OSA_EventSet(&app_event_struct, EVENT_FROM_HOST_STACK);
}








