#include  "BSP.h"

#include "fsl_clock_manager.h"
#include "fsl_smc_hal.h"
#include "fsl_debug_console.h"
#include "fsl_gpio_driver.h"
#include "RNG_Interface.h"


/* Configuration for enter VLPR mode. Core clock = 4MHz. */
const clock_manager_user_config_t g_defaultClockConfigVlpr =
{
  .mcgConfig =
  {
    .mcg_mode           = kMcgModeBLPI,   // Work in BLPI mode.
    .irclkEnable        = true,  // MCGIRCLK enable.
    .irclkEnableInStop  = false, // MCGIRCLK disable in STOP mode.
    .ircs               = kMcgIrcFast, // Select IRC4M.
    .fcrdiv             = 0U,    // FCRDIV is 0.

    .frdiv   = 5U,
    .drs     = kMcgDcoRangeSelLow,  // Low frequency range
    .dmx32   = kMcgDmx32Default,    // DCO has a default range of 25%
    .oscsel  = kMcgOscselOsc,       // Select OSC

  },
  .simConfig =
  {
    .er32kSrc  = kClockEr32kSrcOsc0,     // ERCLK32K selection, use OSC0.
    .outdiv1   = 0U,
    .outdiv4   = 4U,
  }
};

/* Configuration for enter RUN mode. Core clock = 32MHz. */
const clock_manager_user_config_t g_defaultClockConfigRun =
{
  .mcgConfig =
  {
    .mcg_mode           = kMcgModeBLPE, // Work in BLPE mode.
    .irclkEnable        = true,  // MCGIRCLK enable.
    .irclkEnableInStop  = false, // MCGIRCLK disable in STOP mode.
    .ircs               = kMcgIrcSlow, // Select IRC32k.
    .fcrdiv             = 0U,    // FCRDIV is 0.

    .frdiv   = 5U,
    .drs     = kMcgDcoRangeSelLow,  // Low frequency range
    .dmx32   = kMcgDmx32Default,    // DCO has a default range of 25%
    .oscsel  = kMcgOscselOsc,       // Select
  },
  .simConfig =
  {
    .pllFllSel = kClockPllFllSelFll,    // PLLFLLSEL select FLL.
    .er32kSrc  = kClockEr32kSrcLpo,     // ERCLK32K selection, use LPO.
    .outdiv1   = 0U,
    .outdiv4   = 1U,
  }
};

static void CLOCK_SetBootConfig(clock_manager_user_config_t const *config);
extern uint32_t g_xtal0ClkFreq;           /* EXTAL0 clock */
extern uint32_t SystemCoreClock;          /* in Kinetis SDK, this contains the system core clock speed */

/*------------------------------------------------------------------------------
  Function to initialize RTC external clock base on board configuration.
 ------------------------------------------------------------------------------*/
void Init_RTC_Osc(void)
{
  rtc_osc_user_config_t rtcOscConfig =
  {
    .freq                = RTC_XTAL_FREQ,
    .enableCapacitor2p   = RTC_SC2P_ENABLE_CONFIG,
    .enableCapacitor4p   = RTC_SC4P_ENABLE_CONFIG,
    .enableCapacitor8p   = RTC_SC8P_ENABLE_CONFIG,
    .enableCapacitor16p  = RTC_SC16P_ENABLE_CONFIG,
    .enableOsc           = RTC_OSC_ENABLE_CONFIG,
  };

  CLOCK_SYS_RtcOscInit(0U, &rtcOscConfig);
}

/*------------------------------------------------------------------------------

 \param config
 ------------------------------------------------------------------------------*/
static void CLOCK_SetBootConfig(clock_manager_user_config_t const *config)
{
  CLOCK_SYS_SetSimConfigration(&config->simConfig);

  CLOCK_SYS_SetMcgMode(&config->mcgConfig);

}



/*------------------------------------------------------------------------------
 
  Вызывается в модуле startup до того как будут инициализированы области переменных
  поэтому здесь нельзя назначать значения рабочим переменным

 ------------------------------------------------------------------------------*/
void Clock_init(void)
{

  MCG->C2 = (MCG->C2 & ~LSHIFT(0x03, 4)) | LSHIFT(1, 4); // Frequency Range Select | 01 Encoding 1 — High frequency range selected for the crystal oscillator .

  Init_RTC_Osc();

#if (CLOCK_INIT_CONFIG == CLOCK_VLPR)
  CLOCK_SetBootConfig(&g_defaultClockConfigVlpr);
#else
  CLOCK_SetBootConfig(&g_defaultClockConfigRun);
#endif

  // Таймер TPM тактируется от частоты осциллятора
  SIM->SOPT2 = (SIM->SOPT2 & ~LSHIFT(0x03, 24)) | LSHIFT(2, 24);  // TPM Clock Source Select | 10 OSCERCLK clock 
}

