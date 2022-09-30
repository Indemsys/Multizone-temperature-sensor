#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <intrinsics.h>

#include "EmbeddedTypes.h"
#include "BSP.h"

#include "SEGGER_RTT.h"
#include "App_Debug_print.h"

#include "MKW40Z4_features.h"
#include "MKW40Z4_extension.h"
#include "fsl_sim_hal_MKW40Z4.h"
#include "fsl_clock_manager.h"
#include "fsl_clock_MKW40Z4.h"
#include "fsl_osa_ext.h"
#include "fsl_os_abstraction.h"
#include "fsl_device_registers.h"
#include "fsl_interrupt_manager.h"

#include "MemManager.h"
#include "TimersManager.h"
#include "RNG_Interface.h"
#include "PWR_Interface.h"
#include "Flash_Adapter.h"
#include "SecLib.h"
#include "GenericList.h"
#include "FunctionLib.h"
#include "Messaging.h"
#include "Panic.h"


#include "KW4xXcvrDrv.h"
#include "ble_controller_task_config.h"
#include "controller_interface.h"

/* BLE Host Stack */
#include "l2ca_interface.h"
#include "gatt_interface.h"
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gatt_database.h"
#include "gap_interface.h"
#include "gatt_db_app_interface.h"
#include "gatt_db_handles.h"

/* Profile / Services */
#include "battery_interface.h"
#include "device_info_interface.h"
#include "heart_rate_interface.h"

#include "BLE_common_app.h"
#include "BLE_peripheral_app.h"
#include "ble_host_tasks.h"

#include "BLE_Perfomance_Test.h"
#include "MKW40_comm_channel_IDs.h"
#include "App_comm_channel.h"
#include "App_CommandService.h"
#include "App_CommunicationService.h"
#include "CRC_utils.h"


#define RESULT_OK      0
#define RESULT_ERROR  -1


#define EVENT_FROM_HOST_STACK       0x0001
#define EVENT_SETTINGS_DONE         0x0002

typedef struct 
{
  uint32_t in_use;
  event_t  os_event;
}T_event_obj;

typedef struct 
{
  void     *events_obj_array;
  uint32_t  event_obj_size;
  uint32_t  events_number;
} T_events_arr_descriptor;



extern void (*pfBLE_SignalFromISR)(void);


#endif // APP_H



