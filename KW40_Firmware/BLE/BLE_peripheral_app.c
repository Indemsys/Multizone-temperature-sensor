#include "App.h"


#define smpEdiv                  0x1F99
#define mcEncryptionKeySize_c    16


/* Scanning and Advertising Data */
static const uint8_t adData0[1] =  { (gapAdTypeFlags_t)(gLeGeneralDiscoverableMode_c | gBrEdrNotSupported_c)};


static gapAdStructure_t advScanStruct[3] =
{
  {
    .length = NUMBER_OF_ELEMENTS(adData0)+ 1,
    .adType = gAdFlags_c,
    .aData = (void *)adData0
  },
  {
    .length = NUMBER_OF_ELEMENTS(uuid_cmd_service)+ 1, // Объявление сервиса предоставляемого устройством. В данном случае это сервис комманд модуля K66BLEZ
    .adType = gAdComplete128bitServiceList_c,
    .aData = (void *)uuid_cmd_service
  },
  {
    .adType = gAdShortenedLocalName_c,
    .length = sizeof(ADVERTISED_DEVICE_NAME), // Длина ограничена 8-ю символами для этого поля - gAdShortenedLocalName_c.
    .aData = ADVERTISED_DEVICE_NAME
  },
};

gapAdvertisingData_t gAppAdvertisingData =
{
  NUMBER_OF_ELEMENTS(advScanStruct),
  (void *)advScanStruct
};

gapScanResponseData_t gAppScanRspData =
{
  0,
  NULL
};

/* SMP Data */
gapPairingParameters_t gPairingParameters = {
  TRUE,
  gSecurityMode_1_Level_3_c, // gSecurityMode_1_Level_3_c, Понизил уровень защиты !!!
  mcEncryptionKeySize_c,
  gIoDisplayOnly_c,
  TRUE,
  gIrk_c,
  gLtk_c
};

/* LTK */
static const uint8_t smpLtk[gcSmpMaxLtkSize_c] =
{ 0xD6, 0x93, 0xE8, 0xA4, 0x23, 0x55, 0x48, 0x99,
  0x1D, 0x77, 0x61, 0xE6, 0x63, 0x2B, 0x10, 0x8E };

/* RAND*/
static const uint8_t smpRand[gcSmpMaxRandSize_c] =
{ 0x26, 0x1E, 0xF6, 0x09, 0x97, 0x2E, 0xAD, 0x7E };

/* IRK */
static const uint8_t smpIrk[gcSmpIrkSize_c] =
{ 0x0A, 0x2D, 0xF4, 0x65, 0xE3, 0xBD, 0x7B, 0x49,
  0x1E, 0xB4, 0xC0, 0x95, 0x95, 0x13, 0x46, 0x73 };

/* CSRK */
static const uint8_t smpCsrk[gcSmpCsrkSize_c] =
{ 0x90, 0xD5, 0x06, 0x95, 0x92, 0xED, 0x91, 0xD7,
  0xA8, 0x9E, 0x2C, 0xDC, 0x4A, 0x93, 0x5B, 0xF9 };

gapSmpKeys_t gSmpKeys = {
  .cLtkSize = mcEncryptionKeySize_c,
  .aLtk = (void *)smpLtk,
  .aIrk = (void *)smpIrk,
  .aCsrk = (void *)smpCsrk,
  .aRand = (void *)smpRand,
  .cRandSize = gcSmpMaxRandSize_c,
  .ediv = smpEdiv,
};

/* Device Security Requirements */
static const gapSecurityRequirements_t        masterSecurity = gGapDefaultSecurityRequirements_d;

static const gapServiceSecurityRequirements_t serviceSecurity[2] =
{
  {
    .requirements = {
      .securityModeLevel = gSecurityMode_1_Level_3_c,
      .authorization = FALSE,
      .minimumEncryptionKeySize = gDefaultEncryptionKeySize_d
    },
    .serviceHandle = service_battery
  },
  {
    .requirements = {
      .securityModeLevel = gSecurityMode_1_Level_3_c,
      .authorization = FALSE,
      .minimumEncryptionKeySize = gDefaultEncryptionKeySize_d
    },
    .serviceHandle = service_device_info
  }
};

gapDeviceSecurityRequirements_t deviceSecurityRequirements =
{
  .pMasterSecurityRequirements    = (void *)&masterSecurity,
  .cNumServices                   = 3,
  .aServiceSecurityRequirements   = (void *)serviceSecurity
};

#define mBatteryLevelReportInterval_c   (1)        /* battery level report interval in seconds  */


typedef enum
{
#if gBondingSupported_d
  fastWhiteListAdvState_c,
#endif
  fastAdvState_c,
  slowAdvState_c
}T_advertising_type;

typedef struct advState_tag
{
    bool_t               advertising_active;  // Состояние оповещений
    T_advertising_type   advertising_type;
} T_advertising_state;


/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/

/* Host Stack Data*/
static bleDeviceAddress_t         maBleDeviceAddress;
static deviceId_t                 mPeerDeviceId = gInvalidDeviceId_c;

/* Adv Parmeters */
static T_advertising_state                 advertising_state;
static uint32_t                   mAdvTimeout;
static gapAdvertisingParameters_t advParams = gGapDefaultAdvertisingParameters_d;

#if gBondingSupported_d
static bleDeviceAddress_t         maPeerDeviceAddress;
static uint8_t                    mcBondedDevices = 0;
static bleAddressType_t           mPeerDeviceAddressType;
#endif

/* Service Data*/
static basConfig_t                basServiceConfig = { service_battery, 0 };
static disConfig_t                disServiceConfig = { service_device_info };
static hrsUserData_t              hrsUserData;


#ifdef K66BLEZ_PROFILE
// Массив индексов значения характеристик запись в которые вызовет генерацию событий gEvtAttributeWritten_c или gEvtAttributeWrittenWithoutResponse_c
static uint16_t                   k66blez_wr_handlers[4] = { value_k66wrdata_w_resp, value_k66wrdata, value_k66rddata, value_k66_cmd_write };
static uint16_t                   k66blez_rd_handlers[4] = {  value_k66wrdata_w_resp, value_k66wrdata, value_k66rddata, value_k66_cmd_read };


#endif




/* Application specific data*/
static bool_t                     mToggle16BitHeartRate = FALSE;
static bool_t                     mContactStatus = TRUE;
static tmrTimerID_t               advertising_timer_id;
static tmrTimerID_t               heart_rate_service_timer_id;
static tmrTimerID_t               battery_service_timer_id;


/************************************************************************************
*************************************************************************************
* Private functions prototypes
*************************************************************************************
************************************************************************************/

/* Gatt and Att callbacks */
static void BleApp_AdvertisingCallback(gapAdvertisingEvent_t *pAdvertisingEvent);
static void BleApp_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t *pConnectionEvent);
static void BleApp_GattServerCallback(deviceId_t deviceId, gattServerEvent_t *pServerEvent);
static void BleApp_Config(void);

/* Timer Callbacks */
static void AdvertisingTimerCallback(void *);
static void Timer_calback_for_heart_rate_service(void *);
static void Timer_callback_for_battery_service(void *);

static void BleApp_SetAdvertisingParameters(void);
static void GetRSSI(void);


/*------------------------------------------------------------------------------
  Старт процесса оповещения

  Вызывается на старте приложения и каждый раз после события разрыва соединения


 \param void
 ------------------------------------------------------------------------------*/
void BleApp_Restart_Advertising(void)
{
  Restart_perfomance_test();

  // Если еще не включен адвертайзинг, то включить его
  if (!advertising_state.advertising_active)
  {
#if gBondingSupported_d
    if (mcBondedDevices > 0)
    {
      advertising_state.advertising_type = fastWhiteListAdvState_c;
    }
    else
    {
#endif
      advertising_state.advertising_type = fastAdvState_c;
#if gBondingSupported_d
    }
#endif
    BleApp_SetAdvertisingParameters();
  }

#if (USE_POWER_DOWN_MODE)
  PWR_ChangeDeepSleepMode(1); /* MCU=LLS3, LL=DSM, wakeup on GPIO/LL */
  PWR_AllowDeviceToSleep();
#endif
}


/*! *********************************************************************************
* \brief        Handles BLE generic callback.
*
* \param[in]    pGenericEvent    Pointer to gapGenericEvent_t.
********************************************************************************** */
void BleApp_GenericCallback(gapGenericEvent_t *pGenericEvent)
{
  DEBUG_PRINT(".. -> .. -> Function: BleApp_GenericCallback\r\n");
  DEBUG_PRINT("  [\r\n");
  switch (pGenericEvent->eventType)
  {
  case gInitializationComplete_c:
    {
      DEBUG_PRINT("      Event: gInitializationComplete_c\r\n");
      BleApp_Config();
    }
    break;

  case gPublicAddressRead_c:
    {
      DEBUG_PRINT("      Event: gPublicAddressRead_c\r\n");
      /* Use address read from the controller */
      FLib_MemCpyReverseOrder(maBleDeviceAddress, pGenericEvent->eventData.aAddress, sizeof(bleDeviceAddress_t));
    }
    break;

  case gAdvertisingDataSetupComplete_c:
    {
      DEBUG_PRINT("      Event: gAdvertisingDataSetupComplete_c\r\n");
    }
    break;

  case gAdvertisingParametersSetupComplete_c:
    {
      DEBUG_PRINT("      Event: gAdvertisingParametersSetupComplete_c\r\n");
      App_StartAdvertising(BleApp_AdvertisingCallback, BleApp_ConnectionCallback);
    }
    break;

  case gInternalError_c:
    {
      DEBUG_PRINT("      Event: gInternalError_c\r\n");
      Panic(0, 0, 0, 0);
    }
    break;

  case gAdvertisingSetupFailed_c:
    DEBUG_PRINT("      Event: gAdvertisingSetupFailed_c\r\n");
    break;
  case gWhiteListSizeRead_c:
    DEBUG_PRINT("      Event: gWhiteListSizeRead_c\r\n");
    break;
  case gDeviceAddedToWhiteList_c:
    DEBUG_PRINT("      Event: gDeviceAddedToWhiteList_c\r\n");
    break;
  case gDeviceRemovedFromWhiteList_c:
    DEBUG_PRINT("      Event: gDeviceRemovedFromWhiteList_c\r\n");
    break;
  case gWhiteListCleared_c:
    DEBUG_PRINT("      Event: gWhiteListCleared_c\r\n");
    break;
  case gRandomAddressReady_c:
    DEBUG_PRINT("      Event: gRandomAddressReady_c\r\n");
    break;
  case gCreateConnectionCanceled_c:
    DEBUG_PRINT("      Event: gCreateConnectionCanceled_c\r\n");
    break;
  case gAdvTxPowerLevelRead_c:
    DEBUG_PRINT("      Event: gAdvTxPowerLevelRead_c\r\n");
    break;
  case gPrivateResolvableAddressVerified_c:
    DEBUG_PRINT("      Event: gPrivateResolvableAddressVerified_c\r\n");
    break;
  case gRandomAddressSet_c:
    DEBUG_PRINT("      Event: gRandomAddressSet_c\r\n");
    break;

  default:
    DEBUG_PRINT("      Event: default\r\n");
    break;
  }
  DEBUG_PRINT("  ]\r\n");
}

/*! *********************************************************************************
* \brief        Configures BLE Stack after initialization. Usually used for
*               configuring advertising, scanning, white list, services, et al.
*
********************************************************************************** */
static void BleApp_Config(void)
{
  bleResult_t res;

  DEBUG_PRINT("    [\r\n");
  DEBUG_PRINT("       BLE stack configuration after initialisation\r\n");
  /* Read public address from controller */

  res = Gap_ReadPublicDeviceAddress();
  DEBUG_PRINT_ARG("       - Read public device address (%d)\r\n", res);



#ifdef K66BLEZ_PROFILE
  res = GattServer_RegisterHandlesForWriteNotifications(NUMBER_OF_ELEMENTS(k66blez_wr_handlers), k66blez_wr_handlers);
  GattServer_RegisterHandlesForReadNotifications(NUMBER_OF_ELEMENTS(k66blez_rd_handlers), k66blez_rd_handlers);
#endif
  DEBUG_PRINT_ARG("       - Registering handlers for write and read notifications (%d)\r\n", res);



  res = App_RegisterGattServerCallback(BleApp_GattServerCallback);
  DEBUG_PRINT_ARG("       - Registering GATT server callback (%d)\r\n", res);

  /* Register security requirements */
#if gUseServiceSecurity_d
  res = Gap_RegisterDeviceSecurityRequirements(&deviceSecurityRequirements);
  DEBUG_PRINT_ARG("       - Registering device security requirememnts (%d)\r\n", res);
#endif

  /* Set local passkey */
#if gBondingSupported_d
  res = Gap_SetLocalPasskey(pin_code);
  DEBUG_PRINT_ARG("       - Set local passkey (%d)\r\n", res);
#endif

  /* Setup Advertising and scanning data */
  res = Gap_SetAdvertisingData(&gAppAdvertisingData,&gAppScanRspData);
  DEBUG_PRINT_ARG("       - Set advertising data (%d)\r\n", res);

  /* Populate White List if bonding is supported */
#if gBondingSupported_d
  bleDeviceAddress_t aBondedDevAdds[gcGapMaximumBondedDevices_d];
  res = Gap_GetBondedStaticAddresses(aBondedDevAdds, gcGapMaximumBondedDevices_d,&mcBondedDevices);
  DEBUG_PRINT_ARG("       - Get bonded static address (%d)\r\n", res);

  if (gBleSuccess_c == res && mcBondedDevices > 0)
  {
    for (uint8_t i = 0; i < mcBondedDevices; i++)
    {
      res = Gap_AddDeviceToWhiteList(gBleAddrTypePublic_c, aBondedDevAdds[i]);
      DEBUG_PRINT_ARG("       - Adding device to white list (%d)\r\n", res);
    }
  }
#endif

  advertising_state.advertising_active = FALSE;

  basServiceConfig.batteryLevel = 0; //  BOARD_GetBatteryLevel(); Передавать будем RSSI подключенного клиента
  res = Bas_Start(&basServiceConfig);
  DEBUG_PRINT_ARG("       - Battery level service start (%d)\r\n", res);

  /* Allocate application timers */
  advertising_timer_id = TMR_AllocateTimer();
  DEBUG_PRINT_ARG("       - Allocating advertisement timer id =%d\r\n", advertising_timer_id);

  battery_service_timer_id = TMR_AllocateTimer();
  DEBUG_PRINT_ARG("       - Allocating battery measurement service timer id = %d\r\n", battery_service_timer_id);

#if (USE_POWER_DOWN_MODE)
  PWR_ChangeDeepSleepMode(3); /* MCU=LLS3, LL=IDLE, wakeup on GPIO/LL */
  PWR_AllowDeviceToSleep();
#endif

  DEBUG_PRINT("    ]\r\n");
}

/*! *********************************************************************************
* \brief        Configures GAP Advertise parameters. Advertise will satrt after
*               the parameters are set.
*
********************************************************************************** */
static void BleApp_SetAdvertisingParameters(void)
{
  DEBUG_PRINT("Set Advertising Parameters.\r\n");
  switch (advertising_state.advertising_type)
  {
#if gBondingSupported_d
  case fastWhiteListAdvState_c:
    {
      DEBUG_PRINT("Advertising type = White List\r\n");
      advParams.minInterval = gFastConnMinAdvInterval_c;
      advParams.maxInterval = gFastConnMaxAdvInterval_c;
      advParams.filterPolicy = (gapAdvertisingFilterPolicyFlags_t)(gProcessConnWhiteListFlag_c | gProcessScanWhiteListFlag_c);
      mAdvTimeout = gFastConnWhiteListAdvTime_c;
    }
    break;
#endif
  case fastAdvState_c:
    {
      DEBUG_PRINT("Advertising type = Fast\r\n");
      advParams.minInterval = gFastConnMinAdvInterval_c;
      advParams.maxInterval = gFastConnMaxAdvInterval_c;
      advParams.filterPolicy = gProcessAll_c;
      mAdvTimeout = gFastConnAdvTime_c - gFastConnWhiteListAdvTime_c;
    }
    break;

  case slowAdvState_c:
    {
      DEBUG_PRINT("Advertising type = Slow\r\n");
      advParams.minInterval = gReducedPowerMinAdvInterval_c;
      advParams.maxInterval = gReducedPowerMinAdvInterval_c;
      advParams.filterPolicy = gProcessAll_c;
      mAdvTimeout = gReducedPowerAdvTime_c;
    }
    break;
  default:
    DEBUG_PRINT("Advertising type = default\r\n");
    break;
  }

  /* Set advertising parameters */
  Gap_SetAdvertisingParameters(&advParams);
}

/*! *********************************************************************************
* \brief        Handles BLE Advertising callback from host stack.
*
* \param[in]    pAdvertisingEvent    Pointer to gapAdvertisingEvent_t.
********************************************************************************** */
static void BleApp_AdvertisingCallback(gapAdvertisingEvent_t *pAdvertisingEvent)
{
  DEBUG_PRINT(".. -> .. -> Function: BleApp_AdvertisingCallback\r\n");
  DEBUG_PRINT("  [\r\n");

  switch (pAdvertisingEvent->eventType)
  {
  case gAdvertisingStateChanged_c:
    {
      // Переключился режим передачи объявлений
      DEBUG_PRINT("      Event: gAdvertisingStateChanged_c\r\n");


      advertising_state.advertising_active = !advertising_state.advertising_active;

#if (USE_POWER_DOWN_MODE)
      if (!advertising_state.advertising_active)
      {
        PWR_ChangeDeepSleepMode(3);
        PWR_AllowDeviceToSleep();
      }
      else
      {
        PWR_ChangeDeepSleepMode(1);

#ifdef ENABLE_SLOW_ADVERTISING
        // Задаем через некоторое время поменять частоту передачи объявлений в функции AdvertisingTimerCallback
        TMR_StartLowPowerTimer(advertising_timer_id, TMR_LOW_POWER_SECOND_TIMER, TmrSeconds(mAdvTimeout), AdvertisingTimerCallback, NULL);
#endif
      }
#else

      if (advertising_state.advertising_active)
      {
#ifdef ENABLE_SLOW_ADVERTISING
        // Задаем через некоторое время поменять частоту передачи объявлений в функции AdvertisingTimerCallback
        TMR_StartLowPowerTimer(advertising_timer_id, TMR_LOW_POWER_SECOND_TIMER, TmrSeconds(mAdvTimeout), AdvertisingTimerCallback, NULL);
#endif
      }
#endif
    }
    break;

  case gAdvertisingCommandFailed_c:
    {
      DEBUG_PRINT("      Event: gAdvertisingCommandFailed_c\r\n");
      Panic(0, 0, 0, 0);
    }
    break;

  default:
    DEBUG_PRINT("      Event: default\r\n");
    break;
  }
  DEBUG_PRINT("  ]\r\n");

}

/*! *********************************************************************************
* \brief        Handles BLE Connection callback from host stack.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    pConnectionEvent    Pointer to gapConnectionEvent_t.
********************************************************************************** */
static void BleApp_ConnectionCallback(deviceId_t peerDeviceId, gapConnectionEvent_t *pConnectionEvent)
{
  DEBUG_PRINT_ARG(".. -> .. -> Function: BleApp_ConnectionCallback, device %d \r\n",peerDeviceId);
  DEBUG_PRINT("  [\r\n");
  switch (pConnectionEvent->eventType)
  {
  case gConnEvtConnected_c:
    {
      DEBUG_PRINT_ARG("      Event: gConnEvtConnected_c, peerDeviceId=%d\r\n", peerDeviceId);

      mPeerDeviceId = peerDeviceId;

      /* Advertising stops when connected */
      advertising_state.advertising_active = FALSE;

//      DEBUG_PRINT( "      L2ca Update Connection Parameters\r\n");
//      L2ca_UpdateConnectionParameters(peerDeviceId, FAST_CONN_MIN_INTERVAL, FAST_CONN_MAX_INTERVAL, FAST_CONN_SLAVE_LATENCY, FAST_CONN_TIMEOUT_MULTIPLIER, gcConnectionEventMinDefault_c, gcConnectionEventMaxDefault_c);
//      L2ca_EnableUpdateConnectionParameters(peerDeviceId, TRUE);

#if gBondingSupported_d
      /* Copy peer device address information */
      mPeerDeviceAddressType = pConnectionEvent->eventData.connectedEvent.peerAddressType;
      FLib_MemCpy(maPeerDeviceAddress, pConnectionEvent->eventData.connectedEvent.peerAddress, sizeof(bleDeviceAddress_t));
#endif
#if gUseServiceSecurity_d
      {
        bool_t isBonded = FALSE;

        if (gBleSuccess_c == Gap_CheckIfBonded(peerDeviceId,&isBonded) && FALSE == isBonded)
        {
          DEBUG_PRINT("      Gap_SendSlaveSecurityRequest\r\n");
          Gap_SendSlaveSecurityRequest(peerDeviceId, TRUE, gSecurityMode_1_Level_3_c);
        }
      }
#endif
      /* Subscribe client*/
      DEBUG_PRINT("      Battery service subscribe\r\n");
      Bas_Subscribe(peerDeviceId);

      /* Stop Advertising Timer*/
      advertising_state.advertising_active = FALSE;
      DEBUG_PRINT("      Stop advertising timer\r\n");
      TMR_StopTimer(advertising_timer_id);

      /* Start battery measurements */
      DEBUG_PRINT("      Start battery servise timer\r\n");
      TMR_StartLowPowerTimer(battery_service_timer_id, TMR_LOW_POWER_INTERVAL_MILLIS_TIMER, TmrSeconds(mBatteryLevelReportInterval_c), Timer_callback_for_battery_service, NULL);
    }
    break;

  case gConnEvtDisconnected_c:
    {
      DEBUG_PRINT("      Event: gConnEvtDisconnected_c\r\n");

      /* Unsubscribe client */
      DEBUG_PRINT("      Battery service unsubscribe\r\n");
      Bas_Unsubscribe();
      mPeerDeviceId = gInvalidDeviceId_c;

      DEBUG_PRINT("      Stop timers\r\n");
      TMR_StopTimer(battery_service_timer_id);

#if (USE_POWER_DOWN_MODE)

      /* Go to sleep */
      PWR_ChangeDeepSleepMode(3); /* MCU=LLS3, LL=IDLE, wakeup on swithes/LL */
      PWR_AllowDeviceToSleep();
#else
      if (pConnectionEvent->eventData.disconnectedEvent.reason == gHciConnectionTimeout_c)
      {
        DEBUG_PRINT("      Restart advertising\r\n");

        /* Link loss detected*/
        BleApp_Restart_Advertising();
      }
      else
      {
        DEBUG_PRINT("      Restart advertising\r\n");
        /* Connection was terminated by peer or application */
        BleApp_Restart_Advertising();
      }
#endif
    }
    break;

#if gBondingSupported_d
  case gConnEvtKeysReceived_c:
    {
      DEBUG_PRINT("      Event: gConnEvtKeysReceived_c\r\n");

      /* Copy peer device address information when IRK is used */
      if (pConnectionEvent->eventData.keysReceivedEvent.pKeys->aIrk != NULL)
      {
        mPeerDeviceAddressType = pConnectionEvent->eventData.keysReceivedEvent.pKeys->addressType;
        FLib_MemCpy(maPeerDeviceAddress, pConnectionEvent->eventData.keysReceivedEvent.pKeys->aAddress, sizeof(bleDeviceAddress_t));
      }
    }
    break;

  case gConnEvtPairingComplete_c:
    {
      DEBUG_PRINT("      Event: gConnEvtPairingComplete_c\r\n");

      if (pConnectionEvent->eventData.pairingCompleteEvent.pairingSuccessful && pConnectionEvent->eventData.pairingCompleteEvent.pairingCompleteData.withBonding)
      {
        /* If a bond is created, write device address in controller’s White List */
        Gap_AddDeviceToWhiteList(mPeerDeviceAddressType, maPeerDeviceAddress);

        DEBUG_PRINT("      L2ca Update Connection Parameters\r\n");
        L2ca_UpdateConnectionParameters(peerDeviceId, FAST_CONN_MIN_INTERVAL, FAST_CONN_MAX_INTERVAL, FAST_CONN_SLAVE_LATENCY, FAST_CONN_TIMEOUT_MULTIPLIER, gcConnectionEventMinDefault_c, gcConnectionEventMaxDefault_c);
//        L2ca_EnableUpdateConnectionParameters(peerDeviceId, TRUE);

      }
    }
    break;

  case gConnEvtPairingRequest_c:
    {
      DEBUG_PRINT("      Event: gConnEvtPairingRequest_c\r\n");

      gPairingParameters.centralKeys = pConnectionEvent->eventData.pairingEvent.centralKeys;
      Gap_AcceptPairingRequest(peerDeviceId,&gPairingParameters);
    }
    break;

  case gConnEvtKeyExchangeRequest_c:
    {
      gapSmpKeys_t sentSmpKeys = gSmpKeys;

      DEBUG_PRINT("      Event: gConnEvtKeyExchangeRequest_c\r\n");

      if (!(pConnectionEvent->eventData.keyExchangeRequestEvent.requestedKeys & gLtk_c))
      {
        sentSmpKeys.aLtk = NULL;
        /* When the LTK is NULL EDIV and Rand are not sent and will be ignored. */
      }

      if (!(pConnectionEvent->eventData.keyExchangeRequestEvent.requestedKeys & gIrk_c))
      {
        sentSmpKeys.aIrk = NULL;
        /* When the IRK is NULL the Address and Address Type are not sent and will be ignored. */
      }

      if (!(pConnectionEvent->eventData.keyExchangeRequestEvent.requestedKeys & gCsrk_c))
      {
        sentSmpKeys.aCsrk = NULL;
      }

      Gap_SendSmpKeys(peerDeviceId,&sentSmpKeys);
      break;
    }

  case gConnEvtLongTermKeyRequest_c:
    {
      DEBUG_PRINT("      Event: gConnEvtLongTermKeyRequest_c\r\n");

      if (pConnectionEvent->eventData.longTermKeyRequestEvent.ediv == gSmpKeys.ediv &&
          pConnectionEvent->eventData.longTermKeyRequestEvent.randSize == gSmpKeys.cRandSize)
      {
        /* EDIV and RAND both matched */
        Gap_ProvideLongTermKey(peerDeviceId, gSmpKeys.aLtk, gSmpKeys.cLtkSize);
      }
      else
      /* EDIV or RAND size did not match */
      {
        Gap_DenyLongTermKey(peerDeviceId);
      }
    }
    break;
#endif


  case gConnEvtSlaveSecurityRequest_c:
    DEBUG_PRINT("      Event: gConnEvtSlaveSecurityRequest_c\r\n");
    break;
  case gConnEvtPairingResponse_c:
    DEBUG_PRINT("      Event: gConnEvtPairingResponse_c\r\n");
    break;
  case gConnEvtAuthenticationRejected_c:
    DEBUG_PRINT("      Event: gConnEvtAuthenticationRejected_c\r\n");
    break;
  case gConnEvtPasskeyRequest_c:
    DEBUG_PRINT("      Event: gConnEvtPasskeyRequest_c\r\n");
    break;
  case gConnEvtOobRequest_c:
    DEBUG_PRINT("      Event: gConnEvtOobRequest_c\r\n");
    break;
  case gConnEvtPasskeyDisplay_c:
    DEBUG_PRINT("      Event: gConnEvtPasskeyDisplay_c\r\n");
    break;
  case gConnEvtEncryptionChanged_c:
    DEBUG_PRINT("      Event: gConnEvtEncryptionChanged_c\r\n");

    DEBUG_PRINT("      L2ca Update Connection Parameters\r\n");
    L2ca_UpdateConnectionParameters(peerDeviceId, FAST_CONN_MIN_INTERVAL, FAST_CONN_MAX_INTERVAL, FAST_CONN_SLAVE_LATENCY, FAST_CONN_TIMEOUT_MULTIPLIER, gcConnectionEventMinDefault_c, gcConnectionEventMaxDefault_c);
    break;
  case gConnEvtRssiRead_c:
    DEBUG_PRINT("      Event: gConnEvtRssiRead_c\r\n");
    // Имитируем изменение уровня заряда аккумулятора
    basServiceConfig.batteryLevel = pConnectionEvent->eventData.rssi_dBm;

    break;
  case gConnEvtTxPowerLevelRead_c:
    DEBUG_PRINT("      Event: gConnEvtTxPowerLevelRead_c\r\n");
    break;
  case gConnEvtPowerReadFailure_c:
    DEBUG_PRINT("      Event: gConnEvtPowerReadFailure_c\r\n");
    break;

  default:
    DEBUG_PRINT("      Event: default\r\n");
    break;
  }
  DEBUG_PRINT("  ]\r\n");

}

/*! *********************************************************************************
* \brief        Handles GATT server callback from host stack.
*
* \param[in]    deviceId        Peer device ID.
* \param[in]    pServerEvent    Pointer to gattServerEvent_t.
********************************************************************************** */
static void BleApp_GattServerCallback(deviceId_t deviceId, gattServerEvent_t *pServerEvent)
{
  uint16_t attr_handle;
  uint8_t status;

  DEBUG_PRINT_ARG(".. -> .. -> Function: BleApp_GattServerCallback from device %d\r\n",deviceId);
  DEBUG_PRINT("  [\r\n");

  switch (pServerEvent->eventType)
  {
  case gEvtAttributeWritten_c:
    {
      DEBUG_PRINT("      Event: gEvtAttributeWritten_c\r\n");

      attr_handle = pServerEvent->eventData.attributeWrittenEvent.handle;
      status = gAttErrCodeNoError_c;
#ifdef K66BLEZ_PROFILE
      if (attr_handle == value_k66wrdata_w_resp)
      {
        status = Write_to_value_k66wrdata(deviceId, attr_handle, pServerEvent->eventData.attributeWrittenEvent.aValue, pServerEvent->eventData.attributeWrittenEvent.cValueLength);
      }
      if (attr_handle == value_k66rddata)
      {
        status = Write_to_value_k66rddata(deviceId, attr_handle, pServerEvent->eventData.attributeWrittenEvent.aValue, pServerEvent->eventData.attributeWrittenEvent.cValueLength);
      }
      if (attr_handle == value_k66_cmd_write)
      {
        status = CommandService_write(deviceId, attr_handle, pServerEvent->eventData.attributeWrittenEvent.aValue, pServerEvent->eventData.attributeWrittenEvent.cValueLength);
      }
#endif
      GattServer_SendAttributeWrittenStatus(deviceId, attr_handle, status);
    }
    break;


  case gEvtAttributeWrittenWithoutResponse_c:
    {
      DEBUG_PRINT("      Event: gEvtAttributeWrittenWithoutResponse_c\r\n");
#ifdef K66BLEZ_PROFILE
      attr_handle = pServerEvent->eventData.attributeWrittenEvent.handle;
      status = gAttErrCodeNoError_c;
      if (attr_handle == value_k66wrdata)
      {
        status = Write_to_value_k66wrdata(deviceId, attr_handle, pServerEvent->eventData.attributeWrittenEvent.aValue, pServerEvent->eventData.attributeWrittenEvent.cValueLength);
      }
#endif
    }
    break;


  case gEvtMtuChanged_c:
    DEBUG_PRINT("      Event: gEvtMtuChanged_c\r\n");
    break;
  case gEvtHandleValueConfirmation_c:
    DEBUG_PRINT("      Event: gEvtHandleValueConfirmation_c\r\n");
    break;
  case gEvtCharacteristicCccdWritten_c:
    DEBUG_PRINT_ARG("      Event: gEvtCharacteristicCccdWritten_c %04X = %08X\r\n", pServerEvent->eventData.charCccdWrittenEvent.handle, (uint32_t)pServerEvent->eventData.charCccdWrittenEvent.newCccd);
    break;
  case gEvtError_c:
    DEBUG_PRINT("      Event: gEvtError_c\r\n");
    break;
  case gEvtLongCharacteristicWritten_c:
    DEBUG_PRINT("      Event: gEvtLongCharacteristicWritten_c\r\n");
    break;
  case gEvtAttributeRead_c:
    // Событие чтения подписанного на генерацию этого события аттрибута
    // Сначала происходит это событие и после него производится реальное чтение
    DEBUG_PRINT("      Event: gEvtAttributeRead_c\r\n");
#ifdef K66BLEZ_PROFILE
    attr_handle = pServerEvent->eventData.attributeWrittenEvent.handle;
    if (attr_handle == value_k66wrdata_w_resp)
    {
      Restart_perfomance_test();
      status = gAttErrCodeNoError_c;
      GattServer_SendAttributeReadStatus(deviceId, attr_handle, status);
    }
    if (attr_handle == value_k66wrdata)
    {
      Restart_perfomance_test();
      status = gAttErrCodeNoError_c;
      GattServer_SendAttributeReadStatus(deviceId, attr_handle, status);
    }
    if (attr_handle == value_k66rddata)
    {
      Read_from_value_k66rddata();
      status = gAttErrCodeNoError_c;
      GattServer_SendAttributeReadStatus(deviceId, attr_handle, status);
    }
    if (attr_handle == value_k66_cmd_read)
    {
      CommandService_read();
      status = gAttErrCodeNoError_c;
      GattServer_SendAttributeReadStatus(deviceId, attr_handle, status);
    }

#endif
    break;

  default:
    DEBUG_PRINT("      Event: default\r\n");
    break;
  }
  DEBUG_PRINT("  ]\r\n");
}

/*! *********************************************************************************
* \brief        Handles advertising timer callback.
*
* \param[in]    pParam        Calback parameters.
********************************************************************************** */
static void AdvertisingTimerCallback(void *pParam)
{
  DEBUG_CALLBACK_PRINT("~~~~~ Advertisin timer callback.\r\n");
  Gap_StopAdvertising(); // Останавливаем оповещение чтобы инициализировать новые параметры оповещения
  switch (advertising_state.advertising_type)
  {
#if gBondingSupported_d
  case fastWhiteListAdvState_c:
    {
      advertising_state.advertising_type = fastAdvState_c;
    }
    break;
#endif
  case fastAdvState_c:
    {
      advertising_state.advertising_type = slowAdvState_c;
    }
    break;

  default:
    break;
  }
  BleApp_SetAdvertisingParameters();
}

/*! *********************************************************************************
* \brief        Handles measurement timer callback.
*
* \param[in]    pParam        Calback parameters.
********************************************************************************** */
static void Timer_calback_for_heart_rate_service(void *pParam)
{
  DEBUG_CALLBACK_PRINT("~~~~~ Measurement timer callback.\r\n");

#if (USE_POWER_DOWN_MODE)
  PWR_SetDeepSleepTimeInMs(900);
  PWR_ChangeDeepSleepMode(6);
  PWR_AllowDeviceToSleep();
#endif
}

/*! *********************************************************************************
* \brief        Handles battery measurement timer callback.
*
* \param[in]    pParam        Calback parameters.
********************************************************************************** */
static void Timer_callback_for_battery_service(void *pParam)
{
  DEBUG_CALLBACK_PRINT("~~~~~ Battery & RSSI timer callback.\r\n");

  GetRSSI(); // Вместо уровня заряду будем передавать RSSI подключенного клиента

  //basServiceConfig.batteryLevel = BOARD_GetBatteryLevel();

  Bas_RecordBatteryMeasurement(basServiceConfig.serviceHandle, basServiceConfig.batteryLevel);
}

/*------------------------------------------------------------------------------
  Получить значение RSSI для подключенного клиента

 ------------------------------------------------------------------------------*/
static void GetRSSI(void)
{
  Gap_ReadRssi(mPeerDeviceId);
}

extern uint8_t *value_device_name_valueArray;
extern uint8_t *value_sw_rev_valueArray;
/*-----------------------------------------------------------------------------------------------------

 \param void
-----------------------------------------------------------------------------------------------------*/
void BleApp_setup_names(void)
{
  uint32_t  i;
  uint32_t  len;
  uint32_t  n = sizeof(advScanStruct) / sizeof(advScanStruct[0]);

  // Записать имя устройства передаваемое в пакетах адвертайзинга
  for (i=0; i < n; i++)
  {
    if (advScanStruct[i].adType == gAdShortenedLocalName_c)
    {
      advScanStruct[i].aData = (uint8_t *)advertised_device_name;
      advScanStruct[i].length = strlen(advertised_device_name)+1;
    }
  }
  len = strlen(advertised_device_name);
  memcpy((void *)&value_device_name_valueArray ,advertised_device_name, len);
  if (len < 20) memset((void *)((uint8_t *)&value_device_name_valueArray + len), 0x20, 20- len);

  len = strlen(software_revision);
  memcpy((void *)&value_sw_rev_valueArray, software_revision, len);
  if (len < 20) memset((void *)((uint8_t *)&value_sw_rev_valueArray + len), 0x20, 20- len);

}
/*-----------------------------------------------------------------------------------------------------

 \param void
-----------------------------------------------------------------------------------------------------*/
void BleApp_setup_attributes(void)
{

  GattDb_WriteAttribute(value_device_name, strlen(advertised_device_name)+1, (uint8_t *)advertised_device_name);
  GattDb_WriteAttribute(value_sw_rev, strlen(software_revision)+1, (uint8_t *)software_revision);

}

