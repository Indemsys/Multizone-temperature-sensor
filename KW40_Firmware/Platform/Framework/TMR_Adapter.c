/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file TMR_Adapter.c
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

#include "TMR_Adapter.h"

#include "fsl_lptmr_driver.h"
#include "fsl_lptmr_hal.h"

#include "fsl_os_abstraction.h"
#include "fsl_clock_manager.h"
#include "fsl_tpm_driver.h"


static void LPTMR_ISR(void);

static lptmr_state_t gLptmrUserState;
extern const IRQn_Type g_lptmrIrqId[LPTMR_INSTANCE_COUNT];

/*! *********************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
********************************************************************************** */
void StackTimer_Init(void (*cb)(void))
{
  IRQn_Type irqId;
  TPM_Type  *tpmBaseAddr = g_tpmBase[gStackTimerInstance_c];  // Инициализируем таймер

  CLOCK_SYS_EnableTpmClock(gStackTimerInstance_c);

  TPM_HAL_Reset(tpmBaseAddr, gStackTimerInstance_c);
  TPM_HAL_SetClockDiv(tpmBaseAddr, kTpmDividedBy128);

  TPM_HAL_SetClockMode(tpmBaseAddr, kTpmClockSourceNoneClk);
  TPM_HAL_ClearCounter(tpmBaseAddr);
  TPM_HAL_SetMod(tpmBaseAddr, 0xFFFF); //allready done by TPM_HAL_Reset()
  /* Configure channel to Software compare; output pin not used */
  TPM_HAL_SetChnMsnbaElsnbaVal(tpmBaseAddr, gStackTimerChannel_c, TPM_CnSC_MSA_MASK);
  TPM_HAL_SetChnCountVal(tpmBaseAddr, gStackTimerChannel_c, 0x01);

  /* Install ISR */
  irqId = g_tpmIrqId[gStackTimerInstance_c];
  TPM_HAL_EnableTimerOverflowInt(tpmBaseAddr);
  TPM_HAL_EnableChnInt(tpmBaseAddr, gStackTimerChannel_c);
  /* Overwrite old ISR */
  OSA_InstallIntHandler(irqId, cb);
  /* set interrupt priority */
  NVIC_SetPriority(irqId, gStackTimer_IsrPrio_c >> (8 - __NVIC_PRIO_BITS));
  NVIC_ClearPendingIRQ(irqId);
  NVIC_EnableIRQ(irqId);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
void StackTimer_Enable(void)
{
  TPM_HAL_SetClockMode(g_tpmBase[gStackTimerInstance_c], kTpmClockSourceModuleClk);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
void StackTimer_Disable(void)
{
  TPM_HAL_SetClockMode(g_tpmBase[gStackTimerInstance_c], kTpmClockSourceNoneClk);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
 
 \return uint32_t 
-----------------------------------------------------------------------------------------------------*/
uint32_t StackTimer_GetInputFrequency(void)
{
  uint32_t prescaller;
  uint32_t refClk;
  refClk = CLOCK_SYS_GetTpmFreq(gStackTimerInstance_c);
  prescaller = TPM_HAL_GetClockDiv(g_tpmBase[gStackTimerInstance_c]);
  return refClk / (1 << prescaller);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
 
 \return uint32_t 
-----------------------------------------------------------------------------------------------------*/
uint32_t StackTimer_GetCounterValue(void)
{
  return TPM_HAL_GetCounterVal(g_tpmBase[gStackTimerInstance_c]);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param offset 
-----------------------------------------------------------------------------------------------------*/
void StackTimer_SetOffsetTicks(uint32_t offset)
{
  TPM_HAL_SetChnCountVal(g_tpmBase[gStackTimerInstance_c], gStackTimerChannel_c, offset);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
void StackTimer_ClearIntFlag(void)
{
  TPM_DRV_IRQHandler(gStackTimerInstance_c);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param cb 
-----------------------------------------------------------------------------------------------------*/
void LPTMR_Init(void (*cb)(void))
{
  const lptmr_user_config_t userConfig = {
    .timerMode = kLptmrTimerModeTimeCounter,
    .prescalerClockSource = kClockLptmrSrcEr32kClk,
    .prescalerValue = kLptmrPrescalerDivide32GlitchFilter16,
    .freeRunningEnable = 1,
    .isInterruptEnabled = 1,
    .pinSelect = kLptmrPinSelectInput0,
    .pinPolarity = kLptmrPinPolarityActiveHigh
  };

  /* Overwrite old ISR */
  OSA_InstallIntHandler(g_lptmrIrqId[gLptmrInstance_c], LPTMR_ISR);

  LPTMR_DRV_Init(gLptmrInstance_c, &gLptmrUserState, &userConfig);
  LPTMR_DRV_InstallCallback(gLptmrInstance_c, cb);
  LPTMR_DRV_Stop(gLptmrInstance_c);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
void LPTMR_Enable(void)
{
  LPTMR_DRV_Start(gLptmrInstance_c);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
void LPTMR_Disable(void)
{
  LPTMR_DRV_Stop(gLptmrInstance_c);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
 
 \return uint32_t 
-----------------------------------------------------------------------------------------------------*/
uint32_t LPTMR_GetInputFrequency(void)
{
  return gLptmrUserState.prescalerClockHz;
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
 
 \return uint32_t 
-----------------------------------------------------------------------------------------------------*/
uint32_t LPTMR_GetCounterValue(void)
{
  return LPTMR_HAL_GetCounterValue(g_lptmrBase[gLptmrInstance_c]);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param offset 
-----------------------------------------------------------------------------------------------------*/
void LPTMR_SetOffsetTicks(uint32_t offset)
{
  LPTMR_HAL_SetCompareValue(g_lptmrBase[gLptmrInstance_c], offset);
}

/*-----------------------------------------------------------------------------------------------------
 
 \param void 
-----------------------------------------------------------------------------------------------------*/
static void LPTMR_ISR(void)
{
  LPTMR_DRV_IRQHandler(gLptmrInstance_c);
}
