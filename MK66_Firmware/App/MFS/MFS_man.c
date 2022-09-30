#include "App.h"
#include "mfs_prv.h"

MQX_FILE_PTR           com_handle;
MQX_FILE_PTR           partition_handle;
MQX_FILE_PTR           g_sdcard_handle;
MQX_FILE_PTR           g_filesystem_handle;
uint64_t               g_card_size;


/*-------------------------------------------------------------------------------------------------------------
  Инициализация файловой системы MFS
-------------------------------------------------------------------------------------------------------------*/
_mqx_int Init_mfs(void)
{
  uint32_t        i;
  _mqx_int        res;

  g_sd_card_status = SD_CARD_OK;

  com_handle = fopen("esdhc:", 0);
  if (NULL == com_handle)
  {
    g_sd_card_status = SD_CARD_ERROR1;
    LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Error opening communication handle 'esdhc:.'");
    return MQX_ERROR;
  }

  res = _io_sdcard_install("sdcard:", (void *)&_bsp_sdcard0_init, com_handle);
  if (res != MQX_OK)
  {
    g_sd_card_status = SD_CARD_ERROR2;
    LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Error installing SD card device (0x%x).", res);
    return MQX_ERROR;
  }

  for (i=0; i < 5; i++)
  {
    g_sdcard_handle = fopen("sdcard:", 0);
    if (g_sdcard_handle != NULL) break;
    g_sd_card_status = SD_CARD_ERROR3;
    LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Unable to open SD card device.");
    _time_delay(100);
  }
  if (i == 5) return MQX_ERROR;
  g_sd_card_status = SD_CARD_OK;

   /* Install partition manager over SD card driver */
  res = _io_part_mgr_install(g_sdcard_handle, PARTMAN_NAME, 0);
  if (res != MFS_NO_ERROR)
  {
    g_sd_card_status = SD_CARD_FS_ERROR1;
    LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Error installing partition manager: %s", MFS_Error_text((uint32_t)res));
    return MQX_ERROR;
  }

  partition_handle = fopen(PARTITION_NAME, NULL);
  if (partition_handle != NULL)
  {
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT  , "Installing MFS over partition...");

      /* Validate partition */
    res = _io_ioctl(partition_handle, IO_IOCTL_VAL_PART, NULL);
    if (res != MFS_NO_ERROR)
    {
      g_sd_card_status = SD_CARD_FS_ERROR2;
      LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Error validating partition: %s", MFS_Error_text((uint32_t)res));
      LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Not installing MFS.");
      return MQX_ERROR;
    }

      /* Install MFS over partition */
    res = _io_mfs_install(partition_handle, DISK_NAME, 0);
    if (res != MFS_NO_ERROR)
    {
      g_sd_card_status = SD_CARD_FS_ERROR3;
      LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Error initializing MFS over partition: %s\n", MFS_Error_text((uint32_t)res));
      return  MQX_ERROR;
    }

  }
  else
  {

    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT  , "Installing MFS over SD card driver...");

      /* Install MFS over SD card driver */
    res = _io_mfs_install(g_sdcard_handle, DISK_NAME, (_file_size)0);
    if (res != MFS_NO_ERROR)
    {
      g_sd_card_status = SD_CARD_FS_ERROR4;
      LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Error initializing MFS: %s", MFS_Error_text((uint32_t)res));
      return  MQX_ERROR;
    }
  }

   /* Open file system */
  if (res == MFS_NO_ERROR)
  {
    g_filesystem_handle = fopen(DISK_NAME, NULL);
    res = ferror(g_filesystem_handle);
    if (res == MFS_NOT_A_DOS_DISK)
    {
      g_sd_card_status = SD_CARD_FS_ERROR5;
      LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "NOT A DOS DISK! You must format to continue.");
      return MQX_ERROR;
    }
    else if (res != MFS_NO_ERROR)
    {
      g_sd_card_status = SD_CARD_FS_ERROR6;
      LOGs(__FUNCTION__, __LINE__, SEVERITY_RED  , "Error opening filesystem: %s", MFS_Error_text((uint32_t)res));
      return MQX_ERROR;
    }
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT  , "SD card installed to %s", DISK_NAME);

    MQX_FILE  *mf = g_filesystem_handle;
    MFS_DRIVE_STRUCT_PTR drive_ptr = (MFS_DRIVE_STRUCT_PTR)mf->DEV_PTR->DRIVER_INIT_PTR;
    g_card_size = (uint64_t)drive_ptr->LAST_CLUSTER * (uint64_t)drive_ptr->CLUSTER_SIZE_BYTES;
    LOGs(__FUNCTION__, __LINE__, SEVERITY_DEFAULT  , "SD card space %lld Byte", g_card_size );

  }

  return MQX_OK;
}


