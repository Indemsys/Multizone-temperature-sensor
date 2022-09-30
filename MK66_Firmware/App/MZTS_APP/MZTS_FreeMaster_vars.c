#ifndef __FREEMASTER_VARS
  #define __FREEMASTER_VARS
#include "App.h"
#include   "freemaster_tsa.h"

FMSTR_TSA_TABLE_BEGIN(wvar_vars)
FMSTR_TSA_RW_VAR( wvar.max_sens_log_file_size          ,FMSTR_TSA_UINT32    ) // Maximal log file size (byte) | def.val.= 1000000
FMSTR_TSA_RW_VAR( wvar.pin_code                        ,FMSTR_TSA_UINT32    ) // Pin code | def.val.= 123456
FMSTR_TSA_RW_VAR( wvar.screen_rot                      ,FMSTR_TSA_UINT8     ) // Screen rotation (0-0, 1- 90, 2-180, 3-270) | def.val.= 0
FMSTR_TSA_TABLE_END();


#endif
