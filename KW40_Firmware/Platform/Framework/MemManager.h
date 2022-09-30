/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file MemManager.h
* This is the header file for the Memory Manager interface.
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

#ifndef _MEM_MANAGER_H_
#define _MEM_MANAGER_H_


/*! *********************************************************************************
*************************************************************************************
* Include
*************************************************************************************
********************************************************************************** */
#include "EmbeddedTypes.h"
#include "GenericList.h"

/*Defines statuses used in MEM_BufferAlloc and MEM_BufferFree*/
typedef enum
{
  MEM_SUCCESS_c = 0,                    /* No error occurred */
  MEM_INIT_ERROR_c,                     /* Memory initialization error */
  MEM_ALLOC_ERROR_c,                    /* Memory allocation error */
  MEM_FREE_ERROR_c,                     /* Memory free error */
  MEM_UNKNOWN_ERROR_c                   /* something bad has happened... */
}memStatus_t;


/*Initialises the Memory Manager.*/
memStatus_t MEM_Init(void);
/*Returns the number of available blocks that fit the given size.*/
uint32_t MEM_GetAvailableBlocks(uint32_t size);
/*Frees the givem buffer.*/
memStatus_t MEM_BufferFree(void *buffer);
/*Returns the allocated buffer of the given size.*/
void* MEM_BufferAlloc(uint32_t numBytes);
/*Returns the size of a given buffer*/
uint16_t MEM_BufferGetSize(void *buffer);
/*Performs a write-read-verify test accross all pools*/
uint32_t MEM_WriteReadTest(void);

uint32_t Get_boggest_pool_free_block_cnt(void);

#define MEM_ASSERT(condition) (void)(condition);



#ifdef MEM_MANAGER_STATISTICS
/*Statistics structure definition. Used by pools.*/
typedef struct poolStat_tag
{
  uint16_t numBlocks;
  uint16_t allocatedBlocks;
  uint16_t allocatedBlocksPeak;
  uint16_t allocationFailures;
  uint16_t freeFailures;
} poolStat_t;
#endif /*MEM_STATISTICS*/


/*Header description for buffers.*/
typedef struct listHeader_tag
{
  listElement_t     link;
  struct pools_tag *pParentPool;
}
T_pool_block_header;

typedef __packed struct pools_tag
{
    list_t      anchor;              /* MUST be first element in pools_t struct */
    uint16_t    next_pool_block_size;
    uint16_t    block_size;
  #ifdef MEM_MANAGER_STATISTICS
    poolStat_t  statistic;
    uint8_t     padding[2];
  #endif /*MEM_STATISTICS*/
}
T_pool_descriptor;

typedef __packed struct poolInfo_tag
{
    uint16_t block_size;
    uint16_t blocks_number;
}
T_pools_list;

/*! *********************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
********************************************************************************** */

uint8_t MEM_BufferCheck(uint8_t *p, uint32_t size);

#endif /* _MEM_MANAGER_H_ */
