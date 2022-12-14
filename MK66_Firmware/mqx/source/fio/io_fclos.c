
/*HEADER**********************************************************************
*
* Copyright 2008 Freescale Semiconductor, Inc.
* Copyright 2004-2008 Embedded Access Inc.
* Copyright 1989-2008 ARC International
*
* This software is owned or controlled by Freescale Semiconductor.
* Use of this software is governed by the Freescale MQX RTOS License
* distributed with this Material.
* See the MQX_RTOS_LICENSE file distributed for more details.
*
* Brief License Summary:
* This software is provided in source form for you to use free of charge,
* but it is not open source software. You are allowed to use this software
* but you cannot redistribute it or derivative works of it in source form.
* The software may be used only in connection with a product containing
* a Freescale microprocessor, microcontroller, or digital signal processor.
* See license agreement file for full license terms including other restrictions.
*****************************************************************************
*
* Comments:
*
*   Contains the function fclose.
*
*
*END************************************************************************/
#include "mqx_cnfg.h"
#if MQX_USE_IO_OLD

#include "mqx_inc.h"
#include "fio.h"
#include "fio_prv.h"
#include "io.h"
#include "io_prv.h"

/*!
 * \brief Calls the close function for the device.
 *
 * \param[in] file_ptr The stream to close.
 *
 * \return Value returned by close function.
 * \return IO_EOF (Failure.)
 */
_mqx_int _io_fclose
   (
      MQX_FILE_PTR file_ptr
   )
{ /* Body */
/*   KERNEL_DATA_STRUCT_PTR kernel_data; */
   IO_DEVICE_STRUCT_PTR   dev_ptr;
   _mqx_uint               result;

/*   _GET_KERNEL_DATA(kernel_data); */

#if MQX_CHECK_ERRORS
   if (file_ptr == NULL) {
      return(IO_EOF);
   } /* Endif */
#endif

   dev_ptr = file_ptr->DEV_PTR;
#if MQX_CHECK_ERRORS
   if ((dev_ptr == NULL) || (dev_ptr->IO_CLOSE == NULL)) {
      _mem_free(file_ptr);
      return(IO_EOF);
   } /* Endif */
#endif

   result = (*dev_ptr->IO_CLOSE)(file_ptr);
   if (result == MQX_OK) {
      _mem_free(file_ptr);
   }

   return(result);

} /* Endbody */

#endif // MQX_USE_IO_OLD
