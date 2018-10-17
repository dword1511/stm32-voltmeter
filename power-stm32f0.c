#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>

#include "power.h"


#define F_HSI         8000000
#define F_DIV         128

#define _CONCAT(a, b) a##b
#define CONCAT(a, b)  _CONCAT(a, b)
#define RCC_HPRE      CONCAT(RCC_CFGR_HPRE_DIV, F_DIV)


void power_setup(void) {
  rcc_disable_rtc_clock();

  /* Setup clock to around 62kHz, any lower LED segment display may not work. */
  /* MCU is on HSI after reset */
  /*
  rcc_osc_on(RCC_HSI);
  rcc_wait_for_osc_ready(RCC_HSI);
  rcc_set_sysclk_source(RCC_HSI);
  */

  rcc_set_hpre(RCC_HPRE);
  rcc_set_ppre(RCC_CFGR_PPRE_NODIV);

  flash_set_ws(FLASH_ACR_LATENCY_000_024MHZ);

  rcc_apb1_frequency  = F_HSI / F_DIV;
  rcc_ahb_frequency   = F_HSI / F_DIV;
}
