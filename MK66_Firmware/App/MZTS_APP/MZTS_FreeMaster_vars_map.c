#include "App.h"
#include "freemaster_cfg.h"
#include "freemaster.h"
#include "freemaster_tsa.h"


FMSTR_TSA_TABLE_BEGIN(app_vars)

FMSTR_TSA_RW_VAR( cpu_usage                            ,FMSTR_TSA_UINT32    )
FMSTR_TSA_RW_VAR( g_aver_cpu_usage                     ,FMSTR_TSA_UINT32    )
FMSTR_TSA_RW_VAR(app.temperatures[0]                   ,FMSTR_TSA_FLOAT     )
FMSTR_TSA_RW_VAR(app.temperatures[1]                   ,FMSTR_TSA_FLOAT     )
FMSTR_TSA_RW_VAR(app.temperatures[2]                   ,FMSTR_TSA_FLOAT     )
FMSTR_TSA_RW_VAR(app.temperatures[3]                   ,FMSTR_TSA_FLOAT     )
FMSTR_TSA_RW_VAR(app.temperatures[4]                   ,FMSTR_TSA_FLOAT     )
FMSTR_TSA_RW_VAR(app.temperatures[5]                   ,FMSTR_TSA_FLOAT     )
FMSTR_TSA_RW_VAR(app.temperatures[6]                   ,FMSTR_TSA_FLOAT     )
FMSTR_TSA_RW_VAR(app.temperatures[7]                   ,FMSTR_TSA_FLOAT     )

FMSTR_TSA_RW_VAR(app.sensors_state[0]                  ,FMSTR_TSA_UINT8     )
FMSTR_TSA_RW_VAR(app.sensors_state[1]                  ,FMSTR_TSA_UINT8     )
FMSTR_TSA_RW_VAR(app.sensors_state[2]                  ,FMSTR_TSA_UINT8     )
FMSTR_TSA_RW_VAR(app.sensors_state[3]                  ,FMSTR_TSA_UINT8     )
FMSTR_TSA_RW_VAR(app.sensors_state[4]                  ,FMSTR_TSA_UINT8     )
FMSTR_TSA_RW_VAR(app.sensors_state[5]                  ,FMSTR_TSA_UINT8     )
FMSTR_TSA_RW_VAR(app.sensors_state[6]                  ,FMSTR_TSA_UINT8     )
FMSTR_TSA_RW_VAR(app.sensors_state[7]                  ,FMSTR_TSA_UINT8     )

FMSTR_TSA_RW_VAR(app.temperature_aver                  ,FMSTR_TSA_FLOAT     )

FMSTR_TSA_TABLE_END();

