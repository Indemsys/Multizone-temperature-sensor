/*! *********************************************************************************
 * \defgroup BLE_CONFIG BLE - Configuration Constants
 * @{
 ********************************************************************************** */
/*!
* Copyright (c) 2014, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file ble_constants.h
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

#ifndef _BLE_CONSTANTS_H_
#define _BLE_CONSTANTS_H_

/************************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
************************************************************************************/
/*! The stack is compatible with the Bluetooth<sup>??</sup> Specification 4.1 for Low Energy (BLE) */
#define gBLE41_d                        (TRUE)

/*! The stack is compatible with the Bluetooth Specification 4.2 for Low Energy */
#define gBLE42_d                        (FALSE)

/*! Size of a BLE Device Address */
#define gcBleDeviceAddressSize_c        (6)

 /*! Maximum number of simultaneously active connections */
#define gcGapMaximumActiveConnections_c (1)

/*! Maximum number of bonded devices */
#define gcGapMaximumBondedDevices_d     (2)

/*! Maximum number of CCCDs */
#define gcGapMaximumSavedCccds_c        (10)
     
/*! Maximum number of attributes that require authorization */
#define gcGapMaxAuthorizationHandles_c  (1)

/*! Maximum number of gapServiceSecurityRequirements_t structures that can be registered
 * with Gap_RegisterDeviceSecurityRequirements() */
#define gcGapMaxServiceSpecificSecurityRequirements_c   (3)

/*! Whether or not this stack has support for BR/EDR */
#define gHciBrEdrCommandSupport_d       (FALSE)

#define gMwsIsSupported_d               (FALSE)

#define gLePingIsSupported_d                            (FALSE)
#define gLeDataPacketLengthExtensionSupport_d           (FALSE)
#define gLlPrivacySupport_d                             (FALSE)

/*! Size of long UUIDs */
#define gcBleLongUuidSize_c             (16)

/*! Maximum Long Term Key size in bytes */
#define gcSmpMaxLtkSize_c               (16)

/*! Identity Resolving Key size in bytes */
#define gcSmpIrkSize_c                  (16)

/*! Connection Signature Resolving Key size in bytes */
#define gcSmpCsrkSize_c                 (16)

/*! Maximum Rand size in bytes */
#define gcSmpMaxRandSize_c              (8)

/*! SMP OOB size in bytes */
#define gcSmpOobSize_c  (16)

/*! Maximum device name size */
#define gcGapMaxDeviceNameSize_c        (40)

/*! Maximum size of advertising and scan response data */
#define gcGapMaxAdvertisingDataLength_c (31)

/*! Default value of the ATT_MTU */
#define gAttDefaultMtu_c                (23)

/*! Maximum possible value of the ATT_MTU for this device. This is used during the MTU Exchange. */
#define gAttMaxMtu_c                    (23)

/*! Maximum number of simultaneous GATT Clients allowed to use Prepare Write Queues */
#define gcGattDbMaxPrepareWriteClients_c    gcGapMaximumActiveConnections_c

/*! Maximum Flash area available for bonded devices data */
#define gcReservedFlashSizeForBondedDevicesData_c   (1024)

/*! Channel the number of the UART hardware module (For example, if UART1 is used, this value should be 1). */
#define gHciTransportUartChannel_c                  (1)

/*! Number of bytes reserved for storing application-specific information about a device */
#define gcReservedFlashSizeForCustomInformation_c   (50)

/*! Number of handles that may be registered for write notifications */
#define gcGattMaxHandleCountForWriteNotifications_c (10)

/*! Number of handles that may be registered for read notifications */
#define gcGattMaxHandleCountForReadNotifications_c  (10)

#if 0
/*! Use the toolchain specific utility functions */
#define gUseToolchainMemFunc_d 1
#endif

#define gBleHciUsesSerialIf_d           (TRUE)

#endif /* _BLE_CONSTANTS_H_ */

