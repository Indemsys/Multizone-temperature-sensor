/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file MemManager.c
* This is the source file for the Memory Manager.
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
#include "App.h"

T_pools_list g_pools_list[POOLS_NUMBER+1]  =
{
  { POOL_1_BLOCKS_SIZE , POOL_1_BLOCKS_NUMBER },
  { POOL_2_BLOCKS_SIZE , POOL_2_BLOCKS_NUMBER },
  { POOL_3_BLOCKS_SIZE , POOL_3_BLOCKS_NUMBER },
  {0, 0} //termination tag
};

// Heap
uint8_t           pools_memory[POOLS_SIZE];
const uint32_t    heapSize  = POOLS_SIZE;

// Memory pool info and anchors.
T_pool_descriptor g_pool_descriptors[POOLS_NUMBER];


// Free messages counter. Not used by module.
uint16_t          gFreeMessagesCount;

/*-----------------------------------------------------------------------------------------------------
   С помощью этой функции следим за свободным местом в пуле блоков памяти
   В случае если пул не имеет достаточно места стек BLE может зависнуть
 
 \param void 
 
 \return int32_t 
-----------------------------------------------------------------------------------------------------*/
uint32_t Get_boggest_pool_free_block_cnt(void)
{
  uint32_t n;
  n = g_pool_descriptors[POOLS_NUMBER - 1].statistic.numBlocks - g_pool_descriptors[POOLS_NUMBER - 1].statistic.allocatedBlocks;
  return n; 
}

/*! *********************************************************************************
* \brief   This function initializes the message module private variables. 
*          Must be called at boot time, or if device is reset.
*
* \param[in] none
*
* \return MEM_SUCCESS_c if initialization is successful. (It's always successful).
*
********************************************************************************** */
memStatus_t MEM_Init()
{
  T_pools_list      *pools_list = g_pools_list; // IN: Memory layout information
  T_pool_descriptor *pool       = g_pool_descriptors; // OUT: Will be initialized with requested memory pools.
  uint8_t           *memory     = pools_memory; // IN: Memory heap.

  uint8_t           poolN;

  gFreeMessagesCount = 0;

  for (;;)
  {
    poolN = pools_list->blocks_number;
    ListInit((listHandle_t)&pool->anchor, poolN);
    #ifdef MEM_MANAGER_STATISTICS
    pool->statistic.numBlocks = 0;
    pool->statistic.allocatedBlocks = 0;
    pool->statistic.allocatedBlocksPeak = 0;
    pool->statistic.allocationFailures = 0;
    pool->statistic.freeFailures = 0;
    #endif /*MEM_STATISTICS*/

    while (poolN)
    {
      // Add block to list of free memory.
      ListAddTail((listHandle_t)&pool->anchor, (listElementHandle_t)&((T_pool_block_header *)memory)->link);
      ((T_pool_block_header *)memory)->pParentPool = pool;
      #ifdef MEM_MANAGER_STATISTICS
      pool->statistic.numBlocks++;
      #endif /*MEM_STATISTICS*/

      gFreeMessagesCount++;

      // Add block size (without list header)
      memory += pools_list->block_size + sizeof(T_pool_block_header);
      poolN--;
    }

    pool->block_size = pools_list->block_size;
    pool->next_pool_block_size = (pools_list + 1)->block_size;
    if (pool->next_pool_block_size == 0)
    {
      break;
    }

    pool++;
    pools_list++;
  }

  return MEM_SUCCESS_c;
}

/*! *********************************************************************************
* \brief    This function returns the number of available blocks greater or 
*           equal to the given size.
*
* \param[in] size - Size of blocks to check for availability.
*
* \return Number of available blocks greater or equal to the given size.
*
* \pre Memory manager must be previously initialized.
*
********************************************************************************** */
uint32_t MEM_GetAvailableBlocks(uint32_t size)
{
  T_pool_descriptor *pPools     = g_pool_descriptors;
  uint32_t          pTotalCount = 0;

  for (;;)
  {
    if (size <= pPools->block_size)
    {
      pTotalCount += ListGetSize((listHandle_t)&pPools->anchor);
    }

    if (pPools->next_pool_block_size == 0)
    {
      break;
    }

    pPools++;
  }
  return  pTotalCount;
}

/*! *********************************************************************************
* \brief     Allocate a block from the memory pools. The function uses the 
*            numBytes argument to look up a pool with adequate block sizes.
* \param[in] numBytes - Size of buffer to allocate.
*
* \return Pointer to the allocated buffer, NULL if failed.
*
* \pre Memory manager must be previously initialized.
*
********************************************************************************** */
void* MEM_BufferAlloc(uint32_t numBytes)
{

  T_pool_descriptor   *pool           = g_pool_descriptors;
  T_pool_block_header *p_block_header;



  OSA_EnterCritical(kCriticalDisableInt);

  DEBUG_MEM_MAN_PRINT_ARG("+%d\r\n", numBytes);
  while (numBytes)
  {
    if (numBytes <= pool->block_size)
    {
      p_block_header = (T_pool_block_header *)ListRemoveHead((listHandle_t)&pool->anchor);

      if (NULL != p_block_header)
      {
        p_block_header++;
        gFreeMessagesCount--;

        #ifdef MEM_MANAGER_STATISTICS
        pool->statistic.allocatedBlocks++;
        if (pool->statistic.allocatedBlocks > pool->statistic.allocatedBlocksPeak)
        {
          pool->statistic.allocatedBlocksPeak = pool->statistic.allocatedBlocks;
        }
        MEM_ASSERT(pool->statistic.allocatedBlocks <= pool->statistic.numBlocks);
        #endif /*MEM_STATISTICS*/

        OSA_ExitCritical(kCriticalDisableInt);
        return p_block_header;
      }
      else
      {
        if (numBytes > pool->next_pool_block_size) break;
        // No more blocks of that size, try next size.
        numBytes = pool->next_pool_block_size;
      }
    }
    // Try next pool
    if (pool->next_pool_block_size) pool++;
    else break;
  }
  #ifdef MEM_MANAGER_STATISTICS
  pool->statistic.allocationFailures++;
  DEBUG_MEM_MAN_PRINT_ARG("Allocation Error\r\n", numBytes);
  #endif /*MEM_STATISTICS*/

  #ifdef MEM_DEBUG
  Panic(0, (uint32_t)MEM_BufferAlloc, 0, 0);
  #endif

  OSA_ExitCritical(kCriticalDisableInt);
  return NULL;
}

/*! *********************************************************************************
* \brief     Deallocate a memory block by putting it in the corresponding pool 
*            of free blocks. 
*
* \param[in] buffer - Pointer to buffer to deallocate.
*
* \return MEM_SUCCESS_c if deallocation was successful, MEM_FREE_ERROR_c if not.
*
* \pre Memory manager must be previously initialized.
*
* \remarks Never deallocate the same buffer twice.
*
********************************************************************************** */
memStatus_t MEM_BufferFree(void *buffer)
{

  if (buffer == NULL)
  {
    return MEM_FREE_ERROR_c;
  }

  OSA_EnterCritical(kCriticalDisableInt);

  T_pool_block_header *pHeader     = (T_pool_block_header *)buffer - 1;
  T_pool_descriptor   *pParentPool = (T_pool_descriptor *)pHeader->pParentPool;

  T_pool_descriptor   *pool        = g_pool_descriptors;
  for (;;)
  {
    if (pParentPool == pool) break;
    if (pool->next_pool_block_size == 0)
    {
      /* The parent pool was not found! This means that the memory buffer is corrupt or 
        that the MEM_BufferFree() function was called with an invalid parameter */
      #ifdef MEM_MANAGER_STATISTICS
      pParentPool->statistic.freeFailures++;
      DEBUG_MEM_MAN_PRINT("Free error.\r\n");
      #endif /*MEM_STATISTICS*/
      OSA_ExitCritical(kCriticalDisableInt);
      return MEM_FREE_ERROR_c;
    }
    pool++;
  }

  if (pHeader->link.list != NULL)
  {
    /* The memory buffer appears to be enqueued in a linked list. 
       This list may be the free memory buffers pool, or another list. */
    #ifdef MEM_MANAGER_STATISTICS
    pParentPool->statistic.freeFailures++;
    DEBUG_MEM_MAN_PRINT("Free error.\r\n");
    #endif /*MEM_STATISTICS*/
    OSA_ExitCritical(kCriticalDisableInt);
    return MEM_FREE_ERROR_c;
  }

  gFreeMessagesCount++;

  ListAddTail((listHandle_t)&pParentPool->anchor, (listElementHandle_t)&pHeader->link);

  #ifdef MEM_MANAGER_STATISTICS
  MEM_ASSERT(pParentPool->statistic.allocatedBlocks > 0);
  pParentPool->statistic.allocatedBlocks--;
  DEBUG_MEM_MAN_PRINT_ARG("-%d\r\n", pParentPool->block_size);
  #endif /*MEM_STATISTICS*/

  OSA_ExitCritical(kCriticalDisableInt);
  return MEM_SUCCESS_c;
}

/*! *********************************************************************************
* \brief     Determines the size of a memory block
*
* \param[in] buffer - Pointer to buffer.
*
* \return size of memory block
*
* \pre Memory manager must be previously initialized.
*
********************************************************************************** */
uint16_t MEM_BufferGetSize(void *buffer)
{
  if (buffer)
  {
    return ((T_pool_descriptor *)((T_pool_block_header *)buffer - 1)->pParentPool)->block_size;
  }

  return 0;
}


/*! *********************************************************************************
* \brief     This function checks for buffer overflow when copying multiple bytes
*
* \param[in] p    - pointer to destination.
* \param[in] size - number of bytes to copy
*
* \return 1 if overflow detected, else 0
*
********************************************************************************** */
uint8_t MEM_BufferCheck(uint8_t *p, uint32_t size)
{
  T_pools_list *pools_list = g_pools_list;
  uint8_t      *memory     = pools_memory;
  uint32_t     poolBytes,
               blockBytes,
               i;

  if ((p < (uint8_t *)pools_memory) || (p > ((uint8_t *)pools_memory + sizeof(pools_memory)))) return 0;

  while (pools_list->block_size)
  {
    blockBytes = pools_list->block_size + sizeof(T_pool_block_header);
    poolBytes  = blockBytes * pools_list->blocks_number;

    /* Find the correct message pool */
    if ((p >= memory) && (p < memory + poolBytes))
    {
      /* Check if the size to copy is greather then the size of the current block */
      if (size > pools_list->block_size)
      {
        #ifdef MEM_DEBUG
        Panic(0, 0, 0, 0);
        #endif
        return 1;
      }

      /* Find the correct memory block */
      for (i = 0; i < pools_list->blocks_number; i++)
      {
        if ((p >= memory) && (p < memory + blockBytes))
        {
          if (p + size > memory + blockBytes)
          {
            #ifdef MEM_DEBUG
            Panic(0, 0, 0, 0);
            #endif
            return 1;
          }
          else
          {
            return 0;
          }
        }

        memory += blockBytes;
      }
    }

    /* check next pool */
    memory += poolBytes;
    pools_list++;
  }

  return 0;
}

/*! *********************************************************************************
* \brief     Performs a write-read-verify test for every byte in all memory pools.
*
* \return Returns MEM_SUCCESS_c if test was successful, MEM_ALLOC_ERROR_c if a
*         buffer was not allocated successufuly, MEM_FREE_ERROR_c  if a
*         buffer was not freed successufuly or MEM_UNKNOWN_ERROR_c if a verify error,
*         heap overflow or data corruption occurred.
*
********************************************************************************** */
uint32_t MEM_WriteReadTest(void)
{
  uint8_t * data, count = 1;
  uintn32_t idx1,
            idx2,
            idx3;
  uint32_t  freeMsgs;

  /*memory write test*/
  freeMsgs = MEM_GetAvailableBlocks(0);

  for (idx1 = 0; g_pools_list[idx1].block_size != 0; idx1++)
  {
    for (idx2 = 0; idx2 < g_pools_list[idx1].blocks_number; idx2++)
    {
      data = (uint8_t *)MEM_BufferAlloc(g_pools_list[idx1].block_size);

      if (data == NULL)
      {
        return MEM_ALLOC_ERROR_c;
      }

      for (idx3 = 0; idx3 < g_pools_list[idx1].block_size; idx3++)
      {
        if (data > pools_memory + heapSize)
        {
          return MEM_UNKNOWN_ERROR_c;
        }
        *data = count & 0xff;
        data++;
      }
      count++;
    }
  }

  count = 1;
  data = pools_memory;
  /*memory read test*/
  for (idx1 = 0; g_pools_list[idx1].block_size != 0; idx1++)
  {
    for (idx2 = 0; idx2 < g_pools_list[idx1].blocks_number; idx2++)
    {
      /*New block; jump over list header*/
      data = data + sizeof(T_pool_block_header);
      for (idx3 = 0; idx3 < g_pools_list[idx1].block_size; idx3++)
      {
        if (*data == count)
        {
          data++;
        }
        else
        {
          return MEM_UNKNOWN_ERROR_c;
        }
      }
      if (MEM_BufferFree(data - g_pools_list[idx1].block_size) != MEM_SUCCESS_c)
      {
        return MEM_FREE_ERROR_c;
      }
      count++;
    }
  }
  if (MEM_GetAvailableBlocks(0) != freeMsgs)
  {
    return MEM_UNKNOWN_ERROR_c;
  }
  #ifdef MEM_MANAGER_STATISTICS
  for (idx1 = 0; g_pools_list[idx1].block_size != 0; idx1++)
  {
    g_pool_descriptors[idx1].statistic.allocatedBlocksPeak = 0;
  }
  #endif /*MEM_STATISTICS*/

  return MEM_SUCCESS_c;
}
