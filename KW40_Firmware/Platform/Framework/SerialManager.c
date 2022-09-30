/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file SerialManager.c
* This is the source file for the Serial Manager.
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


/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */

#include "SerialManager.h"
#include "Panic.h"
#include "MemManager.h"
#include "Messaging.h"
#include "FunctionLib.h"
#include "Gpio_IrqAdapter.h"

#include "fsl_device_registers.h"
#include "fsl_os_abstraction.h"
#include "fsl_gpio_driver.h"
#include <string.h>

#if gSerialMgr_DisallowMcuSleep_d
  #include "PWR_Interface.h"
#endif

#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
  #include "fsl_uart_driver.h"
  #include "fsl_uart_hal.h"
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
  #include "fsl_lpuart_driver.h"
  #include "fsl_lpuart_hal.h"
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
  #include "fsl_lpsci_driver.h"
  #include "fsl_lpsci_hal.h"
#endif
  #include "fsl_clock_manager.h"
#endif

/*! *********************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
********************************************************************************** */
#ifndef gSMGR_UseOsSemForSynchronization_c
#define gSMGR_UseOsSemForSynchronization_c  (USE_RTOS)
#endif

#define mSerial_IncIdx_d(idx, max) if( ++(idx) >= (max) ) { (idx) = 0; }

#define mSerial_DecIdx_d(idx, max) if( (idx) > 0 ) { (idx)--; } else  { (idx) = (max) - 1; }

#define gSMRxBufSize_c (gSerialMgrRxBufSize_c + 1)

#define mSMGR_DapIsrPrio_c    (0x80)
#define mSMGR_I2cIsrPrio_c    (0x40)
#define mSMGR_UartIsrPrio_c   (0x40)
#define mSMGR_LpuartIsrPrio_c (0x40)
#define mSMGR_LpsciIsrPrio_c  (0x40)

/*! *********************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
********************************************************************************** */
/* 
 * Set the size of the Rx buffer indexes 
 */
#if gSMRxBufSize_c < 255
typedef uint8_t bufIndex_t;
#else
typedef uint16_t bufIndex_t;
#endif

/* 
 * Defines events recognized by the SerialManager's Task
 * Message used to enque async tx data 
 */
typedef struct SerialManagetMsg_tag{
    pSerialCallBack_t txCallback;
    void             *pTxParam;
    uint8_t          *pData;
    uint16_t          dataSize;
}SerialMsg_t;

/* 
 * Defines the serial interface structure 
 */
typedef struct serial_tag{
    serialInterfaceType_t  serialType;
    uint8_t                serialChannel;
    /* Rx parameters */
    bufIndex_t             rxIn;
    volatile bufIndex_t    rxOut;
    pSerialCallBack_t      rxCallback;
    void                  *pRxParam;
    uint8_t                rxBuffer[gSMRxBufSize_c];
    /* Tx parameters */
    SerialMsg_t            txQueue[gSerialMgrTxQueueSize_c];
#if gSMGR_UseOsSemForSynchronization_c
    semaphore_t            txSyncSem;
#if gSerialMgr_BlockSenderOnQueueFull_c
    semaphore_t            txQueueSem;
    uint8_t                txBlockedTasks;
#endif
#endif
    uint8_t                txIn;
    uint8_t                txOut;
    uint8_t                txCurrent;
    uint8_t                events;
    uint8_t                state;
}serial_t;

/* 
 * SMGR task event flags 
 */
typedef enum{
    gSMGR_Rx_c     = (1<<0),
    gSMGR_TxDone_c = (1<<1),
    gSMGR_TxNew_c  = (1<<2)
}serialEventType_t;


/*
 * Common driver data structure union
 */
typedef union smgrDrvData_tag
{
#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
  uart_state_t uartState;
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
  lpuart_state_t lpuartState;
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
  lpsci_state_t lpsciState;
#endif
#endif /* #if (gSerialMgrUseUart_c) */
  void *pDrvData;
}smgrDrvData_t;

/*! *********************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
********************************************************************************** */
#if (gSerialManagerMaxInterfaces_c)
/*
 * SMGR internal functions
 */
void SerialManagerTask(task_param_t argument);
void  SerialManager_RxNotify(uint32_t interfaceId);
void  SerialManager_TxNotify(uint32_t interfaceId);
#if gSMGR_UseOsSemForSynchronization_c
static void  Serial_SyncTxCallback(void *pSer);
#endif
static void  Serial_TxQueueMaintenance(serial_t *pSer);
static serialStatus_t Serial_WriteInternal (uint8_t InterfaceId);

/*
 * UART, LPUART and LPSCI specific functions
 */
#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
void UART_IRQHandler(void);
extern void UART_DRV_IRQHandler(uint32_t instance);
void Serial_UartRxCb(uint32_t instance, void* state);
void Serial_UartTxCb(uint32_t instance, void* state);
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
void LPUART_IRQHandler(void);
extern void LPUART_DRV_IRQHandler(uint32_t instance);
void Serial_LpuartRxCb(uint32_t instance, void* state);
void Serial_LpuartTxCb(uint32_t instance, void* state);
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
void LPSCI_IRQHandler(void);
extern void LPSCI_DRV_IRQHandler(uint32_t instance);
void Serial_LpsciRxCb(uint32_t instance, void* state);
void Serial_LpsciTxCb(uint32_t instance, void* state);
#endif
#endif

#endif /* #if (gSerialManagerMaxInterfaces_c) */



/*! *********************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
********************************************************************************** */
#if gSerialManagerMaxInterfaces_c

/*
 * RTOS objects definition
 */

OSA_TASK_DEFINE( SMGR, SERIAL_TASK_STACK_SZ );
task_handler_t gSerialManagerTaskId;
event_t        mSMTaskEvent;

/*
 * SMGR internal data
 */
static serial_t      mSerials[gSerialManagerMaxInterfaces_c];
static smgrDrvData_t mDrvData[gSerialManagerMaxInterfaces_c];


/*
 * Default configuration for UART, LPUART and LPSCI drivers
 */
#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
uart_user_config_t mSmgr_UartCfg = {
    .baudRate = 115200,
    .parityMode = kUartParityDisabled,
    .stopBitCount = kUartOneStopBit,
    .bitCountPerChar = kUart8BitsPerChar
};
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
const lpuart_user_config_t mSmgr_LpuartCfg = {
    .clockSource = kClockLpuartSrcOsc0erClk,
    .baudRate = 115200,
    .parityMode = kLpuartParityDisabled,
    .stopBitCount = kLpuartOneStopBit,
    .bitCountPerChar = kLpuart8BitsPerChar
};
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
const lpsci_user_config_t mSmgr_LpsciCfg = {
    .clockSource = kClockLpsciSrcPllFllSel,         
    .baudRate = 115200,
    .parityMode = kLpsciParityDisabled,
    .stopBitCount = kLpsciOneStopBit,
    .bitCountPerChar = kLpsci8BitsPerChar
};
#endif
#endif /* #if (gSerialMgrUseUart_c) */

#endif /* #if gSerialManagerMaxInterfaces_c */

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
* \brief   Creates the SerialManager's task and initializes internal data structures
*
********************************************************************************** */
void SerialManager_Init( void )
{
#if (gSerialManagerMaxInterfaces_c)       
    static uint8_t initialized = FALSE;

    /* Check if SMGR is already initialized */
    if( initialized )
        return;

    initialized = TRUE;

    /* Fill the structure with zeros */
    FLib_MemSet( mSerials, 0x00, sizeof(mSerials) );
    osa_status_t status;
    
    status = OSA_EventCreate( &mSMTaskEvent, kEventAutoClear);
    if( kStatus_OSA_Success != status )
    {
        Panic(0,0,0,0);
        return;
    }

    status = OSA_TaskCreate(SerialManagerTask, "SMGR_Task", SERIAL_TASK_STACK_SZ, SMGR_stack,SERIAL_TASK_PRIO, (task_param_t)NULL, FALSE, &gSerialManagerTaskId);
    if( kStatus_OSA_Success != status )
    {
        Panic(0,0,0,0);
        return;
    }
#endif /* #if (gSerialManagerMaxInterfaces_c) */
}

/*! *********************************************************************************
* \brief   The main task of the Serial Manager
*
* \param[in] initialData unused
*
********************************************************************************** */
#if (gSerialManagerMaxInterfaces_c)
void SerialManagerTask(task_param_t argument)
{
    uint16_t i;
    uint8_t ev;    

    event_flags_t  mSMTaskEventFlags;        

    while( 1 )
    {
        /* Wait for an event. The task will block here. */
        (void)OSA_EventWait(&mSMTaskEvent, 0x00FFFFFF, FALSE, OSA_WAIT_FOREVER ,&mSMTaskEventFlags);
        for( i = 0; i < gSerialManagerMaxInterfaces_c; i++ )
        {
            OSA_EnterCritical(kCriticalDisableInt);
            ev = mSerials[i].events;
            mSerials[i].events = 0;
            OSA_ExitCritical(kCriticalDisableInt);

            if ( (ev & gSMGR_Rx_c) &&
                 (NULL != mSerials[i].rxCallback) )
            {
                mSerials[i].rxCallback( mSerials[i].pRxParam );
            }

            if( ev & gSMGR_TxDone_c )
            {
                Serial_TxQueueMaintenance(&mSerials[i]);
            }

            /* If the Serial is IDLE and there is data to tx */
            if( (mSerials[i].state == 0) && mSerials[i].txQueue[mSerials[i].txCurrent].dataSize )
            {
                (void)Serial_WriteInternal( i );
            }
        }
         
    } /* while(1) */
}
#endif

/*! *********************************************************************************
* \brief   Initialize a communication interface.
*
* \param[in] pInterfaceId   pointer to a location where the interface Id will be stored
* \param[in] interfaceType  the type of the interface: UART/SPI/IIC/USB
* \param[in] instance       the instance of the HW module (ex: if UART1 is used, this value should be 1)
*
* \return The interface number if success or gSerialManagerInvalidInterface_c if an error occured.
*
********************************************************************************** */
serialStatus_t Serial_InitInterface( uint8_t *pInterfaceId,
                                     serialInterfaceType_t interfaceType,
                                     uint8_t instance )
{
#if gSerialManagerMaxInterfaces_c
    uint8_t i;
    serial_t *pSer;

    *pInterfaceId = gSerialMgrInvalidIdx_c;

    for ( i=0; i<gSerialManagerMaxInterfaces_c; i++ )
    {
        pSer = &mSerials[i];

        if ( (pSer->serialType == interfaceType) &&
            (pSer->serialChannel == instance) )
        {
            /* The Interface is allready opened. */
            return gSerial_InterfaceInUse_c;
        }
        else if ( pSer->serialType == gSerialMgrNone_c )
        {
            OSA_EnterCritical(kCriticalDisableInt);
            pSer->serialChannel = instance;
            switch ( interfaceType )
            {
            case gSerialMgrUart_c:
#if gSerialMgrUseUart_c && FSL_FEATURE_SOC_UART_COUNT
                {
                    IRQn_Type irq = g_uartRxTxIrqId[instance];

                    configure_uart_pins(instance);
                    NVIC_SetPriority(irq, mSMGR_UartIsrPrio_c >> (8 - __NVIC_PRIO_BITS));
                    OSA_InstallIntHandler(irq, UART_IRQHandler);
                    UART_DRV_Init(instance, &mDrvData[i].uartState, &mSmgr_UartCfg);
                    UART_DRV_InstallRxCallback(instance, Serial_UartRxCb, &pSer->rxBuffer[pSer->rxIn], (void*)i, TRUE);
                    UART_DRV_InstallTxCallback(instance, Serial_UartTxCb, NULL, (void*)i);
                }
#endif
                break;

            case gSerialMgrLpuart_c:
#if gSerialMgrUseUart_c && FSL_FEATURE_SOC_LPUART_COUNT
                {
                    IRQn_Type irq = g_lpuartRxTxIrqId[instance];


                    NVIC_SetPriority(irq, mSMGR_LpuartIsrPrio_c >> (8 - __NVIC_PRIO_BITS));
                    OSA_InstallIntHandler(irq, LPUART_IRQHandler);
                    LPUART_DRV_Init(instance, &mDrvData[i].lpuartState, &mSmgr_LpuartCfg);
                    LPUART_DRV_InstallRxCallback(instance, Serial_LpuartRxCb, &pSer->rxBuffer[pSer->rxIn], (void*)i, TRUE);
                    LPUART_DRV_InstallTxCallback(instance, Serial_LpuartTxCb, NULL, (void*)i);
                }
#endif
                break;

            case gSerialMgrLpsci_c:
#if gSerialMgrUseUart_c && FSL_FEATURE_SOC_LPSCI_COUNT
                {
                    IRQn_Type irq = g_lpsciRxTxIrqId[instance];

                    configure_lpsci_pins(instance);
                    NVIC_SetPriority(irq, mSMGR_LpsciIsrPrio_c >> (8 - __NVIC_PRIO_BITS));
                    OSA_InstallIntHandler(irq, LPSCI_IRQHandler);
                    LPSCI_DRV_Init(instance, &mDrvData[i].lpsciState, &mSmgr_LpsciCfg);
                    LPSCI_DRV_InstallRxCallback(instance, Serial_LpsciRxCb, &pSer->rxBuffer[pSer->rxIn], (void*)i, TRUE);
                    LPSCI_DRV_InstallTxCallback(instance, Serial_LpsciTxCb, NULL, (void*)i);
                }
#endif
                break;

            case gSerialMgrUSB_c:
                break;

            case gSerialMgrIICMaster_c:
                break;                

            case gSerialMgrIICSlave_c:
                break;

            case gSerialMgrSPIMaster_c:
                break;

            case gSerialMgrSPISlave_c:
                break;

            default:
                OSA_ExitCritical(kCriticalDisableInt);
                return gSerial_InvalidInterface_c;
            }

#if gSMGR_UseOsSemForSynchronization_c
            if( kStatus_OSA_Success != OSA_SemaCreate(&pSer->txSyncSem, 0) )
            {
                OSA_ExitCritical(kCriticalDisableInt);
                return gSerial_SemCreateError_c;
            }

#if gSerialMgr_BlockSenderOnQueueFull_c
            if( kStatus_OSA_Success != OSA_SemaCreate(&pSer->txQueueSem, 0) )
            {
                OSA_ExitCritical(kCriticalDisableInt);
                return gSerial_SemCreateError_c;
            }
#endif /* gSerialMgr_BlockSenderOnQueueFull_c */
#endif /* gSMGR_UseOsSemForSynchronization_c */

            pSer->serialType = interfaceType;
            *pInterfaceId = i;
            OSA_ExitCritical(kCriticalDisableInt);
            return gSerial_Success_c;
        }
    }

    /* There are no more free interfaces. */
    return gSerial_MaxInterfacesReached_c;
#else
    (void)interfaceType;
    (void)instance;
    (void)pInterfaceId;
    return gSerial_Success_c;
#endif
}

/*! *********************************************************************************
* \brief   Transmit a data buffer asynchronously
*
* \param[in] InterfaceId the interface number
* \param[in] pBuf pointer to data location
* \param[in] bufLen the number of bytes to be sent
* \param[in] pSerialRxCallBack pointer to a function that will be called when
*            a new char is available
*
* \return The status of the operation
*
********************************************************************************** */
serialStatus_t Serial_AsyncWrite( uint8_t id,
                                  uint8_t *pBuf,
                                  uint16_t bufLen,
                                  pSerialCallBack_t cb,
                                  void *pTxParam )
{
#if gSerialManagerMaxInterfaces_c
    SerialMsg_t *pMsg = NULL;
    serial_t *pSer = &mSerials[id];

#if gSerialMgr_ParamValidation_d
    if( (NULL == pBuf) || (0 == bufLen)       ||
        (id >= gSerialManagerMaxInterfaces_c) ||
        (pSer->serialType == gSerialMgrNone_c) )
    {
        return gSerial_InvalidParameter_c;
    }
#endif
    task_handler_t taskHandler = OSA_TaskGetHandler();

#if (gSerialMgr_BlockSenderOnQueueFull_c == 0)
    if( taskHandler == gSerialManagerTaskId )
    {
        Serial_TxQueueMaintenance(pSer);
    }
#endif

    /* Check if slot is free */
#if gSerialMgr_BlockSenderOnQueueFull_c    
    while(1)
#endif      
    {
        OSA_EnterCritical(kCriticalDisableInt);
        if( (0 == pSer->txQueue[pSer->txIn].dataSize) && (NULL == pSer->txQueue[pSer->txIn].txCallback) )
        {
            pMsg = &pSer->txQueue[pSer->txIn];
            pMsg->dataSize   = bufLen;
            pMsg->pData      = (void*)pBuf;
            pMsg->txCallback = cb;
            pMsg->pTxParam   = pTxParam;
            mSerial_IncIdx_d(pSer->txIn, gSerialMgrTxQueueSize_c);
        }
#if (gSerialMgr_BlockSenderOnQueueFull_c && gSMGR_UseOsSemForSynchronization_c)
        else
        {
            if(taskHandler != gSerialManagerTaskId)
            {
                pSer->txBlockedTasks++;
            }
        }
#endif      
        OSA_ExitCritical(kCriticalDisableInt);

        if( pMsg )
        {
            return Serial_WriteInternal( id );
        }
#if gSerialMgr_BlockSenderOnQueueFull_c
        else
        {
#if gSMGR_UseOsSemForSynchronization_c              
            if(taskHandler != gSerialManagerTaskId)
            {
                (void)OSA_SemaWait(&pSer->txQueueSem, OSA_WAIT_FOREVER);
            }
            else
#endif
            {
                Serial_TxQueueMaintenance(pSer); 
            }   
        }
#endif      
    }
    
#if (gSerialMgr_BlockSenderOnQueueFull_c == 0)
    return gSerial_OutOfMemory_c;
#endif  
#else
    (void)id;
    (void)pBuf;
    (void)bufLen;
    (void)cb;
    (void)pTxParam;
    return gSerial_Success_c;
#endif /* gSerialManagerMaxInterfaces_c */
}


/*! *********************************************************************************
* \brief Transmit a data buffer synchronously. The task will block until the Tx is done
*
* \param[in] pBuf pointer to data location
* \param[in] bufLen the number of bytes to be sent
* \param[in] InterfaceId the interface number
*
* \return The status of the operation
*
********************************************************************************** */
serialStatus_t Serial_SyncWrite( uint8_t InterfaceId,
                                 uint8_t *pBuf,
                                 uint16_t bufLen )
{
    serialStatus_t status = gSerial_Success_c;
#if gSerialManagerMaxInterfaces_c
    pSerialCallBack_t cb = NULL;
    volatile serial_t *pSer = &mSerials[InterfaceId];

#if gSMGR_UseOsSemForSynchronization_c
    /* If the calling task is SMGR do not block on semaphore */
    if( OSA_TaskGetHandler() != gSerialManagerTaskId )
         cb = Serial_SyncTxCallback;
#endif

    status  = Serial_AsyncWrite(InterfaceId, pBuf, bufLen, cb, (void*)pSer);

    if( gSerial_Success_c == status )
    {
        /* Wait until Tx finishes. The sem will be released by the SMGR task */
#if gSMGR_UseOsSemForSynchronization_c
        if( cb )
        {
            (void)OSA_SemaWait((semaphore_t*)&pSer->txSyncSem, OSA_WAIT_FOREVER);
        }
        else
#endif
        {
            while(pSer->state);
        }
    }
#else
    (void)pBuf;
    (void)bufLen;
    (void)InterfaceId;
#endif /* gSerialManagerMaxInterfaces_c */
    return status;
}

/*! *********************************************************************************
* \brief   Returns a specified number of characters from the Rx buffer
*
* \param[in] InterfaceId the interface number
* \param[out] pData pointer to location where to store the characters
* \param[in] dataSize the number of characters to be read
* \param[out] bytesRead the number of characters read
*
* \return The status of the operation
*
********************************************************************************** */
serialStatus_t Serial_Read( uint8_t InterfaceId,
                            uint8_t *pData,
                            uint16_t dataSize,
                            uint16_t *bytesRead )
{
#if (gSerialManagerMaxInterfaces_c)
    serial_t *pSer = &mSerials[InterfaceId];
    serialStatus_t status = gSerial_Success_c;
    uint16_t i, bytes;

#if gSerialMgr_ParamValidation_d
    if ( (InterfaceId >= gSerialManagerMaxInterfaces_c) ||
        (NULL == pData) || (0 == dataSize) )
        return gSerial_InvalidParameter_c;
#endif

    /* Copy bytes from the SMGR Rx buffer */
    Serial_RxBufferByteCount(InterfaceId, &bytes);

    if( bytes > 0 )
    {
        if( bytes > dataSize )
            bytes = dataSize;

        /* Copy data */
        for( i=0; i<bytes; i++ )
        {
           OSA_EnterCritical(kCriticalDisableInt);          
           *pData++ = pSer->rxBuffer[pSer->rxOut++];
            if ( pSer->rxOut >= gSMRxBufSize_c )
            {
                pSer->rxOut = 0;
            }
           OSA_ExitCritical(kCriticalDisableInt);
        }

        dataSize -= bytes;
    }

    /* Aditional processing depending on interface */
    switch ( pSer->serialType )
    {
    default:
        break;
    }

    if( bytesRead )
        *bytesRead = bytes;

    return status;
#else
    (void)InterfaceId;
    (void)pData;
    (void)dataSize;
    (void)bytesRead;
    return gSerial_InvalidInterface_c;
#endif
}

/*! *********************************************************************************
* \brief   Returns a the number of bytes available in the RX buffer
*
* \param[in] InterfaceId the interface number
* \param[out] bytesCount the number of bytes available
*
* \return The status of the operation
*
********************************************************************************** */
serialStatus_t Serial_RxBufferByteCount( uint8_t InterfaceId, uint16_t *bytesCount )
{
#if (gSerialManagerMaxInterfaces_c)
#if gSerialMgr_ParamValidation_d
    if ( (InterfaceId >= gSerialManagerMaxInterfaces_c) ||
        (NULL == bytesCount) )
        return  gSerial_InvalidParameter_c;
#endif

    OSA_EnterCritical(kCriticalDisableInt);

    if( mSerials[InterfaceId].rxIn >= mSerials[InterfaceId].rxOut )
    {
        *bytesCount = mSerials[InterfaceId].rxIn - mSerials[InterfaceId].rxOut;
    }
    else
    {
        *bytesCount = gSMRxBufSize_c - mSerials[InterfaceId].rxOut + mSerials[InterfaceId].rxIn;
    }

    OSA_ExitCritical(kCriticalDisableInt);
#else
    (void)bytesCount;
    (void)InterfaceId;
#endif
    return gSerial_Success_c;
}

/*! *********************************************************************************
* \brief   Sets a pointer to a function that will be called when data is received
*
* \param[in] InterfaceId the interface number
* \param[in] pfCallBack pointer to the function to be called
* \param[in] pRxParam pointer to a parameter which will be passed to the CB function
*
* \return The status of the operation
*
********************************************************************************** */
serialStatus_t Serial_SetRxCallBack( uint8_t InterfaceId, pSerialCallBack_t cb, void *pRxParam )
{
#if (gSerialManagerMaxInterfaces_c)
#if gSerialMgr_ParamValidation_d
    if ( InterfaceId >= gSerialManagerMaxInterfaces_c )
        return gSerial_InvalidParameter_c;
#endif
    mSerials[InterfaceId].rxCallback = cb;
    mSerials[InterfaceId].pRxParam = pRxParam;
#else
    (void)InterfaceId;
    (void)cb;
    (void)pRxParam;
#endif
    return gSerial_Success_c;
}

/*! *********************************************************************************
* \brief   Set the communication speed for an interface
*
* \param[in] baudRate communication speed
* \param[in] InterfaceId the interface number
*
* \return The status of the operation
*
********************************************************************************** */
serialStatus_t Serial_SetBaudRate( uint8_t InterfaceId, uint32_t baudRate  )
{
    serialStatus_t status = gSerial_Success_c;
#if gSerialManagerMaxInterfaces_c

#if gSerialMgr_ParamValidation_d
    if ( (InterfaceId >= gSerialManagerMaxInterfaces_c) || (0 == baudRate) )
        return gSerial_InvalidParameter_c;
#endif

    switch ( mSerials[InterfaceId].serialType )
    {
#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
    case gSerialMgrUart_c:
        {
            uint32_t instance = mSerials[InterfaceId].serialChannel;
            UART_HAL_SetBaudRate(g_uartBase[instance], CLOCK_SYS_GetUartFreq(instance), baudRate);
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
    case gSerialMgrLpuart_c:
        {
            uint32_t instance = mSerials[InterfaceId].serialChannel;
            LPUART_HAL_SetBaudRate(g_lpuartBase[instance], CLOCK_SYS_GetLpuartFreq(instance), baudRate);
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
    case gSerialMgrLpsci_c:
        {
            uint32_t instance = mSerials[InterfaceId].serialChannel;
            LPSCI_HAL_SetBaudRate(g_lpsciBase[instance], CLOCK_SYS_GetLpsciFreq(instance), baudRate);
        }
        break;
#endif
#endif /* #if (gSerialMgrUseUart_c) */
    default:
        status = gSerial_InvalidInterface_c;
    }
#endif
    return status;
}

/*! *********************************************************************************
* \brief   Prints a string to the serial interface
*
* \param[in] InterfaceId the interface number
* \param[in] pString pointer to the string to be printed
* \param[in] allowToBlock specify if the task will wait for the tx to finish or not.
*
* \return The status of the operation
*
********************************************************************************** */
serialStatus_t Serial_Print( uint8_t InterfaceId, char* pString, serialBlock_t allowToBlock )
{
#if gSerialManagerMaxInterfaces_c
    if ( allowToBlock )
    {
        return Serial_SyncWrite( InterfaceId, (uint8_t*)pString, strlen(pString) );
    }
    else
    {
        return Serial_AsyncWrite( InterfaceId, (uint8_t*)pString, strlen(pString), NULL, NULL );
    }
#else
    (void)pString;
    (void)allowToBlock;
    (void)InterfaceId;
    return gSerial_Success_c;
#endif
}

/*! *********************************************************************************
* \brief   Prints an number in hedadecimal format to the serial interface
*
* \param[in] InterfaceId the interface number
* \param[in] hex pointer to the number to be printed
* \param[in] len the number ob bytes of the number
* \param[in] flags specify display options: comma, space, new line
*
* \return The status of the operation
*
* \remarks The task will waituntil the tx has finished
*
********************************************************************************** */
serialStatus_t Serial_PrintHex( uint8_t InterfaceId,
                                uint8_t *hex,
                                uint8_t len,
                                uint8_t flags )
{
#if (gSerialManagerMaxInterfaces_c)
    uint8_t i=0;
    serialStatus_t status;
    uint8_t hexString[6]; /* 2 bytes  - hexadecimal display
    1 byte   - separator ( comma)
    1 byte   - separator ( space)
    2 bytes  - new line (\n\r)  */

    if ( !(flags & gPrtHexBigEndian_c) )
        hex = hex + (len-1);

    while ( len )
    {
        /* start preparing the print of a new byte */
        i=0;
        hexString[i++] = HexToAscii( (*hex)>>4 );
        hexString[i++] = HexToAscii( *hex );

        if ( flags & gPrtHexCommas_c )
        {
            hexString[i++] = ',';
        }
        if ( flags & gPrtHexSpaces_c )
        {
            hexString[i++] = ' ';
        }
        hex = hex + (flags & gPrtHexBigEndian_c ? 1 : -1);
        len--;

        if ( (len == 0) && (flags & gPrtHexNewLine_c) )
        {
            hexString[i++] = '\n';
            hexString[i++] = '\r';
        }

        /* transmit formatted byte */
        status = Serial_SyncWrite( InterfaceId, (uint8_t*)hexString, (uint8_t)i) ;
        if ( gSerial_Success_c != status )
            return status;
    }
#else
    /* Avoid compiler warning */
    (void)hex;
    (void)len;
    (void)InterfaceId;
    (void)flags;
#endif
    return gSerial_Success_c;
}

/*! *********************************************************************************
* \brief   Prints an unsigned integer to the serial interface
*
* \param[in] InterfaceId the interface number
* \param[in] nr the number to be printed
*
* \return The status of the operation
*
* \remarks The task will waituntil the tx has finished
*
********************************************************************************** */
serialStatus_t Serial_PrintDec( uint8_t InterfaceId, uint32_t nr )
{
#if (gSerialManagerMaxInterfaces_c)
#define gDecStringLen_d 12
    uint8_t i = gDecStringLen_d-1;
    uint8_t decString[gDecStringLen_d];

    if ( nr == 0 )
    {
        decString[i] = '0';
    }
    else
    {
        while ( nr )
        {
            decString[i] = '0' + (uint8_t)(nr % 10);
            nr = nr / 10;
            i--;
        }
        i++;
    }

    /* transmit formatted byte */
    return Serial_SyncWrite( InterfaceId, (uint8_t*)&decString[i], gDecStringLen_d-i );
#else
    (void)nr;
    (void)InterfaceId;
    return gSerial_Success_c;
#endif
}


/*! *********************************************************************************
* \brief   Configures the enabled hardware modules of the given interface type as a wakeup source from STOP mode  
*
* \param[in] interface type of the modules to configure
*
* \return  gSerial_Success_c if there is at least one module to configure
*          gSerial_InvalidInterface_c otherwise 
* \pre
*
* \post
*
* \remarks 
*
********************************************************************************** */

serialStatus_t Serial_EnableLowPowerWakeup( serialInterfaceType_t interfaceType )
{
    serialStatus_t status = gSerial_Success_c;
#if gSerialManagerMaxInterfaces_c
    uint8_t uartIdx = 0;

    switch(interfaceType)
    {
#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
    case gSerialMgrUart_c:
        while( uartIdx <= FSL_FEATURE_SOC_UART_COUNT-1 )
        {
            if(CLOCK_SYS_GetUartGateCmd(uartIdx))
            {
                UART_HAL_SetIntMode(g_uartBase[uartIdx], kUartIntRxActiveEdge, FALSE);
                UART_HAL_ClearStatusFlag(g_uartBase[uartIdx], kUartRxActiveEdgeDetect);
                UART_HAL_SetIntMode(g_uartBase[uartIdx], kUartIntRxActiveEdge, TRUE);
            }
            uartIdx++;
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
    case gSerialMgrLpuart_c:
        while( uartIdx <= FSL_FEATURE_SOC_LPUART_COUNT-1 )
        {
            if(CLOCK_SYS_GetLpuartGateCmd(uartIdx))
            {
                LPUART_HAL_SetIntMode(g_lpuartBase[uartIdx], kLpuartIntRxActiveEdge, FALSE);
                LPUART_HAL_ClearStatusFlag(g_lpuartBase[uartIdx], kLpuartRxActiveEdgeDetect);
                LPUART_HAL_SetIntMode(g_lpuartBase[uartIdx], kLpuartIntRxActiveEdge, TRUE);
            }
            uartIdx++;
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
    case gSerialMgrLpsci_c:
        while( uartIdx <= FSL_FEATURE_SOC_LPSCI_COUNT-1 )
        {
            if(CLOCK_SYS_GetLpsciGateCmd(uartIdx))
            {
                LPSCI_HAL_SetIntMode(g_lpsciBase[uartIdx], kLpsciIntRxActiveEdge, FALSE);
                LPSCI_HAL_ClearStatusFlag(g_lpsciBase[uartIdx], kLpsciRxActiveEdgeDetect);
                LPSCI_HAL_SetIntMode(g_lpsciBase[uartIdx], kLpsciIntRxActiveEdge, TRUE);
            }
            uartIdx++;
        }
        break;
#endif
#endif /* #if (gSerialMgrUseUart_c) */
    default:
        status = gSerial_InvalidInterface_c;
        break;
    }
#endif /* #if gSerialManagerMaxInterfaces_c */
    return status;
}

/*! *********************************************************************************
* \brief   Configures the enabled hardware modules of the given interface type as modules without wakeup capabilities  
*
* \param[in] interface type of the modules to configure
*
* \return  gSerial_Success_c if there is at least one module to configure 
*          gSerial_InvalidInterface_c otherwise 
* \pre
*
* \post
*
* \remarks 
*
********************************************************************************** */

serialStatus_t Serial_DisableLowPowerWakeup( serialInterfaceType_t interfaceType )
{
    serialStatus_t status = gSerial_Success_c;
#if gSerialManagerMaxInterfaces_c
    uint8_t uartIdx = 0;

    switch(interfaceType)
    {
#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
    case gSerialMgrUart_c:
        while( uartIdx <= FSL_FEATURE_SOC_UART_COUNT-1 )
        {
            if(CLOCK_SYS_GetUartGateCmd(uartIdx))
            {
                UART_HAL_SetIntMode(g_uartBase[uartIdx], kUartIntRxActiveEdge, FALSE);
                UART_HAL_ClearStatusFlag(g_uartBase[uartIdx], kUartRxActiveEdgeDetect);
            }
            uartIdx++;
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
    case gSerialMgrLpuart_c:
        while( uartIdx <= FSL_FEATURE_SOC_LPUART_COUNT-1 )
        {
            if(CLOCK_SYS_GetLpuartGateCmd(uartIdx))
            {
                LPUART_HAL_SetIntMode(g_lpuartBase[uartIdx], kLpuartIntRxActiveEdge, FALSE);
                LPUART_HAL_ClearStatusFlag(g_lpuartBase[uartIdx], kLpuartRxActiveEdgeDetect);
            }
            uartIdx++;
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
    case gSerialMgrLpsci_c:
        while( uartIdx <= FSL_FEATURE_SOC_LPSCI_COUNT-1 )
        {
            if(CLOCK_SYS_GetLpsciGateCmd(uartIdx))
            {
                LPSCI_HAL_SetIntMode(g_lpsciBase[uartIdx], kLpsciIntRxActiveEdge, FALSE);
                LPSCI_HAL_ClearStatusFlag(g_lpsciBase[uartIdx], kLpsciRxActiveEdgeDetect);
            }
            uartIdx++;
        }
        break;
#endif
#endif /* #if (gSerialMgrUseUart_c) */
    default:
        status = gSerial_InvalidInterface_c;
        break;
    }
#endif /* #if gSerialManagerMaxInterfaces_c */
    return status;
}

/*! *********************************************************************************
* \brief   Decides whether a enabled hardware module of the given interface type woke up the CPU from STOP mode.  
*
* \param[in] interface type of the modules to be evaluated as wakeup source.
*
* \return  TRUE if a module of the given interface type was the wakeup source
*          FALSE otherwise 
* \pre
*
* \post
*
* \remarks 
*
********************************************************************************** */

bool_t Serial_IsWakeUpSource( serialInterfaceType_t interfaceType)
{
#if gSerialManagerMaxInterfaces_c
    uint8_t uartIdx = 0;

    switch(interfaceType)
    {
#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
    case gSerialMgrUart_c:
        while( uartIdx <= FSL_FEATURE_SOC_UART_COUNT-1 )
        {
            if(CLOCK_SYS_GetUartGateCmd(uartIdx))
            {
                if( UART_HAL_GetStatusFlag(g_uartBase[uartIdx], kUartRxActiveEdgeDetect) )
                {
                    return TRUE;
                }  
            }
            uartIdx++;
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
    case gSerialMgrLpuart_c:
        while( uartIdx <= FSL_FEATURE_SOC_LPUART_COUNT-1 )
        {
            if(CLOCK_SYS_GetLpuartGateCmd(uartIdx))
            {
                if( LPUART_HAL_GetStatusFlag(g_lpuartBase[uartIdx], kLpuartRxActiveEdgeDetect) )
                {
                    return TRUE;
                }  
            }
            uartIdx++;
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
    case gSerialMgrLpsci_c:
        while( uartIdx <= FSL_FEATURE_SOC_LPSCI_COUNT-1 )
        {
            if(CLOCK_SYS_GetLpsciGateCmd(uartIdx))
            {
                if( LPSCI_HAL_GetStatusFlag(g_lpsciBase[uartIdx], kLpsciRxActiveEdgeDetect) )
                {
                    return TRUE;
                }  
            }
            uartIdx++;
        }
        break;
#endif
#endif /* #if (gSerialMgrUseUart_c) */
    default:
        break;
    }
#else
    (void)interfaceType;
#endif
    return FALSE;
}


/*! *********************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
********************************************************************************* */
#if (gSerialManagerMaxInterfaces_c)
/*! *********************************************************************************
* \brief Transmit a data buffer to the specified interface.
*
* \param[in] InterfaceId the interface number
*
* \return The status of the operation
*
********************************************************************************** */
static serialStatus_t Serial_WriteInternal( uint8_t InterfaceId )
{
    serialStatus_t status = gSerial_Success_c;
    serial_t *pSer = &mSerials[InterfaceId];
    uint16_t idx;

    OSA_EnterCritical(kCriticalDisableInt);
    if( pSer->state == 0 )
    {
        pSer->state = 1;
#if gSerialMgr_DisallowMcuSleep_d
        PWR_DisallowDeviceToSleep();
#endif
    }
    else
    {
        /* The interface is busy transmitting!
         * The current data will be transmitted after the previous transmissions end. 
         */
        OSA_ExitCritical(kCriticalDisableInt);
        return gSerial_Success_c;
    }
    OSA_ExitCritical(kCriticalDisableInt);

    idx = pSer->txCurrent;
    if(pSer->txQueue[idx].dataSize == 0)
    {
#if gSerialMgr_DisallowMcuSleep_d
        PWR_AllowDeviceToSleep();
#endif
        pSer->state = 0;
        return gSerial_Success_c;
    }

    switch ( mSerials[InterfaceId].serialType )
    {
#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
    case gSerialMgrUart_c:
        if( kStatus_UART_Success != UART_DRV_SendData( pSer->serialChannel, 
                                                       pSer->txQueue[idx].pData, 
                                                       pSer->txQueue[idx].dataSize ) )
        {
            status = gSerial_InternalError_c;
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPUART_COUNT
    case gSerialMgrLpuart_c:
        if( kStatus_LPUART_Success != LPUART_DRV_SendData( pSer->serialChannel, 
                                                           pSer->txQueue[idx].pData, 
                                                           pSer->txQueue[idx].dataSize ) )
        {
            status = gSerial_InternalError_c;
        }
        break;
#endif
#if FSL_FEATURE_SOC_LPSCI_COUNT
    case gSerialMgrLpsci_c:
        if( kStatus_LPSCI_Success != LPSCI_DRV_SendData( pSer->serialChannel, 
                                                         pSer->txQueue[idx].pData, 
                                                         pSer->txQueue[idx].dataSize ) )
        {
            status = gSerial_InternalError_c;
        }
        break;
#endif
#endif /* #if (gSerialMgrUseUart_c) */



    default:
        status = gSerial_InternalError_c;
    }

    if( status != gSerial_Success_c )
    {
#if gSerialMgr_DisallowMcuSleep_d
        PWR_AllowDeviceToSleep();
#endif
        pSer->txQueue[idx].dataSize = 0;
        pSer->txQueue[idx].txCallback = NULL;
        mSerial_IncIdx_d(pSer->txCurrent, gSerialMgrTxQueueSize_c);
        pSer->state = 0;
    }

    return status;
}
/*! *********************************************************************************
* \brief Inform the Serial Manager task that new data is available
*
* \param[in] pData The id interface
*
* \return none
*
* \remarks Called from ISR
*
********************************************************************************** */
void SerialManager_RxNotify( uint32_t i )
{
    serial_t *pSer = &mSerials[i];

    mSerial_IncIdx_d(pSer->rxIn, gSMRxBufSize_c);
    if(pSer->rxIn == pSer->rxOut)
    {
        mSerial_IncIdx_d(pSer->rxOut, gSMRxBufSize_c);
    }

    switch( pSer->serialType )
    {
        /* Uart driver is in continuous Rx. No need to restart reception. */
    default:
        break;
    }

    /* Signal SMGR task if not allready done */
    if( !(pSer->events & gSMGR_Rx_c) )
    {
        pSer->events |= gSMGR_Rx_c;
        (void)OSA_EventSet(&mSMTaskEvent, gSMGR_Rx_c);
    }
}

/*! *********************************************************************************
* \brief Inform the Serial Manager task that a transmission has finished
*
* \param[in] pData the Id interface
*
* \return none
*
* \remarks Called from ISR
*
********************************************************************************** */
void SerialManager_TxNotify( uint32_t i )
{
  
    serial_t *pSer = &mSerials[i];

    OSA_EnterCritical(kCriticalDisableInt);
    pSer->events |= gSMGR_TxDone_c;
    pSer->txQueue[pSer->txCurrent].dataSize = 0; //Mark as transmitted
    mSerial_IncIdx_d(pSer->txCurrent, gSerialMgrTxQueueSize_c);
#if gSerialMgr_DisallowMcuSleep_d
    PWR_AllowDeviceToSleep();
#endif
    pSer->state = 0;
    OSA_ExitCritical(kCriticalDisableInt);

    /* Transmit next block if available */
    if( pSer->txCurrent != pSer->txIn )
    {
       if( pSer->serialType != gSerialMgrIICMaster_c && pSer->serialType != gSerialMgrIICSlave_c )
        {
            (void)Serial_WriteInternal(i);
        }
    }
    else
    {
    }
    (void)OSA_EventSet(&mSMTaskEvent, gSMGR_TxDone_c);
}


/*! *********************************************************************************
* \brief   This function will mark all finished TX queue entries as empty.
*          If a calback was provided, it will be run.
*
* \param[in] pSer pointer to the serial interface internal structure
*
********************************************************************************** */
static void Serial_TxQueueMaintenance(serial_t *pSer)
{
    uint32_t i;

    while( pSer->txQueue[pSer->txOut].dataSize == 0 )
    {
        i = pSer->txOut;
        mSerial_IncIdx_d(pSer->txOut, gSerialMgrTxQueueSize_c);
        
        /* Run Calback */
        if( pSer->txQueue[i].txCallback )
        {
            pSer->txQueue[i].txCallback( pSer->txQueue[i].pTxParam );
            pSer->txQueue[i].txCallback = NULL;
        }

#if gSerialMgr_BlockSenderOnQueueFull_c && gSMGR_UseOsSemForSynchronization_c
        OSA_EnterCritical(kCriticalDisableInt);        
        if( pSer->txBlockedTasks )
        {
            pSer->txBlockedTasks--;
            OSA_ExitCritical(kCriticalDisableInt);
            (void)OSA_SemaPost(&pSer->txQueueSem);
        }
        else
        {
          OSA_ExitCritical(kCriticalDisableInt);
        }
#endif
        if( pSer->txOut == pSer->txIn )
            break;
    }
}

/*! *********************************************************************************
* \brief   This function will unblock the task who called Serial_SyncWrite().
*
* \param[in] pSer pointer to the serial interface internal structure
*
********************************************************************************** */
#if gSMGR_UseOsSemForSynchronization_c
static void Serial_SyncTxCallback(void *pSer)
{
    (void)OSA_SemaPost( &((serial_t *)pSer)->txSyncSem );
}
#endif

/*! *********************************************************************************
* \brief   This function will return the interfaceId for the specified interface
*
* \param[in] type     the interface type
* \param[in] channel  the instance of the interfacte
*
* \return The mSerials index for the specified interface type and channel
*
********************************************************************************** */
uint32_t Serial_GetInterfaceId(serialInterfaceType_t type, uint32_t channel)
{
    uint32_t i;
    
    for(i=0; i<gSerialManagerMaxInterfaces_c; i++)
    {
        if( (mSerials[i].serialType == type) && 
            (mSerials[i].serialChannel == channel) )
            return i;
    }

    return gSerialMgrInvalidIdx_c;
}

#if (gSerialMgrUseUart_c)
#if FSL_FEATURE_SOC_UART_COUNT
/*! *********************************************************************************
* \brief   Common UART ISR
*
********************************************************************************** */
void UART_IRQHandler(void)
{
    uint32_t instance = 0;
#if (FSL_FEATURE_SOC_UART_COUNT > 1)
    uint32_t uartIrqOffset = g_uartRxTxIrqId[1] - g_uartRxTxIrqId[0];
    instance = (__get_IPSR() - 16 - g_uartRxTxIrqId[0])/uartIrqOffset;
#endif
    
    UART_DRV_IRQHandler(instance);
}

/*! *********************************************************************************
* \brief   UART Rx ISR callback.
*
* \param[in] instance  UART instance
* \param[in] state     pointer to the UART state structure
*
********************************************************************************** */
void Serial_UartRxCb(uint32_t instance, void* state)
{
    uart_state_t *pState = (uart_state_t*)state;
    uint32_t i = (uint32_t)pState->rxCallbackParam;

    SerialManager_RxNotify(i);
    /* Update rxBuff because rxIn was incremented by the RxNotify function */
    pState->rxBuff = &mSerials[i].rxBuffer[mSerials[i].rxIn];
}

/*! *********************************************************************************
* \brief   UART Tx ISR callback.
*
* \param[in] instance  UART instance
* \param[in] state     pointer to the UART state structure
*
********************************************************************************** */
void Serial_UartTxCb(uint32_t instance, void* state)
{
    uart_state_t *pState = (uart_state_t*)state;
    uint32_t i = (uint32_t)pState->txCallbackParam;

    /* will get here only if txSize > 0 */
    pState->txBuff++;
    pState->txSize--;
    
    if( pState->txSize == 0 )
    {
        /* Transmit complete. Notify SMGR */
        UART_DRV_AbortSendingData(instance);
        SerialManager_TxNotify(i);
    }
}
#endif /* #if FSL_FEATURE_SOC_UART_COUNT */

#if FSL_FEATURE_SOC_LPUART_COUNT
/*! *********************************************************************************
* \brief   Common LPUART ISR.
*
********************************************************************************** */
void LPUART_IRQHandler(void)
{
    uint32_t instance = 0;
#if (FSL_FEATURE_SOC_LPUART_COUNT > 1)
    uint32_t lpuartIrqOffset = g_lpuartRxTxIrqId[1] - g_lpuartRxTxIrqId[0];
    instance = (__get_IPSR() - 16 - g_lpuartRxTxIrqId[0])/lpuartIrqOffset;
#endif
    
    LPUART_DRV_IRQHandler(instance);
}

/*! *********************************************************************************
* \brief   LPUART Rx ISR callback.
*
* \param[in] instance  LPUART instance
* \param[in] state     pointer to the LPUART state structure
*
********************************************************************************** */
void Serial_LpuartRxCb(uint32_t instance, void* state)
{
    lpuart_state_t *pState = (lpuart_state_t*)state;
    uint32_t i = (uint32_t)pState->rxCallbackParam;

    SerialManager_RxNotify(i);
    /* Update rxBuff because rxIn was incremented by the RxNotify function */
    pState->rxBuff = &mSerials[i].rxBuffer[mSerials[i].rxIn];
}

/*! *********************************************************************************
* \brief   LPUART Tx ISR callback.
*
* \param[in] instance  LPUART instance
* \param[in] state     pointer to the LPUART state structure
*
********************************************************************************** */
void Serial_LpuartTxCb(uint32_t instance, void* state)
{
    lpuart_state_t *pState = (lpuart_state_t*)state;
    uint32_t i = (uint32_t)pState->txCallbackParam;

    /* will get here only if txSize > 0 */
    pState->txBuff++;
    pState->txSize--;
    
    if( pState->txSize == 0 )
    {
        /* Transmit complete. Notify SMGR */
        LPUART_DRV_AbortSendingData(instance);
        SerialManager_TxNotify(i);
    }
}
#endif /* #if FSL_FEATURE_SOC_LPUART_COUNT */

#if FSL_FEATURE_SOC_LPSCI_COUNT
/*! *********************************************************************************
* \brief   Common LPSCI ISR.
*
********************************************************************************** */
void LPSCI_IRQHandler(void)
{
    uint32_t instance = 0;
#if (FSL_FEATURE_SOC_LPSCI_COUNT > 1)
    uint32_t lpsciIrqOffset = g_lpsciRxTxIrqId[1] - g_lpsciRxTxIrqId[0];
    instance = (__get_IPSR() - 16 - g_lpsciRxTxIrqId[0])/lpsciIrqOffset;
#endif

    LPSCI_DRV_IRQHandler(instance);
}

/*! *********************************************************************************
* \brief   LPSCI Rx ISR callback.
*
* \param[in] instance  LPSCI instance
* \param[in] state     pointer to the LPSCI state structure
*
********************************************************************************** */
void Serial_LpsciRxCb(uint32_t instance, void* state)
{
    lpsci_state_t *pState = (lpsci_state_t*)state;
    uint32_t i = (uint32_t)pState->rxCallbackParam;

    SerialManager_RxNotify(i);
    /* Update rxBuff because rxIn was incremented by the RxNotify function */
    pState->rxBuff = &mSerials[i].rxBuffer[mSerials[i].rxIn];
}

/*! *********************************************************************************
* \brief   LPSCI Tx ISR callback.
*
* \param[in] instance  LPSCI instance
* \param[in] state     pointer to the LPSCI state structure
*
********************************************************************************** */
void Serial_LpsciTxCb(uint32_t instance, void* state)
{
    lpsci_state_t *pState = (lpsci_state_t*)state;
    uint32_t i = (uint32_t)pState->txCallbackParam;

    /* will get here only if txSize > 0 */
    pState->txBuff++;
    pState->txSize--;
    
    if( pState->txSize == 0 )
    {
        /* Transmit complete. Notify SMGR */
        LPSCI_DRV_AbortSendingData(instance);
        SerialManager_TxNotify(i);
    }
}
#endif /* #if FSL_FEATURE_SOC_LPSCI_COUNT */
#endif /* #if (gSerialMgrUseUart_c) */
#endif /* #if (gSerialManagerMaxInterfaces_c) */
