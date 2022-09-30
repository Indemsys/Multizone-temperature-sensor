#include "app.h"

#if (MODBUS_CFG_FC01_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Возвращаем значение бита coil  
  
  \param coil  
  \param perr   MODBUS_ERR_NONE     the specified coil is valid and you are returning its value.
                MODBUS_ERR_RANGE    the specified coil is an invalid coil number in your 
                                    application (i.e. product).  YOUR product defines what the
                                    valid range of values is for the 'coil' argument.
  
  \return uint8_t 
-----------------------------------------------------------------------------------------------------*/
uint8_t  MB_CoilRd(uint16_t   coil, uint16_t  *perr)
{
  uint8_t  coil_val = 0;

  switch (coil)
  {
  case 0:
    coil_val = 1;
    break;
  }

  *perr = MODBUS_ERR_NONE;
  return (coil_val);
}
#endif

#if (MODBUS_CFG_FC05_EN == 1) || (MODBUS_CFG_FC15_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  Меняем значение бита 
  
  \param coil  
  \param coil_val  
  \param perr    is a pointer to an error code variable.  You must either return:

                        MODBUS_ERR_NONE     the specified coil is valid and your code changed the value 
                                            of the coil.
                        MODBUS_ERR_RANGE    the specified coil is an invalid coil number in your 
                                            application (i.e. product).  YOUR product defines what the
                                            valid range of values is for the 'coil' argument.
                        MODBUS_ERR_WR       if the device is not able to write or accept the value
-----------------------------------------------------------------------------------------------------*/
void  MB_CoilWr(uint16_t    coil, uint8_t   coil_val, uint16_t   *perr)
{
  (void)coil;
  (void)coil_val;
  *perr = MODBUS_ERR_NONE;
}
#endif


#if (MODBUS_CFG_FC02_EN == 1)
/*-----------------------------------------------------------------------------------------------------
 This function reads the value of a single DI (DI means Discrete Input).      
  
 Arguments  : di        is the Discrete Input number that needs to be read.

              perr      is a pointer to an error code variable.  You must either return:

                        MODBUS_ERR_NONE     the specified DI is valid and your code is returning its
                                            current value.
                        MODBUS_ERR_RANGE    the specified DI is an invalid Discrete Input number in your 
                                            application (i.e. product).  YOUR product defines what the
                                            valid range of values is for the 'di' argument.

-----------------------------------------------------------------------------------------------------*/
uint8_t  MB_DIRd(uint16_t   di, uint16_t  *perr)
{
  (void)di;
  *perr = MODBUS_ERR_NONE;
  return (0);
}
#endif

#if (MODBUS_CFG_FC04_EN == 1)
/*-----------------------------------------------------------------------------------------------------
 This function reads the value of a single Input Register.                                            
 
 Arguments  : reg       is the Input Register number that needs to be read.

              perr      is a pointer to an error code variable.  You must either return:

                        MODBUS_ERR_NONE     the specified input register is valid and your code is 
                                            returning its current value.
                        MODBUS_ERR_RANGE    the specified input register is an invalid number in your 
                                            application (i.e. product).  YOUR product defines what the
                                            valid range of values is for the 'reg' argument.
-----------------------------------------------------------------------------------------------------*/
uint16_t  MB_InRegRd(uint16_t   reg, uint16_t  *perr)
{
  uint16_t  val;

  switch (reg)
  {
  case 10:
    //val = some_variable;
    break;

  default:
    val = 0;
    break;
  }
  *perr = MODBUS_ERR_NONE;
  return (val);
}
#endif

#if (MODBUS_CFG_FP_EN   == 1)
  #if (MODBUS_CFG_FC04_EN == 1)
/*-----------------------------------------------------------------------------------------------------
 This function reads the value of a single Input Register.                                    
 
 Arguments  : reg       is the Input Register number that needs to be read.

              perr      is a pointer to an error code variable.  You must either return:

                        MODBUS_ERR_NONE     the specified input register is valid and your code is 
                                            returning its current value.
                        MODBUS_ERR_RANGE    the specified input register is an invalid number in your 
                                            application (i.e. product).  YOUR product defines what the
                                            valid range of values is for the 'reg' argument.
-----------------------------------------------------------------------------------------------------*/
float  MB_InRegRdFP(uint16_t   reg, uint16_t  *perr)
{
  (void)reg;
  *perr = MODBUS_ERR_NONE;
  return ((float)0);
}
  #endif
#endif


#if (MODBUS_CFG_FC03_EN == 1)
/*-----------------------------------------------------------------------------------------------------
 This function reads the value of a single Holding Register.                                     
 
 Arguments  : reg       is the Holding Register number that needs to be read.

              perr      is a pointer to an error code variable.  You must either return:

                        MODBUS_ERR_NONE     the specified holding register is valid and your code is 
                                            returning its current value.
                        MODBUS_ERR_RANGE    the specified holding register is an invalid number in your 
                                            application (i.e. product).  YOUR product defines what the
                                            valid range of values is for the 'reg' argument.

-----------------------------------------------------------------------------------------------------*/
uint16_t  MB_HoldingRegRd(uint16_t   reg, uint16_t  *perr)
{
  uint16_t  val;

  switch (reg)
  {
  case 0:
    //val = some_variable;
    break;


  default:
    val = 0;
    break;
  }
  *perr = MODBUS_ERR_NONE;
  return (val);
}
#endif

#if (MODBUS_CFG_FP_EN   == 1)
  #if (MODBUS_CFG_FC03_EN == 1)
/*-----------------------------------------------------------------------------------------------------
 This function reads the value of a single Floating-Point Holding Register.                    
  
 Arguments  : reg       is the Holding Register number that needs to be read.

              perr      is a pointer to an error code variable.  You must either return:

                        MODBUS_ERR_NONE     the specified holding register is valid and your code is 
                                            returning its current value.
                        MODBUS_ERR_RANGE    the specified holding register is an invalid number in your 
                                            application (i.e. product).  YOUR product defines what the
                                            valid range of values is for the 'reg' argument.
-----------------------------------------------------------------------------------------------------*/
float  MB_HoldingRegRdFP(uint16_t   reg, uint16_t  *perr)
{
  (void)reg;
  *perr = MODBUS_ERR_NONE;
  return ((float)0);
}
  #endif
#endif

#if (MODBUS_CFG_FC06_EN == 1) || (MODBUS_CFG_FC16_EN == 1)
/*-----------------------------------------------------------------------------------------------------
 
 This function is called to change the value of a single Integer Holding Register.

 Arguments  : reg       is the Holding Register number that needs to be read.

              reg_val   is the desired value of the holding register.
                        The value is specified as an unsigned integer even though it could actually be
                        represented by a signed integer.

              perr      is a pointer to an error code variable.  You must either return:

                        MODBUS_ERR_NONE     the specified holding register is valid and your code is 
                                            returning its current value.
                        MODBUS_ERR_RANGE    the specified holding register is an invalid number in your 
                                            application (i.e. product).  YOUR product defines what the
                                            valid range of values is for the 'reg' argument.
                        MODBUS_ERR_WR       if the device is not able to write or accept the value
-----------------------------------------------------------------------------------------------------*/
void  MB_HoldingRegWr(uint16_t   reg, uint16_t   reg_val_16, uint16_t  *perr)
{
  (void)reg;
  (void)reg_val_16;
  *perr = MODBUS_ERR_NONE;
}
#endif

#if (MODBUS_CFG_FP_EN    == 1)
  #if (MODBUS_CFG_FC06_EN == 1) || (MODBUS_CFG_FC16_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  This function is called to change the value of a single Floating-Point Holding Register.

  Arguments  : reg       is the Holding Register number that needs to be read.

              reg_val   is the desired value of the holding register.
                        The value is specified as an unsigned integer even though it could actually be
                        represented by a signed integer.

              perr      is a pointer to an error code variable.  You must either return:

                        MODBUS_ERR_NONE     the specified holding register is valid and your code is 
                                            returning its current value.
                        MODBUS_ERR_RANGE    the specified holding register is an invalid number in your 
                                            application (i.e. product).  YOUR product defines what the
                                            valid range of values is for the 'reg' argument.
                        MODBUS_ERR_WR       if the device is not able to write or accept the value
-----------------------------------------------------------------------------------------------------*/
void  MB_HoldingRegWrFP(uint16_t   reg, float     reg_val_fp, uint16_t  *perr)
{
  (void)reg;
  (void)reg_val_fp;
  *perr = MODBUS_ERR_RANGE;
}
  #endif
#endif


#if (MODBUS_CFG_FC20_EN == 1)
/*-----------------------------------------------------------------------------------------------------
 This function is called to read a single integer from a file.  
  
 Arguments  : file_nbr    is the number of the desired file.

              record_nbr  is the desired record within the file

              ix          is the desired entry in the specified record.

              record_len  is the desired length of the record.  Note that this parameter is passed to
                          this function to provide the 'requested' requested length from the MODBUS command.

              perr        is a pointer to an error code variable.  You must either return:

                          MODBUS_ERR_NONE     the specified file/record/entry is valid and your code is returning its current value.
                          MODBUS_ERR_FILE     if the specified 'file_nbr' is not a valid file number in your product.
                          MODBUS_ERR_RECORD   if the specified 'record_nbr' is not a valid record in the specified file.
                          MODBUS_ERR_IX       if the specified 'ix' is not a valid index into the specified
-----------------------------------------------------------------------------------------------------*/
uint16_t  MB_FileRd(uint16_t   file_nbr, uint16_t   record_nbr, uint16_t   ix, uint8_t   record_len, uint16_t  *perr)
{
  (void)file_nbr;
  (void)record_nbr;
  (void)ix;
  (void)record_len;
  *perr  = MODBUS_ERR_NONE;
  return (0);
}
#endif


#if (MODBUS_CFG_FC21_EN == 1)
/*-----------------------------------------------------------------------------------------------------
  This function is called to change a single integer value in a file.  
  
 Arguments  : file_nbr    is the number of the desired file.

              record_nbr  is the desired record within the file

              ix          is the desired entry in the specified record.

              record_len  is the desired length of the record.  Note that this parameter is passed to
                          this function to provide the 'requested' requested length from the MODBUS command.

              val         is the new value to place in the file.

              perr        is a pointer to an error code variable.  You must either return:

                          MODBUS_ERR_NONE     the specified file/record/entry is valid and your code is returning its current value.
                          MODBUS_ERR_FILE     if the specified 'file_nbr' is not a valid file number in your product.
                          MODBUS_ERR_RECORD   if the specified 'record_nbr' is not a valid record in the specified file.
                          MODBUS_ERR_IX       if the specified 'ix' is not a valid index into the specified
                                              record.
-----------------------------------------------------------------------------------------------------*/
void  MB_FileWr(uint16_t   file_nbr, uint16_t   record_nbr, uint16_t   ix, uint8_t   record_len, uint16_t   val, uint16_t  *perr)
{
  (void)file_nbr;
  (void)record_nbr;
  (void)ix;
  (void)record_len;
  (void)val;
  *perr = MODBUS_ERR_NONE;
}
#endif

