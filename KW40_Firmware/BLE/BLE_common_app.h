#ifndef _APPL_MAIN_H_
  #define _APPL_MAIN_H_

  #include "EmbeddedTypes.h"
  #include "fsl_os_abstraction.h"
  #include "l2ca_cb_interface.h"
  #include "gap_types.h"
  #include "gatt_client_interface.h"
  #include "gatt_server_interface.h"



bleResult_t App_StartAdvertising(gapAdvertisingCallback_t    advertisingCallback, gapConnectionCallback_t     connectionCallback);
bleResult_t App_StartScanning(gapScanningParameters_t *pScanningParameters, gapScanningCallback_t       scanningCallback);
bleResult_t App_RegisterGattClientNotificationCallback(gattClientNotificationCallback_t  callback);
bleResult_t App_RegisterGattClientIndicationCallback(gattClientIndicationCallback_t  callback);
bleResult_t App_RegisterGattServerCallback(gattServerCallback_t  serverCallback);
bleResult_t App_RegisterGattClientProcedureCallback(gattClientProcedureCallback_t  callback);
bleResult_t App_RegisterLePsm(uint16_t lePsm, l2caLeCbDataCallback_t pCallback, l2caLeCbControlCallback_t   pCtrlCallback, uint16_t lePsmMtu);
bleResult_t App_RegisterL2caControlCallback(l2caControlCallback_t pCallback);

/* Host to Application Messages Types */
typedef enum {
  gAppGapGenericMsg_c = 0,
  gAppGapConnectionMsg_c,
  gAppGapAdvertisementMsg_c,
  gAppGapScanMsg_c,
  gAppGattServerMsg_c,
  gAppGattClientProcedureMsg_c,
  gAppGattClientNotificationMsg_c,
  gAppGattClientIndicationMsg_c,
  gAppL2caLeDataMsg_c,
  gAppL2caLeControlMsg_c,
}appHostMsgType_tag;

typedef uint8_t appHostMsgType_t;

/* Host to Application Connection Message */
typedef struct connectionMsg_tag {
  deviceId_t              deviceId;
  gapConnectionEvent_t    connEvent;
}connectionMsg_t;

/* Host to Application GATT Server Message */
typedef struct gattServerMsg_tag {
  deviceId_t          deviceId;
  gattServerEvent_t   serverEvent;
}gattServerMsg_t;

/* Host to Application GATT Client Procedure Message */
typedef struct gattClientProcMsg_tag {
  deviceId_t              deviceId;
  gattProcedureType_t     procedureType;
  gattProcedureResult_t   procedureResult;
  bleResult_t             error;
}gattClientProcMsg_t;

/* Host to Application GATT Client Notification/Indication Message */
typedef struct gattClientNotifIndMsg_tag {
  uint8_t *aValue;
  uint16_t    characteristicValueHandle;
  uint16_t    valueLength;
  deviceId_t  deviceId;
}gattClientNotifIndMsg_t;

/* L2ca to Application Data Message */
typedef struct l2caLeCbDataMsg_tag {
  deviceId_t  deviceId;
  uint16_t    lePsm;
  uint16_t    packetLength;
  uint8_t     aPacket[0];
}l2caLeCbDataMsg_t;

/* L2ca to Application Control Message */
typedef struct l2caLeCbControlMsg_tag {
  l2capControlMessageType_t   messageType;
  uint16_t                    padding;
  uint8_t                     aMessage[0];
}l2caLeCbControlMsg_t;


typedef struct appMsgFromHost_tag {
  appHostMsgType_t    msgType;
  union
  {
    gapGenericEvent_t       genericMsg;
    gapAdvertisingEvent_t   advMsg;
    connectionMsg_t         connMsg;
    gapScanningEvent_t      scanMsg;
    gattServerMsg_t         gattServerMsg;
    gattClientProcMsg_t     gattClientProcMsg;
    gattClientNotifIndMsg_t gattClientNotifIndMsg;
    l2caLeCbDataMsg_t       l2caLeCbDataMsg;
    l2caLeCbControlMsg_t    l2caLeCbControlMsg;
  } msgData;
}appMsgFromHost_t;


void App_HandleHostMessageInput(appMsgFromHost_t *pMsg);
void App_GenericCallback(gapGenericEvent_t *pGenericEvent);
/*------------------------------------------------------------------------------
 \brief  Performs full initialization of the BLE stack.

 \param[in] genericCallback  Callback used by the Host Stack to propagate GAP generic
 events to the application.

 \return  gBleSuccess_c or error.

 \remarks The gInitializationComplete_c generic GAP event is triggered on completion.
 ------------------------------------------------------------------------------*/
bleResult_t Ble_Initialize(gapGenericCallback_t gapGenericCallback);


#endif /* _APPL_MAIN_H_ */
