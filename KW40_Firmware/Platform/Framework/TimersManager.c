/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file TimersManager.c
* TIMER implementation file for the ARM CORTEX-M4 processor
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

#include "EmbeddedTypes.h"
#include "TimersManagerInternal.h"
#include "TimersManager.h"
#include "Panic.h"
#include "TMR_Adapter.h"

#include "fsl_os_abstraction.h"
#include "fsl_rtc_driver.h"
#include "fsl_pit_driver.h"
#include "fsl_clock_manager.h"



/*****************************************************************************
******************************************************************************
* Private macros
******************************************************************************
*****************************************************************************/
#define mTmrDummyEvent_c (1<<16)


/*****************************************************************************
 *****************************************************************************
 * Private prototypes
 *****************************************************************************
 *****************************************************************************/

#if gTMR_Enabled_d

/*---------------------------------------------------------------------------
 * NAME: TMR_GetTimerStatus
 * DESCRIPTION: RETURNs the timer status
 * PARAMETERS:  IN: timerID - the timer ID
 * RETURN: see definition of tmrStatus_t
 * NOTES: none
 *---------------------------------------------------------------------------*/
static tmrStatus_t TMR_GetTimerStatus(tmrTimerID_t timerID);

/*---------------------------------------------------------------------------
 * NAME: TMR_SetTimerStatus
 * DESCRIPTION: Set the timer status
 * PARAMETERS:  IN: timerID - the timer ID
 *             IN: status - the status of the timer
 * RETURN: None
 * NOTES: none
 *---------------------------------------------------------------------------*/
static void TMR_SetTimerStatus(tmrTimerID_t timerID, tmrStatus_t status);

/*---------------------------------------------------------------------------
 * NAME: TMR_GetTimerType
 * DESCRIPTION: RETURNs the timer type
 * PARAMETERS:  IN: timerID - the timer ID
 * RETURN: see definition of tmrTimerType_t
 * NOTES: none
 *---------------------------------------------------------------------------*/
static tmrTimerType_t TMR_GetTimerType(tmrTimerID_t timerID);

/*---------------------------------------------------------------------------
 * NAME: TMR_SetTimerType
 * DESCRIPTION: Set the timer type
 * PARAMETERS:  IN: timerID - the timer ID
 *              IN: type - timer type
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
static void TMR_SetTimerType(tmrTimerID_t timerID, tmrTimerType_t type);

/*---------------------------------------------------------------------------
 * NAME: TmrTicksFromMilliseconds
 * DESCRIPTION: Convert milliseconds to ticks
 * PARAMETERS:  IN: milliseconds
 * RETURN: tmrTimerTicks64_t - ticks number
 * NOTES: none
 *---------------------------------------------------------------------------*/
tmrTimerTicks64_t TmrTicksFromMilliseconds(tmrTimeInMilliseconds_t milliseconds);


/*---------------------------------------------------------------------------
 * NAME: StackTimer_ISR
 * DESCRIPTION: Event function called by driver ISR on channel match in interrupt context.
 * PARAMETERS:  IN: User data; unused.
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void StackTimer_ISR(void);

/*---------------------------------------------------------------------------
 * NAME: TMR_Task
 * DESCRIPTION: Timer thread.
 *              Called by the kernel when the timer ISR posts a timer event.
 * PARAMETERS:  IN: param - User parameter to timer thread; not used.
 * RETURN: None
 * NOTES: none
 *---------------------------------------------------------------------------*/

void TMR_Task(task_param_t param);
#endif /*gTMR_Enabled_d*/

#if gPrecision_Timers_Enabled_d

/*---------------------------------------------------------------------------
 * NAME: TMR_PrecisionTimerOverflowNotify
 * DESCRIPTION: Event function called by driver ISR on timer overflow in interrupt context.
 * PARAMETERS:  IN: User data; unused.
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void TMR_PrecisionTimerOverflowNotify
(
  LDD_TUserData *UserDataPtr
  );

#endif /*gPrecision_Timers_Enabled_d*/

#if gTimestamp_Enabled_d

/*---------------------------------------------------------------------------
 * NAME: TMR_RTCAlarmNotify
 * DESCRIPTION: Event function called by driver ISR on RTC alarm in interrupt context.
 * PARAMETERS:  IN: User data; unused.
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void TMR_RTCAlarmNotify(void);

#endif /*gTimestamp_Enabled_d*/

#if gTMR_EnableHWLowPowerTimers_d
void LPTMR_Notify(void);
#endif

/*****************************************************************************
 *****************************************************************************
 * Private memory definitions
 *****************************************************************************
 *****************************************************************************/

#if gTMR_Enabled_d

/*
 * NAME: previousTimeInTicks
 * DESCRIPTION: The previous time in ticks when the counter register was read
 * VALUES: 0..65535
 */
static tmrTimerTicks16_t    prev_timer_counter;

/*
 * NAME: mMaxToCountDown_c
 * DESCRIPTION:  Count to maximum (0xffff - 2*4ms(in ticks)), to be sure that
 * the currentTimeInTicks will never roll over previousTimeInTicks in the
 * TMR_Task(); A thread have to be executed at most in 4ms.
 * VALUES: 0..65535
 */
static uint16_t             max_timer_period_in_ticks;

/*
 * NAME: mTicksFor4ms
 * DESCRIPTION:  Ticks for 4ms. The TMR_Task()event will not be issued faster than 4ms
 * VALUES: uint16_t range
 */
static uint16_t             ticks_for_4ms;

/*
 * NAME: mCounterFreqHz
 * DESCRIPTION:  The counter frequency in hz.
 * VALUES: see definition
 */
static uint32_t             mCounterFreqHz;

/*
 * NAME: maTmrTimerTable
 * DESCRIPTION:  Main timer table. All allocated timers are stored here.
 *               A timer's ID is it's index in this table.
 * VALUES: see definition
 */
static tmrTimerTableEntry_t tmr_timer_table[TMR_TOTAL_TIMERS_NUMBER];

/*
 * NAME: maTmrTimerStatusTable
 * DESCRIPTION: timer status stable. Making the single-byte-per-timer status
 *              table a separate array saves a bit of code space.
 *              If an entry is == 0, the timer is not allocated.
 * VALUES: see definition
 */
static tmrStatus_t          tmr_timer_status_table[TMR_TOTAL_TIMERS_NUMBER];

/*
 * NAME: numberOfActiveTimers
 * DESCRIPTION: Number of Active timers (without low power capability)
 *              the MCU can not enter low power if numberOfActiveTimers!=0
 * VALUES: 0..255
 */
static uint8_t              numberOfActiveTimers         = 0;

/*
 * NAME: numberOfLowPowerActiveTimers
 * DESCRIPTION: Number of low power active timer.
 *              The MCU can enter in low power if more low power timers are active
 * VALUES:
 */
static uint8_t              numberOfLowPowerActiveTimers = 0;


/*
 * NAME: timerHardwareIsRunning
 * DESCRIPTION: Flag if the hardware timer is running or not
 * VALUES: TRUE/FALSE
 */
static bool_t               timer_hardware_is_running       = FALSE;



/*
 * NAME: 
 * DESCRIPTION: Defines the timer thread's stack
 * VALUES:
 */
OSA_TASK_DEFINE(TMR, TMR_TASK_STACK_SZ);

/*
 * NAME: 
 * DESCRIPTION: The OS threadId for TMR task.
 * VALUES:
 */
static event_t                    tmr_task_event_object;

                     #if gTMR_EnableHWLowPowerTimers_d

/*
 * NAME: previousLpTimeInTicks
 * DESCRIPTION: The previous time in ticks when the counter register was read
 * VALUES: 0..65535
 */
tmrTimerTicks16_t                 previousLpTimeInTicks;

/*
 * NAME: mLpMaxToCountDown_c
 * DESCRIPTION:  Count to maximum (0xffff - 2*4ms(in ticks)), to be sure that
 * the currentTimeInTicks will never roll over previousTimeInTicks in the
 * TMR_Task(); A thread have to be executed at most in 4ms.
 * VALUES: 0..65535
 */
static uint16_t                   mLpMaxToCountDown_c;

/*
 * NAME: mTicksFor4ms
 * DESCRIPTION:  Ticks for 4ms. The TMR_Task()event will not be issued faster than 4ms
 * VALUES: uint16_t range
 */
static uint16_t                   mLpTicksFor4ms;

/*
 * NAME: mCounterFreqHz
 * DESCRIPTION:  The counter frequency in hz.
 * VALUES: see definition
 */
static uint32_t                   mLpCounterFreqHz;

/*
 * NAME: timerHardwareIsRunning
 * DESCRIPTION: Flag if the hardware timer is running or not
 * VALUES: TRUE/FALSE
 */
static bool_t                     lpTimerHardwareIsRunning   = FALSE;

                        #endif /* gTMR_EnableHWLowPowerTimers_d */

                      #endif /*gTMR_Enabled_d*/

                      #if gPrecision_Timers_Enabled_d

/*
 * NAME: globalTimeTicks
 * DESCRIPTION: 64bit timer extension.
 * VALUES:
 */
static volatile tmrTimerTicks64_t globalTimeTicks;

/*
 * NAME: gPrecisionTimerHandle
 * DESCRIPTION: Hardware timer handle for precision timer
 * VALUES:
 */
static LDD_TDeviceData            *gPrecisionTimerHandle;

/*
 * NAME: gPrecisionTimerTickus
 * DESCRIPTION: Period of the hardware timer tick for precision timer
 * VALUES:
 */
static uint32_t                   gPrecisionTimerTickus;

/*
 * NAME: gPrecisionTimerTickusReal
 * DESCRIPTION: Period of the hardware timer tick for precision timer
 * VALUES:
 */
static float                      gPrecisionTimerTickusReal;

/*
 * NAME: gPrecisionTimerPeriodTicks
 * DESCRIPTION: Period in ticks of the precision timer
 * VALUES:
 */
static uint32_t                   gPrecisionTimerPeriodTicks;

/*
 * NAME: PrecisionTimer_OnCounterRestart_fptr
 * DESCRIPTION: Function pointer for pit timer callback. Gets called from PEx Events.c
 * VALUES:
 */
extern void (*PrecisionTimer_OnCounterRestart_fptr)(LDD_TUserData *);

#endif

#if gTimestamp_Enabled_d

/*
 * NAME: gRTCTimeOffset
 * DESCRIPTION: Holds time offset in microseconds, used to calculate the date
 * VALUES:
 */
static volatile uint64_t gRTCTimeOffset;

/*
 * NAME: gRTCPrescalerOffset
 * DESCRIPTION: Holds time prescaler offset in ticks, used to calculate the date
 * VALUES:
 */
static volatile uint16_t gRTCPrescalerOffset;

/*
 * NAME: gRTCAlarmCallback
 * DESCRIPTION: Callback for the alarm.
 * VALUES:
 */
static pfTmrCallBack_t   gRTCAlarmCallback;

/*
 * NAME: gRTCAlarmCallbackParam
 * DESCRIPTION: Parameter for the alarm callback.
 * VALUES:
 */
static void              *gRTCAlarmCallbackParam;

             #endif /*gTimestamp_Enabled_d*/


             #if gTMR_PIT_Timestamp_Enabled_d
               #if FSL_FEATURE_PIT_TIMER_COUNT < 3
static uint32_t          mPIT_TimestampHigh;
  #endif
#endif

/*****************************************************************************
******************************************************************************
* Private functions
******************************************************************************
*****************************************************************************/

#if gTMR_Enabled_d

/*---------------------------------------------------------------------------
* NAME: TMR_GetTimerStatus
* DESCRIPTION: Returns the timer status
* PARAMETERS:  IN: timerID - the timer ID
* RETURN: see definition of tmrStatus_t
* NOTES: none
*---------------------------------------------------------------------------*/
static tmrStatus_t TMR_GetTimerStatus(tmrTimerID_t timerID)
{
  return tmr_timer_status_table[timerID] & TIMER_STATUS_MASK;
}

/*---------------------------------------------------------------------------
* NAME: TMR_SetTimerStatus
* DESCRIPTION: Set the timer status
* PARAMETERS:  IN: timerID - the timer ID
*              IN: status - the status of the timer
* RETURN: None
* NOTES: none
*---------------------------------------------------------------------------*/
static void TMR_SetTimerStatus(tmrTimerID_t timerID, tmrStatus_t status)
{
  tmr_timer_status_table[timerID] = (tmr_timer_status_table[timerID] & ~TIMER_STATUS_MASK) | status;
}

/*---------------------------------------------------------------------------
* NAME: TMR_GetTimerType
* DESCRIPTION: Returns the timer type
* PARAMETERS:  IN: timerID - the timer ID
* RETURN: see definition of tmrTimerType_t
* NOTES: none
*---------------------------------------------------------------------------*/
static tmrTimerType_t TMR_GetTimerType(tmrTimerID_t timerID)
{
  return tmr_timer_status_table[timerID] & TYMER_TYPE_MASK;
}

/*---------------------------------------------------------------------------
* NAME: TMR_SetTimerType
* DESCRIPTION: Set the timer type
* PARAMETERS:  IN: timerID - the timer ID
*              IN: type - timer type
* RETURN: none
* NOTES: none
*---------------------------------------------------------------------------*/
static void TMR_SetTimerType(tmrTimerID_t timerID, tmrTimerType_t type)
{
  tmr_timer_status_table[timerID] = (tmr_timer_status_table[timerID] & ~TYMER_TYPE_MASK) | type;
}

#endif /*gTMR_Enabled_d*/

#if gTimestamp_Enabled_d

/*---------------------------------------------------------------------------
 * NAME: TMR_RTCAlarmNotify
 * DESCRIPTION: Event function called by driver ISR on RTC alarm in interrupt context.
 * PARAMETERS:  IN: User data; unused.
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void TMR_RTCAlarmNotify(void)
{
  RTC_Type *rtcBaseAddr = g_rtcBase[gTmrRtcInstance_c];

  RTC_HAL_SetAlarmIntCmd(rtcBaseAddr, FALSE);
  RTC_HAL_SetAlarmReg(rtcBaseAddr, RTC_HAL_GetAlarmReg(rtcBaseAddr));

  if (gRTCAlarmCallback != NULL)
  {
    gRTCAlarmCallback(gRTCAlarmCallbackParam);
  }
}

#endif /*gTimestamp_Enabled_d*/


#if (gTMR_PIT_Timestamp_Enabled_d)
  #if FSL_FEATURE_PIT_TIMER_COUNT < 3

static void TMR_PIT_ISR(void)
{
  PIT_HAL_ClearIntFlag(g_pitBase[gTmrPitInstance_c], 1);
  mPIT_TimestampHigh--;
}

  #endif
#endif

/*****************************************************************************
******************************************************************************
* Public functions
******************************************************************************
*****************************************************************************/

#if gTMR_Enabled_d
/*---------------------------------------------------------------------------
 * NAME: TmrTicksFromMilliseconds
 * DESCRIPTION: Convert milliseconds to ticks
 * PARAMETERS:  IN: milliseconds
 * RETURN: tmrTimerTicks64_t - ticks number
 * NOTES: none
 *---------------------------------------------------------------------------*/
tmrTimerTicks64_t TmrTicksFromMilliseconds(tmrTimeInMilliseconds_t milliseconds)
{
  return (tmrTimerTicks64_t)milliseconds * mCounterFreqHz / 1000;
}

  #if gTMR_EnableHWLowPowerTimers_d
/*---------------------------------------------------------------------------
 * NAME: LptmrTicksFromMilliseconds
 * DESCRIPTION: Convert milliseconds to ticks
 * PARAMETERS:  IN: milliseconds
 * RETURN: tmrTimerTicks64_t - ticks number
 * NOTES: none
 *---------------------------------------------------------------------------*/
tmrTimerTicks64_t LptmrTicksFromMilliseconds(tmrTimeInMilliseconds_t milliseconds)
{
  return (tmrTimerTicks64_t)milliseconds * mLpCounterFreqHz / 1000;
}

/*---------------------------------------------------------------------------
 * NAME: LPTMR_Notify
 * DESCRIPTION: Event function called by driver ISR on compare match in interrupt context.
 * PARAMETERS: none
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void LPTMR_Notify(void)
{
  (void)OSA_EventSet(&tmr_task_event_object, mTmrDummyEvent_c);
}
  #endif

/*---------------------------------------------------------------------------
 * NAME: TMR_Init
 * DESCRIPTION: initialize the timer module
 * PARAMETERS: -
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_Init(void)
{
  static uint8_t initialized = FALSE;

  /* Check if TMR is already initialized */
  if (initialized) return;

  initialized = TRUE;

  StackTimer_Init(StackTimer_ISR);
  mCounterFreqHz = (uint32_t)((StackTimer_GetInputFrequency()));

  /* Count to maximum (0xffff - 2*4ms(in ticks)), to be sure that the currentTimeInTicks will never roll over previousTimeInTicks in the TMR_Task() */
  max_timer_period_in_ticks = 0xFFFF - TmrTicksFromMilliseconds(8);
  /* The TMR_Task()event will not be issued faster than 4ms*/
  ticks_for_4ms = TmrTicksFromMilliseconds(4);

  #if gTMR_EnableHWLowPowerTimers_d
  LPTMR_Init(LPTMR_Notify);
  mLpCounterFreqHz = LPTMR_GetInputFrequency();
  mLpTicksFor4ms = LptmrTicksFromMilliseconds(4);
  mLpMaxToCountDown_c = 0xFFFF - (2 * mLpTicksFor4ms);
  #endif

  osa_status_t   status;
  task_handler_t timerThreadId;

  status = OSA_EventCreate(&tmr_task_event_object, kEventAutoClear);
  if (kStatus_OSA_Success != status)
  {
    Panic(0, (uint32_t)TMR_Init, 0, 0);
    return;
  }

  status = OSA_TaskCreate(TMR_Task, "TMR_Task", TMR_TASK_STACK_SZ, TMR_stack, TMR_TASK_PRIO, (task_param_t)NULL, FALSE, &timerThreadId);

  if (kStatus_OSA_Success != status)
  {
    Panic(0, (uint32_t)TMR_Init, 0, 0);
    return;
  }
}

/*---------------------------------------------------------------------------
 * NAME: TMR_NotifyClkChanged
 * DESCRIPTION: This function is called when the clock is changed
 * PARAMETERS: IN: clkKhz (uint32_t) - new clock
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_NotifyClkChanged(uint32_t clkKhz)
{
  (void)clkKhz;
  mCounterFreqHz = (uint32_t)((StackTimer_GetInputFrequency()));
  /* Clock was changed, so calculate again  mMaxToCountDown_c.
  Count to maximum (0xffff - 2*4ms(in ticks)), to be sure that the currentTimeInTicks
  will never roll over previousTimeInTicks in the TMR_Task() */
  max_timer_period_in_ticks = 0xFFFF - TmrTicksFromMilliseconds(8);
  /* The TMR_Task()event will not be issued faster than 4ms*/
  ticks_for_4ms = TmrTicksFromMilliseconds(4);
  #if gTMR_EnableHWLowPowerTimers_d
  mLpCounterFreqHz = LPTMR_GetInputFrequency();
  mLpTicksFor4ms = LptmrTicksFromMilliseconds(4);
  mLpMaxToCountDown_c = 0xFFFF - (2 * mLpTicksFor4ms);
  #endif
}

/*---------------------------------------------------------------------------
  
   Помимо прочего функция используется в модуле sm_states.o библиотеки ble_host_lib.a
 
 * NAME: TMR_AllocateTimer
 * DESCRIPTION: allocate a timer
 * PARAMETERS: -
 * RETURN: timer ID
 *---------------------------------------------------------------------------*/
tmrTimerID_t TMR_AllocateTimer(void)
{
  uint32_t i;

  for (i = 0; i < NUMBER_OF_ELEMENTS(tmr_timer_table); ++i)
  {
    if (!TMR_IsTimerAllocated(i))
    {
      TMR_SetTimerStatus(i, TMR_STATUS_INACTIVE);
      return i;
    }
  }

  return TMR_INVALID_TIMER_ID;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_AreAllTimersOff
 * DESCRIPTION: Check if all timers except the LP timers are OFF.
 * PARAMETERS: -
 * RETURN: TRUE if there are no active non-low power timers, FALSE otherwise
 *---------------------------------------------------------------------------*/
bool_t TMR_AreAllTimersOff(void)
{
  return !numberOfActiveTimers;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_FreeTimer
 * DESCRIPTION: Free a timer
 * PARAMETERS:  IN: timerID - the ID of the timer
 * RETURN: -
 * NOTES: Safe to call even if the timer is running.
 *        Harmless if the timer is already free.
 *---------------------------------------------------------------------------*/
tmrErrCode_t TMR_FreeTimer(tmrTimerID_t timerID)
{
  tmrErrCode_t status;

  status = TMR_StopTimer(timerID);

  if (status == gTmrSuccess_c)
  {
    TMR_MarkTimerFree(timerID);
  }

  return gTmrSuccess_c;
}

/*---------------------------------------------------------------------------
 * NAME: StackTimer_OnChannel0_Notify
 * DESCRIPTION: Event function called by driver ISR on channel match in interrupt context.
 * PARAMETERS:  IN: User data; unused.
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void StackTimer_ISR(void)
{
  StackTimer_ClearIntFlag();
  (void)OSA_EventSet(&tmr_task_event_object, mTmrDummyEvent_c);
}

/*---------------------------------------------------------------------------
 * NAME: TMR_IsTimerActive
 * DESCRIPTION: Check if a specified timer is active
 * PARAMETERS: IN: timerID - the ID of the timer
 * RETURN: TRUE if the timer (specified by the timerID) is active,
 *         FALSE otherwise
 *---------------------------------------------------------------------------*/
bool_t TMR_IsTimerActive(tmrTimerID_t timerID)
{
  return TMR_GetTimerStatus(timerID) == TMR_STATUS_ACTIVE;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_IsTimerReady
 * DESCRIPTION: Check if a specified timer is ready
 * PARAMETERS: IN: timerID - the ID of the timer
 * RETURN: TRUE if the timer (specified by the timerID) is ready,
 *         FALSE otherwise
 *---------------------------------------------------------------------------*/
bool_t TMR_IsTimerReady(tmrTimerID_t timerID)
{
  return TMR_GetTimerStatus(timerID) == TMR_STATUS_READY;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_GetRemainingTime
 * DESCRIPTION: Returns the remaining time until timeout, for the specified
 *              timer
 * PARAMETERS: IN: timerID - the ID of the timer
 * RETURN: remaining time in milliseconds until next timer timeout
 *---------------------------------------------------------------------------*/
uint32_t TMR_GetRemainingTime(tmrTimerID_t tmrID)
{
  tmrTimerTicks16_t currentTime,
                    elapsedRemainingTicks;
  uint32_t          remainingTime,
                    freq                  = mCounterFreqHz;

  if ((tmrID >= TMR_TOTAL_TIMERS_NUMBER) ||
      !TMR_IsTimerAllocated(tmrID) ||
      (tmr_timer_table[tmrID].remaining_ticks == 0)) return 0;

  TmrIntDisableAll();

  #if gTMR_EnableHWLowPowerTimers_d
  if (IsLowPowerTimer(TMR_GetTimerType(tmrID)))
  {
    currentTime = LPTMR_GetCounterValue();
    freq = mLpCounterFreqHz;
  }
  else
  #endif
  {
    currentTime = StackTimer_GetCounterValue();
  }

  if (currentTime < tmr_timer_table[tmrID].timestamp)
  {
    currentTime += 0xFFFF;
  }

  elapsedRemainingTicks = currentTime - tmr_timer_table[tmrID].timestamp;

  if (elapsedRemainingTicks > tmr_timer_table[tmrID].remaining_ticks)
  {
    TmrIntRestoreAll();
    return 1;
  }

  remainingTime = ((uint64_t)(tmr_timer_table[tmrID].remaining_ticks - elapsedRemainingTicks) * 1000 + freq - 1) / freq;

  TmrIntRestoreAll();
  return remainingTime;
}


/*---------------------------------------------------------------------------
 * NAME: TMR_StartTimer (BeeStack or application)
 * DESCRIPTION: Start a specified timer
 * PARAMETERS: IN: timerId - the ID of the timer
 *             IN: timerType - the type of the timer
 *             IN: timeInMilliseconds - time expressed in millisecond units
 *             IN: pfTmrCallBack - callback function
 *             IN: param - parameter to callback function
 * RETURN: -
 * NOTES: When the timer expires, the callback function is called in
 *        non-interrupt context. If the timer is already running when
 *        this function is called, it will be stopped and restarted.
 *---------------------------------------------------------------------------*/
tmrErrCode_t TMR_StartTimer(tmrTimerID_t timerID, tmrTimerType_t timerType, tmrTimeInMilliseconds_t timeInMilliseconds, void (*pfTimerCallBack)(void *), void *param)
{
  tmrErrCode_t      status;
  tmrTimerTicks64_t intervalInTicks;

  /* Stopping an already stopped timer is harmless. */
  status = TMR_StopTimer(timerID);

  if (status == gTmrSuccess_c)
  {
    #if gTMR_EnableHWLowPowerTimers_d
    if (IsLowPowerTimer(timerType))
    {
      intervalInTicks = LptmrTicksFromMilliseconds(timeInMilliseconds);
      tmr_timer_table[timerID].timestamp = LPTMR_GetCounterValue();
    }
    else
    #endif
    {
      intervalInTicks = TmrTicksFromMilliseconds(timeInMilliseconds);
      tmr_timer_table[timerID].timestamp = StackTimer_GetCounterValue();
    }

    if (!intervalInTicks)
    {
      intervalInTicks = 1;
    }

    TMR_SetTimerType(timerID, timerType);
    tmr_timer_table[timerID].interval_in_ticks = intervalInTicks;
    tmr_timer_table[timerID].remaining_ticks = intervalInTicks;
    tmr_timer_table[timerID].call_back_func = pfTimerCallBack;
    tmr_timer_table[timerID].param = param;

    /* Enable timer, the timer thread will do the rest of the work. */
    TMR_EnableTimer(timerID);
  }

  return status;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_StartLowPowerTimer
 * DESCRIPTION: Start a low power timer. When the timer goes off, call the
 *              callback function in non-interrupt context.
 *              If the timer is running when this function is called, it will
 *              be stopped and restarted.
 *              Start the timer with the following timer types:
 *                          - gTmrLowPowerMinuteTimer_c
 *                          - gTmrLowPowerSecondTimer_c
 *                          - gTmrLowPowerSingleShotMillisTimer_c
 *                          - gTmrLowPowerIntervalMillisTimer_c
 *              The MCU can enter in low power if there are only active low
 *              power timers.
 * PARAMETERS: IN: timerId - the ID of the timer
 *             IN: timerType - the type of the timer
 *             IN: timeIn - time in ticks
 *             IN: pfTmrCallBack - callback function
 *             IN: param - parameter to callback function
 * RETURN: type/DESCRIPTION
 *---------------------------------------------------------------------------*/
tmrErrCode_t TMR_StartLowPowerTimer(tmrTimerID_t timerId, tmrTimerType_t timerType, uint32_t timeIn, void (*pfTmrCallBack)(void *), void *param)
{
  #if(gTMR_EnableLowPowerTimers_d)
  return TMR_StartTimer(timerId, timerType | TMR_LOW_POWER_TIMER, timeIn, pfTmrCallBack, param);
  #else
  timerId = timerId;
  timerType = timerType;
  timeIn = timeIn;
  pfTmrCallBack = pfTmrCallBack;
  param = param;
  return gTmrSuccess_c;
  #endif
}

/*---------------------------------------------------------------------------
 * NAME: TMR_StartMinuteTimer
 * DESCRIPTION: Starts a minutes timer
 * PARAMETERS:  IN: timerId - the ID of the timer
 *              IN: timeInMinutes - time expressed in minutes
 *              IN: pfTmrCallBack - callback function
 *              IN: param - parameter to callback function
 * RETURN: None
 * NOTES: Customized form of TMR_StartTimer(). This is a single shot timer.
 *        There are no interval minute timers.
 *---------------------------------------------------------------------------*/
  #if gTMR_EnableMinutesSecondsTimers_d
tmrErrCode_t TMR_StartMinuteTimer(tmrTimerID_t timerId, tmrTimeInMinutes_t timeInMinutes, void (*pfTmrCallBack)(void *), void *param)
{
  return TMR_StartTimer(timerId, TMR_MINUTE_TIMER, TmrMinutes(timeInMinutes), pfTmrCallBack, param);
}
  #endif

/*---------------------------------------------------------------------------
 * NAME: TMR_StartSecondTimer
 * DESCRIPTION: Starts a seconds timer
 * PARAMETERS:  IN: timerId - the ID of the timer
 *              IN: timeInSeconds - time expressed in seconds
 *              IN: pfTmrCallBack - callback function
 * RETURN: None
 * NOTES: Customized form of TMR_StartTimer(). This is a single shot timer.
 *        There are no interval seconds timers.
 *---------------------------------------------------------------------------*/
  #if gTMR_EnableMinutesSecondsTimers_d
tmrErrCode_t TMR_StartSecondTimer(tmrTimerID_t timerId, tmrTimeInSeconds_t timeInSeconds, void (*pfTmrCallBack)(void *), void *argument)
{
  return TMR_StartTimer(timerId, TMR_SECOND_TIMER, TmrSeconds(timeInSeconds), pfTmrCallBack, argument);
}
  #endif

/*---------------------------------------------------------------------------
 * NAME: TMR_StartIntervalTimer
 * DESCRIPTION: Starts an interval count timer
 * PARAMETERS:  IN: timerId - the ID of the timer
 *              IN: timeInMilliseconds - time expressed in milliseconds
 *              IN: pfTmrCallBack - callback function
 *              IN: param - parameter to callback function
 * RETURN: None
 * NOTES: Customized form of TMR_StartTimer()
 *---------------------------------------------------------------------------*/
tmrErrCode_t TMR_StartIntervalTimer(tmrTimerID_t timerID, tmrTimeInMilliseconds_t timeInMilliseconds, void (*pfTimerCallBack)(void *), void *param)
{
  return TMR_StartTimer(timerID, TMR_INTERVAL_TIMER, timeInMilliseconds, pfTimerCallBack, param);
}

/*---------------------------------------------------------------------------
 * NAME: TMR_StartSingleShotTimer
 * DESCRIPTION: Starts an single-shot timer
 * PARAMETERS:  IN: timerId - the ID of the timer
 *              IN: timeInMilliseconds - time expressed in milliseconds
 *              IN: pfTmrCallBack - callback function
 *              IN: param - parameter to callback function
 * RETURN: None
 * NOTES: Customized form of TMR_StartTimer()
 *---------------------------------------------------------------------------*/
tmrErrCode_t TMR_StartSingleShotTimer(tmrTimerID_t timerID, tmrTimeInMilliseconds_t timeInMilliseconds, void (*pfTimerCallBack)(void *), void *param)
{
  return TMR_StartTimer(timerID, TMR_SINGLE_SHOT_TIMER, timeInMilliseconds, pfTimerCallBack, param);
}

/*---------------------------------------------------------------------------
 * NAME: TMR_StopTimer
 * DESCRIPTION: Stop a timer
 * PARAMETERS:  IN: timerID - the ID of the timer
 * RETURN: None
 * NOTES: Associated timer callback function is not called, even if the timer
 *        expires. Does not frees the timer. Safe to call anytime, regardless
 *        of the state of the timer.
 *---------------------------------------------------------------------------*/
tmrErrCode_t TMR_StopTimer(tmrTimerID_t timerID)
{
  tmrStatus_t status;

  /* check if timer is not allocated or if it has an invalid ID (fix@ENGR00323423) */
  if ((timerID >= TMR_TOTAL_TIMERS_NUMBER) || !TMR_IsTimerAllocated(timerID)) return gTmrInvalidId_c;

  TmrIntDisableAll();
  status = TMR_GetTimerStatus(timerID);

  if ((status == TMR_STATUS_ACTIVE) || (status == TMR_STATUS_READY))
  {
    TMR_SetTimerStatus(timerID, TMR_STATUS_INACTIVE);
    DecrementActiveTimerNumber(TMR_GetTimerType(timerID));
    /* if no sw active timers are enabled, */
    /* call the TMR_Task() to countdown the ticks and stop the hw timer*/
    if (!numberOfActiveTimers && !numberOfLowPowerActiveTimers)
    {
      (void)OSA_EventSet(&tmr_task_event_object, mTmrDummyEvent_c);
    }

  }

  TmrIntRestoreAll();
  return gTmrSuccess_c;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_Task
 * DESCRIPTION: Timer thread.
 *              Called by the kernel when the timer ISR posts a timer event.
 * PARAMETERS:  IN: events - timer events mask
 * RETURN: None
 * NOTES: none
 *---------------------------------------------------------------------------*/
void TMR_Task(task_param_t param)
{
  tmrTimerTicks16_t next_interrupt_time;
  tmrTimerTicks16_t current_timer_counter;
  tmrTimerTicks16_t elapsed_time,
                    elapsed_ticks;
  pfTmrCallBack_t   call_back_func;
  tmrTimerType_t    timer_type;
  tmrTimerStatus_t  status;
  uint8_t           timer_nr;
  #if gTMR_EnableHWLowPowerTimers_d
  tmrTimerTicks16_t currentLpTimeInTicks;
  tmrTimerTicks16_t lpTicksSinceLastHere;
  tmrTimerTicks16_t next_Lp_interrupt_time;
  #endif

  (void)param;

  event_flags_t     ev;

  while (1)
  {
    // Ожидаем событие прерывания от таймера
    (void)OSA_EventWait(&tmr_task_event_object, 0x00FFFFFF, FALSE, OSA_WAIT_FOREVER, &ev);

    TmrIntDisableAll();

    current_timer_counter = StackTimer_GetCounterValue();

    #if gTMR_EnableHWLowPowerTimers_d
    currentLpTimeInTicks = LPTMR_GetCounterValue();
    #endif

    TmrIntRestoreAll();

    elapsed_time = (current_timer_counter - prev_timer_counter);
    prev_timer_counter = current_timer_counter;

    
    next_interrupt_time = max_timer_period_in_ticks;  // Начинаем с определения времени когда должно быть вызвано следующее прерывание


    #if gTMR_EnableHWLowPowerTimers_d

    next_Lp_interrupt_time   = mLpMaxToCountDown_c;
    lpTicksSinceLastHere  = currentLpTimeInTicks - previousLpTimeInTicks;
    previousLpTimeInTicks = currentLpTimeInTicks;

    #endif

    // Проходим по списку всех таймеров
    for (timer_nr = 0; timer_nr < NUMBER_OF_ELEMENTS(tmr_timer_table); ++timer_nr)
    {
      timer_type = TMR_GetTimerType(timer_nr);

      TmrIntDisableAll();

      status = TMR_GetTimerStatus(timer_nr);

      // If TMR_StartTimer() has been called for this timer, start it's count down as of now.
      if (status == TMR_STATUS_READY)
      {
        TMR_SetTimerStatus(timer_nr, TMR_STATUS_ACTIVE);

        TmrIntRestoreAll();

        #if gTMR_EnableHWLowPowerTimers_d

        if (IsLowPowerTimer(timer_type))
        {
          if (next_Lp_interrupt_time > tmr_timer_table[timer_nr].remainingTicks)
          {
            next_Lp_interrupt_time = tmr_timer_table[timer_nr].remainingTicks;
          }
        }
        else

        #endif

        // Корректируем наименьшее время до следующего прерываний учитывая данный таймер
        if (next_interrupt_time > tmr_timer_table[timer_nr].remaining_ticks)
        {
          next_interrupt_time = tmr_timer_table[timer_nr].remaining_ticks;
        }
        continue;
      }

      TmrIntRestoreAll();


      if (status != TMR_STATUS_ACTIVE) continue;

      // Здесь продолжаем только если таймер активен


      /* This timer is active. Decrement it's countdown.. */
      #if gTMR_EnableHWLowPowerTimers_d

      if (IsLowPowerTimer(timer_type))
      {
        if (tmr_timer_table[timer_nr].remainingTicks > lpTicksSinceLastHere)
        {
          TmrIntDisableAll();
          tmr_timer_table[timer_nr].remainingTicks -= lpTicksSinceLastHere;
          tmr_timer_table[timer_nr].timestamp = LPTMR_GetCounterValue();
          TmrIntRestoreAll();

          if (next_Lp_interrupt_time > tmr_timer_table[timer_nr].remainingTicks)
          {
            next_Lp_interrupt_time = tmr_timer_table[timer_nr].remainingTicks;
          }
          continue;
        }
      }
      else

      #endif


      // Таймеры установливаются и обслуживаются так чтобы время между прерываниями не было бы короче 4 мс

      if (tmr_timer_table[timer_nr].remaining_ticks > elapsed_time)
      {

        TmrIntDisableAll();
        tmr_timer_table[timer_nr].remaining_ticks -= elapsed_time;
        tmr_timer_table[timer_nr].timestamp = StackTimer_GetCounterValue();
        TmrIntRestoreAll();

        // Корректируем наименьшее время до следующего прерываний учитывая данный таймер
        if (next_interrupt_time > tmr_timer_table[timer_nr].remaining_ticks)
        {
          next_interrupt_time = tmr_timer_table[timer_nr].remaining_ticks;
        }
        continue;
      }

      if ((timer_type & TMR_SINGLE_SHOT_TIMER) || (timer_type & TMR_SET_MINUTE_TIMER) || (timer_type & TMR_SET_SECOND_TIMER))
      {
        // Остановливаем таймер если это одноразовый таймер
        tmr_timer_table[timer_nr].remaining_ticks = 0;
        (void)TMR_StopTimer(timer_nr);
      }
      else
      {
        TmrIntDisableAll();
        #if gTMR_EnableHWLowPowerTimers_d

        if (IsLowPowerTimer(timer_type))
        {
          tmr_timer_table[timer_nr].remainingTicks = tmr_timer_table[timer_nr].intervalInTicks;
          tmr_timer_table[timer_nr].timestamp = LPTMR_GetCounterValue();
          if (next_Lp_interrupt_time > tmr_timer_table[timer_nr].remainingTicks)
          {
            next_Lp_interrupt_time = tmr_timer_table[timer_nr].remainingTicks;
          }
        }
        else

        #endif

        {
          tmr_timer_table[timer_nr].remaining_ticks = tmr_timer_table[timer_nr].interval_in_ticks;
          tmr_timer_table[timer_nr].timestamp = StackTimer_GetCounterValue();
          if (next_interrupt_time > tmr_timer_table[timer_nr].remaining_ticks)
          {
            next_interrupt_time = tmr_timer_table[timer_nr].remaining_ticks;
          }
        }
        TmrIntRestoreAll();
      }
 
      // Время таймера истекло, вызываем назначенную ему функцию 
      call_back_func = tmr_timer_table[timer_nr].call_back_func;
      if (call_back_func) call_back_func(tmr_timer_table[timer_nr].param);

    }  

    TmrIntDisableAll();

    // Узнаем сколько тиков истекло пока мы обрабатывали таймеры
    elapsed_ticks = (uint16_t)(StackTimer_GetCounterValue() - current_timer_counter);

    if (elapsed_ticks >= next_interrupt_time)
    {
      // Мы упустили момент срабатывания ближайшего таймера, откладываем обработку на 4 мс
      next_interrupt_time = elapsed_ticks + ticks_for_4ms;
    }
    else
    {
      // Если до следующей сработки ближайшего таймера осталось меньше 4-х мс, то откладываем сработку этого таймера на 4 мс 
      if ((next_interrupt_time - elapsed_ticks) < ticks_for_4ms)
      {
        next_interrupt_time = elapsed_ticks + ticks_for_4ms;
      }
    }

    next_interrupt_time += current_timer_counter; // Устанавливаем время сработки ближайшего таймера

    if (numberOfActiveTimers
        #if (!gTMR_EnableHWLowPowerTimers_d)
        || numberOfLowPowerActiveTimers
        #endif
       ) /*not about to stop*/
    {
      /*Causes a bug with flex timers if CxV is set before hw timer switches off*/
      StackTimer_SetOffsetTicks(next_interrupt_time);
      if (!timer_hardware_is_running)
      {
        StackTimer_Enable();
        timer_hardware_is_running = TRUE;
      }
    }
    else if (timer_hardware_is_running)
    {
      StackTimer_Disable();
      timer_hardware_is_running = FALSE;
    }

    #if gTMR_EnableHWLowPowerTimers_d

    ticksdiff = LPTMR_GetCounterValue();
    ticksdiff = (uint16_t)(ticksdiff - currentLpTimeInTicks);
    if (ticksdiff >= next_Lp_interrupt_time)
    {
      next_Lp_interrupt_time = ticksdiff + mLpTicksFor4ms;
    }
    else
    {
      if ((next_Lp_interrupt_time - ticksdiff) < mLpTicksFor4ms)
      {
        next_Lp_interrupt_time = ticksdiff + mLpTicksFor4ms;
      }
    }
    /* Update the compare register */
    next_Lp_interrupt_time += currentLpTimeInTicks;

    if (numberOfLowPowerActiveTimers) /*not about to stop*/
    {
      LPTMR_SetOffsetTicks(next_Lp_interrupt_time);
      if (!lpTimerHardwareIsRunning)
      {
        LPTMR_Enable();
        lpTimerHardwareIsRunning = TRUE;
      }
    }
    else if (lpTimerHardwareIsRunning)
    {
      LPTMR_Disable();
      lpTimerHardwareIsRunning = FALSE;
    }

    #endif

    TmrIntRestoreAll();

  }
}

/*---------------------------------------------------------------------------
 * NAME: TMR_EnableTimer
 * DESCRIPTION: Enable the specified timer
 * PARAMETERS:  IN: tmrID - the timer ID
 * RETURN: None
 * NOTES: none
 *---------------------------------------------------------------------------*/
void TMR_EnableTimer(tmrTimerID_t tmrID)
{
  TmrIntDisableAll();

  if (TMR_GetTimerStatus(tmrID) == TMR_STATUS_INACTIVE)
  {
    IncrementActiveTimerNumber(TMR_GetTimerType(tmrID));
    TMR_SetTimerStatus(tmrID, TMR_STATUS_READY);
    (void)OSA_EventSet(&tmr_task_event_object, mTmrDummyEvent_c);
  }

  TmrIntRestoreAll();
}

/*---------------------------------------------------------------------------
 * NAME: TMR_NotCountedMillisTimeBeforeSleep
 * DESCRIPTION: This function is called by Low Power module;
 * Also this function stops the hardware timer.
 * PARAMETERS:  none
 * RETURN: uint32 - time in millisecond that wasn't counted before
 *        entering in sleep
 * NOTES: none
 *---------------------------------------------------------------------------*/
uint16_t TMR_NotCountedTicksBeforeSleep(void)
{
  #if (gTMR_EnableLowPowerTimers_d)
  uint16_t currentTimeInTicks;

  if (!numberOfLowPowerActiveTimers) return 0;

  currentTimeInTicks = StackTimer_GetCounterValue();
  StackTimer_Disable();
  timer_hardware_is_running = FALSE;

  /* The hw timer is stopped but keep timerHardwareIsRunning = TRUE...*/
  /* The Lpm timers are considered as being in running mode, so that  */
  /* not to start the hw timer if a TMR event occurs (this shouldn't happen) */

  return  (uint16_t)(currentTimeInTicks - prev_timer_counter);
  #else
  return 0;
  #endif
}

/*---------------------------------------------------------------------------
 * NAME: TMR_SyncLpmTimers
 * DESCRIPTION: This function is called by the Low Power module
 * each time the MCU wakes up.
 * PARAMETERS:  sleep duration in milliseconds
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void TMR_SyncLpmTimers(uint32_t sleepDurationTmrTicks)
{
  #if (gTMR_EnableLowPowerTimers_d)
  uint32_t       timerID;
  tmrTimerType_t timerType;

  /* Check if there are low power active timer */
  if (!numberOfLowPowerActiveTimers) return;

  /* For each timer, detect the timer type and count down the spent duration in sleep */
  for (timerID = 0; timerID < NUMBER_OF_ELEMENTS(tmr_timer_table); ++timerID)
  {

    /* Detect the timer type and count down the spent duration in sleep */
    timerType = TMR_GetTimerType(timerID);

    /* Sync. only the low power timers that are active */
    if ((TMR_GetTimerStatus(timerID) == TMR_STATUS_ACTIVE)
        && (IsLowPowerTimer(timerType)))
    {
      /* Timer expired when MCU was in sleep mode??? */
      if (tmr_timer_table[timerID].remaining_ticks > sleepDurationTmrTicks)
      {
        tmr_timer_table[timerID].remaining_ticks -= sleepDurationTmrTicks;

      }
      else
      {
        tmr_timer_table[timerID].remaining_ticks = 0;
      }

    }

  } /* end for (timerID = 0;.... */

  StackTimer_Enable();
  prev_timer_counter = StackTimer_GetCounterValue();

  #else
  sleepDurationTmrTicks = sleepDurationTmrTicks;
  #endif /* #if (gTMR_EnableLowPowerTimers_d) */
}
/*---------------------------------------------------------------------------
 * NAME: TMR_MakeTMRThreadReady
 * DESCRIPTION: This function is called by the Low Power module
 * each time the MCU wakes up after low power timers synchronisation.
 * PARAMETERS:
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void TMR_MakeTMRThreadReady(void)
{
  #if (gTMR_EnableLowPowerTimers_d)

  (void)OSA_EventSet(&tmr_task_event_object, mTmrDummyEvent_c);

  #endif /* #if (gTMR_EnableLowPowerTimers_d) */
}

/*---------------------------------------------------------------------------
 * NAME: TMR_GetTimerFreq
 * DESCRIPTION:
 * PARAMETERS: -
 * RETURN: -
 *---------------------------------------------------------------------------*/
uint32_t TMR_GetTimerFreq(void)
{
  return mCounterFreqHz;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_TimeStampInit
 * DESCRIPTION: Initialize the timestamp module
 * PARAMETERS: -
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_TimeStampInit(void)
{
  #if gTMR_PIT_Timestamp_Enabled_d
  TMR_PITInit();
  #else
  TMR_RTCInit();
  #endif
}

/*---------------------------------------------------------------------------
 * NAME: TMR_GetTimestamp
 * DESCRIPTION: return an 64-bit absolute timestamp
 * PARAMETERS: -
 * RETURN: absolute timestamp in us
 *---------------------------------------------------------------------------*/
uint64_t TMR_GetTimestamp(void)
{
  #if gTMR_PIT_Timestamp_Enabled_d
  return TMR_PITGetTimestamp();
  #else
  return TMR_RTCGetTimestamp();
  #endif
}

#endif /*gTMR_Enabled_d*/

#if gPrecision_Timers_Enabled_d

/*---------------------------------------------------------------------------
 * NAME: TMR_PrecisionTimerInit
 * DESCRIPTION: initialize the precision timer module
 * PARAMETERS: -
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_PrecisionTimerInit()
{
  globalTimeTicks = 0;

  gPrecisionTimerHandle = PrecisionTimer_Init(NULL);

  PrecisionTimer_OnCounterRestart_fptr = TMR_PrecisionTimerOverflowNotify;

  if (PrecisionTimer_Enable(gPrecisionTimerHandle) != ERR_OK)
  {
    Panic(0, (uint32_t)TMR_Init, 0, 0);
    return;
  }

  gPrecisionTimerTickus = 1000000U / PrecisionTimer_GetInputFrequency(gPrecisionTimerHandle);
  gPrecisionTimerTickusReal = 1000000.0F / PrecisionTimer_GetInputFrequencyReal(gPrecisionTimerHandle);
  PrecisionTimer_GetPeriodTicks(gPrecisionTimerHandle, (uint32_t *)&gPrecisionTimerPeriodTicks);

  (void)gPrecisionTimerTickus;
  (void)gPrecisionTimerTickusReal;
  (void)gPrecisionTimerPeriodTicks;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_GetAbsoluteTimeus
 * DESCRIPTION: Gets the absolute time in microseconds.
 * PARAMETERS:  None
 * RETURN: Time in microseconds
 * NOTES:
 *---------------------------------------------------------------------------*/
uint64_t TMR_GetAbsoluteTimeus()
{
  uint64_t globalTime;
  uint64_t ticks;

  TmrIntDisableAll();
  ticks = gPrecisionTimerPeriodTicks - PrecisionTimer_GetCounterValue(gPrecisionTimerHandle) + globalTimeTicks;
  globalTime = (uint64_t)(ticks * gPrecisionTimerTickusReal);
  TmrIntRestoreAll();

  return globalTime;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_PrecisionTimerOverflowNotify
 * DESCRIPTION: Event function called by driver ISR on timer overflow in interrupt context.
 * PARAMETERS:  IN: User data; unused.
 * RETURN: none
 * NOTES: none
 *---------------------------------------------------------------------------*/
void TMR_PrecisionTimerOverflowNotify(LDD_TUserData *UserDataPtr)
{
  uint32_t period;

  (void)UserDataPtr;

  /* I assume here that int_kernel_isr calls the ISR with interrupts disabled*/
  PrecisionTimer_GetPeriodTicks(gPrecisionTimerHandle, &period);
  globalTimeTicks += period;
}

#endif /*gPrecision_Timers_Enabled_d*/

/*---------------------------------------------------------------------------
 * NAME: TMR_RTCIsOscStarted
 * DESCRIPTION: returns the state of the RTC oscillator
 * PARAMETERS: -
 * RETURN: -
 *---------------------------------------------------------------------------*/
bool_t TMR_RTCIsOscStarted()
{
  return TRUE;
}
#if gTimestamp_Enabled_d

/*---------------------------------------------------------------------------
 * NAME: TMR_RTCInit
 * DESCRIPTION: initialize the RTC part of the timer module
 * PARAMETERS: -
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_RTCInit()
{
  static uint8_t         gRTCInitFlag = 0;
  RTC_Type               *rtcBaseAddr = g_rtcBase[gTmrRtcInstance_c];
  extern const IRQn_Type g_rtcIrqId[];

  TmrIntDisableAll();

  if (gRTCInitFlag)
  {
    TmrIntRestoreAll();
    return; /* already initialized */
  }

  gRTCInitFlag = TRUE;

  /* Overwrite old ISR */
  OSA_InstallIntHandler(g_rtcIrqId[gTmrRtcInstance_c], TMR_RTCAlarmNotify);

  RTC_DRV_Init(gTmrRtcInstance_c);
  /* Reset the seconds register */
  RTC_HAL_EnableCounter(rtcBaseAddr, FALSE);
  RTC_HAL_SetSecsReg(rtcBaseAddr, 1);

  gRTCTimeOffset = 0;
  gRTCPrescalerOffset = 0;
  gRTCAlarmCallback = NULL;
  gRTCAlarmCallbackParam = NULL;

  RTC_HAL_SetAlarmIntCmd(rtcBaseAddr, FALSE);
  RTC_HAL_SetSecsReg(rtcBaseAddr, 0x00 + 0x01);
  RTC_HAL_SetPrescaler(rtcBaseAddr, 0x00);

  RTC_HAL_EnableCounter(rtcBaseAddr, TRUE);

  TmrIntRestoreAll();
}

/*---------------------------------------------------------------------------
 * NAME: TMR_RTCGetTimestamp
 * DESCRIPTION: Returns the absolute time at the moment of the call.
 * PARAMETERS: -
 * RETURN: Absolute time at the moment of the call in microseconds.
 *---------------------------------------------------------------------------*/
uint64_t TMR_RTCGetTimestamp()
{
  uint32_t seconds1,
           seconds2,
           prescaler0,
           prescaler1,
           prescaler2;
  uint64_t useconds,
           offset,
           prescalerOffset;
  RTC_Type *rtcBaseAddr    = g_rtcBase[gTmrRtcInstance_c];

  TmrIntDisableAll();
  offset = gRTCTimeOffset;
  prescalerOffset = gRTCPrescalerOffset;

  prescaler0 = RTC_HAL_GetPrescaler(rtcBaseAddr);
  seconds1   = RTC_HAL_GetSecsReg(rtcBaseAddr);
  prescaler1 = RTC_HAL_GetPrescaler(rtcBaseAddr);
  seconds2   = RTC_HAL_GetSecsReg(rtcBaseAddr);
  prescaler2 = RTC_HAL_GetPrescaler(rtcBaseAddr);

  TmrIntRestoreAll();

  prescaler0 &= 0x7fff;
  seconds1 -= 1;
  prescaler1 &= 0x7fff;
  seconds2 -= 1;
  prescaler2 &= 0x7fff;

  if (seconds1 != seconds2)
  {
    seconds1 = seconds2;
    prescaler1 = prescaler2;
  }
  else
  {
    if (prescaler1 != prescaler2)
    {
      prescaler1 = prescaler0;
    }
  }

  useconds = ((prescaler1 + prescalerOffset + ((uint64_t)seconds1 << 15)) * 15625) >> 9;

  return useconds + offset;
}

/*---------------------------------------------------------------------------
 * NAME: TMR_RTCSetTime
 * DESCRIPTION: Sets the absolute time.
 * PARAMETERS: Time in microseconds.
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_RTCSetTime(uint64_t microseconds)
{
  uint64_t initialAlarm;
  RTC_Type *rtcBaseAddr = g_rtcBase[gTmrRtcInstance_c];

  TmrIntDisableAll();
  RTC_HAL_EnableCounter(rtcBaseAddr, FALSE);

  initialAlarm = gRTCTimeOffset;
  initialAlarm = RTC_HAL_GetAlarmReg(rtcBaseAddr) + (initialAlarm / 1000000L);
  gRTCTimeOffset = microseconds;

  RTC_HAL_SetSecsReg(rtcBaseAddr, 0x00 + 0x01);
  RTC_HAL_SetPrescaler(rtcBaseAddr, 0x00);

  if ((initialAlarm * 1000000L) <= microseconds)
  {
    RTC_HAL_SetAlarmIntCmd(rtcBaseAddr, FALSE);
    if (gRTCAlarmCallback != NULL)
    {
      gRTCAlarmCallback(gRTCAlarmCallbackParam);
    }
  }
  else
  {
    RTC_HAL_SetAlarmReg(rtcBaseAddr, initialAlarm - (microseconds / 1000000L));
  }

  RTC_HAL_EnableCounter(rtcBaseAddr, TRUE);
  TmrIntRestoreAll();
}

/*---------------------------------------------------------------------------
 * NAME: TMR_RTCSetAlarm
 * DESCRIPTION: Sets the alarm absolute time in seconds.
 * PARAMETERS: Time in seconds for the alarm. Callback function pointer. Parameter for callback.
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_RTCSetAlarm(uint64_t seconds, pfTmrCallBack_t callback, void *param)
{
  RTC_Type *rtcBaseAddr = g_rtcBase[gTmrRtcInstance_c];

  TmrIntDisableAll();

  gRTCAlarmCallback = callback;
  gRTCAlarmCallbackParam = param;
  seconds = seconds - (gRTCTimeOffset / 1000000L);
  RTC_HAL_SetAlarmReg(rtcBaseAddr, seconds);
  RTC_HAL_SetAlarmIntCmd(rtcBaseAddr, TRUE);

  TmrIntRestoreAll();
}

/*---------------------------------------------------------------------------
 * NAME: TMR_RTCSetAlarmRelative
 * DESCRIPTION: Sets the alarm relative time in seconds.
 * PARAMETERS: Time in seconds for the alarm. Callback function pointer. Parameter for callback.
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_RTCSetAlarmRelative(uint32_t seconds, pfTmrCallBack_t callback, void *param)
{
  uint32_t rtcSeconds,
           rtcPrescaler;
  RTC_Type *rtcBaseAddr = g_rtcBase[gTmrRtcInstance_c];

  if (seconds == 0)
  {
    callback(param);
    return;
  }

  TmrIntDisableAll();

  RTC_HAL_EnableCounter(rtcBaseAddr, FALSE);
  rtcSeconds = RTC_HAL_GetSecsReg(rtcBaseAddr);
  rtcPrescaler = RTC_HAL_GetPrescaler(rtcBaseAddr);
  RTC_HAL_SetPrescaler(rtcBaseAddr, 0x00);
  /*If bit prescaler 14 transitions from 1 to 0 the seconds reg get incremented.
  Rewrite seconds register to prevent this.*/
  RTC_HAL_SetSecsReg(rtcBaseAddr, rtcSeconds);
  RTC_HAL_EnableCounter(rtcBaseAddr, TRUE);
  rtcPrescaler &= 0x7fff;

  gRTCPrescalerOffset += rtcPrescaler;

  if (gRTCPrescalerOffset & 0x8000)
  {
    rtcSeconds++;
    RTC_HAL_EnableCounter(rtcBaseAddr, FALSE);
    RTC_HAL_SetSecsReg(rtcBaseAddr, rtcSeconds);
    RTC_HAL_EnableCounter(rtcBaseAddr, TRUE);
    gRTCPrescalerOffset = gRTCPrescalerOffset & 0x7FFF;
  }

  gRTCAlarmCallback = callback;
  gRTCAlarmCallbackParam = param;
  RTC_HAL_SetAlarmReg(rtcBaseAddr,  seconds + rtcSeconds - 1);
  RTC_HAL_SetAlarmIntCmd(rtcBaseAddr, TRUE);

  TmrIntRestoreAll();
}


#endif /*gTimestamp_Enabled_d*/

#if gTMR_PIT_Timestamp_Enabled_d

  #if (FSL_FEATURE_PIT_TIMER_COUNT >= 3)
/*---------------------------------------------------------------------------
 * NAME: TMR_PITInit
 * DESCRIPTION: initialize the PIT part of the timer module
 * PARAMETERS: -
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_PITInit()
{
  static uint8_t gPITInitFlag = FALSE;
  PIT_Type       *baseAddr    = g_pitBase[gTmrPitInstance_c];
  uint32_t       pitFreq;

  if (gPITInitFlag)
  {
    return; /*already inited*/
  }

  TmrIntDisableAll();

  gPITInitFlag = TRUE;

  PIT_DRV_Init(gTmrPitInstance_c, FALSE);
  PIT_HAL_Disable(baseAddr);

  pitFreq = CLOCK_SYS_GetPitFreq(gTmrPitInstance_c);

  PIT_HAL_SetTimerPeriodByCount(baseAddr, 0, pitFreq / 1000000 - 1);
  PIT_HAL_SetTimerPeriodByCount(baseAddr, 1, 0xFFFFFFFF);
  PIT_HAL_SetTimerPeriodByCount(baseAddr, 2, 0xFFFFFFFF);

  PIT_HAL_SetTimerChainCmd(baseAddr, 1, TRUE);
  PIT_HAL_SetTimerChainCmd(baseAddr, 2, TRUE);

  PIT_HAL_StartTimer(baseAddr, 0);
  PIT_HAL_StartTimer(baseAddr, 1);
  PIT_HAL_StartTimer(baseAddr, 2);

  PIT_HAL_Enable(baseAddr);

  TmrIntRestoreAll();
}

/*---------------------------------------------------------------------------
 * NAME: TMR_PITGetTimestamp
 * DESCRIPTION: Returns the absolute time at the moment of the call.
 * PARAMETERS: -
 * RETURN: Absolute time at the moment of the call in microseconds.
 *---------------------------------------------------------------------------*/
uint64_t TMR_PITGetTimestamp()
{
  PIT_Type *baseAddr = g_pitBase[gTmrPitInstance_c];
  uint32_t pit2_0,
           pit2_1 ,
           pit1_0,
           pit1_1;
  uint64_t useconds;

  TmrIntDisableAll();
  pit1_0 = PIT_HAL_ReadTimerCount(baseAddr, 1);
  pit2_0 = PIT_HAL_ReadTimerCount(baseAddr, 2);
  pit1_1 = PIT_HAL_ReadTimerCount(baseAddr, 1);
  pit2_1 = PIT_HAL_ReadTimerCount(baseAddr, 2);
  TmrIntRestoreAll();

  if (pit1_1 <= pit1_0)
  {
    useconds = pit2_0;
  }
  else
  {
    useconds = pit2_1;
  }

  useconds <<= 32;
  useconds += pit1_1;
  useconds = ~useconds;
  #if !gTMR_PIT_FreqMultipleOfMHZ_d
  {
    uint32_t pitFreq;
    uint32_t pitLoadVal;

    pitFreq = CLOCK_SYS_GetPitFreq(gTmrPitInstance_c);
    pitLoadVal = PIT_HAL_GetTimerPeriodByCount(baseAddr, 0) + 1;
    pitLoadVal *= 1000000;

    if (pitFreq != pitLoadVal)
    {
      /*
      To adjust the value to useconds the following formula is used.
      useconds = (useconds*pitLoadVal)/pitFreq;
      Because this formula causes overflow the useconds/pitFreq is split in its Integer  and Fractional part.
      */
      uint64_t uSecAdjust1 , uSecAdjust2;
      uSecAdjust1  = useconds / pitFreq;
      uSecAdjust2  = useconds % pitFreq;
      uSecAdjust1 *= pitLoadVal;
      uSecAdjust2 *= pitLoadVal;
      uSecAdjust2 /= pitFreq;
      useconds = uSecAdjust1 + uSecAdjust2;
    }
  }
  #endif//gTMR_PIT_FreqMultipleOfMHZ_d
  return useconds;
}

  #else

/*---------------------------------------------------------------------------
 * NAME: TMR_PITInit
 * DESCRIPTION: initialize the PIT part of the timer module
 * PARAMETERS: -
 * RETURN: -
 *---------------------------------------------------------------------------*/
void TMR_PITInit()
{
  static uint8_t gPITInitFlag = FALSE;
  PIT_Type       *baseAddr    = g_pitBase[gTmrPitInstance_c];
  uint32_t       pitFreq;

  TmrIntDisableAll();

  if (gPITInitFlag)
  {
    TmrIntRestoreAll();
    return; /*already inited*/
  }

  gPITInitFlag = TRUE;
  mPIT_TimestampHigh = (uint32_t)-1;

  CLOCK_SYS_EnablePitClock(gTmrPitInstance_c);
  //PIT_HAL_Disable(baseAddr);

  pitFreq = CLOCK_SYS_GetPitFreq(gTmrPitInstance_c);

  PIT_HAL_SetTimerPeriodByCount(baseAddr, 0, pitFreq / 1000000 - 1);
  PIT_HAL_SetTimerPeriodByCount(baseAddr, 1, 0xFFFFFFFF);

  PIT_HAL_SetTimerChainCmd(baseAddr, 1, TRUE);
  PIT_HAL_SetIntCmd(baseAddr, 1, TRUE);

  /* Overwrite old ISR */
  OSA_InstallIntHandler(g_pitIrqId[gTmrPitInstance_c], TMR_PIT_ISR);
  NVIC_ClearPendingIRQ(g_pitIrqId[gTmrPitInstance_c]);
  NVIC_EnableIRQ(g_pitIrqId[gTmrPitInstance_c]);

  PIT_HAL_Enable(baseAddr);

  PIT_HAL_StartTimer(baseAddr, 1);
  PIT_HAL_StartTimer(baseAddr, 0);

  TmrIntRestoreAll();
}

/*---------------------------------------------------------------------------
 * NAME: TMR_PITGetTimestamp
 * DESCRIPTION: Returns the absolute time at the moment of the call.
 * PARAMETERS: -
 * RETURN: Absolute time at the moment of the call in microseconds.
 *---------------------------------------------------------------------------*/
uint64_t TMR_PITGetTimestamp()
{
  PIT_Type *baseAddr = g_pitBase[gTmrPitInstance_c];
  uint32_t pit2,
           pit1_0,
           pit1_1,
           pitIF;
  uint64_t useconds;

  TmrIntDisableAll();

  pit2 = mPIT_TimestampHigh;
  pit1_0 = PIT_HAL_ReadTimerCount(baseAddr, 1);
  pitIF = PIT_HAL_IsIntPending(baseAddr, 1);
  pit1_1 = PIT_HAL_ReadTimerCount(baseAddr, 1);

  TmrIntRestoreAll();

  if (pitIF)
  {
    useconds = pit2 - 1;
  }
  else
  {
    useconds = pit2;
  }
  useconds <<= 32;
  if (pitIF)
  {
    useconds += pit1_1;
  }
  else
  {
    useconds += pit1_0;
  }
  useconds = ~useconds;
  #if !gTMR_PIT_FreqMultipleOfMHZ_d
  {
    uint32_t pitFreq;
    uint32_t pitLoadVal;

    pitFreq = CLOCK_SYS_GetPitFreq(gTmrPitInstance_c);
    pitLoadVal = PIT_HAL_GetTimerPeriodByCount(baseAddr, 0) + 1;
    pitLoadVal *= 1000000;

    if (pitFreq != pitLoadVal)
    {
      /*
      To adjust the value to useconds the following formula is used.
      useconds = (useconds*pitLoadVal)/pitFreq;
      Because this formula causes overflow the useconds/pitFreq is split in its Integer  and Fractional part.
      */
      uint64_t uSecAdjust1 , uSecAdjust2;
      uSecAdjust1  = useconds / pitFreq;
      uSecAdjust2  = useconds % pitFreq;
      uSecAdjust1 *= pitLoadVal;
      uSecAdjust2 *= pitLoadVal;
      uSecAdjust2 /= pitFreq;
      useconds = uSecAdjust1 + uSecAdjust2;
    }
  }
  #endif//gTMR_PIT_FreqMultipleOfMHZ_d
  return useconds;
}

  #endif

#endif//gTMR_PIT_Timestamp_Enabled_d


/*****************************************************************************
 *                               <<< EOF >>>                                 *
 *****************************************************************************/
