#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>

#include "power.h"


void power_setup(void) {
  /* Setup clock to conserve power */
  rcc_osc_on(RCC_HSI);
  rcc_wait_for_osc_ready(RCC_HSI);
  rcc_set_sysclk_source(RCC_HSI);

  rcc_set_hpre(RCC_CFGR_HPRE_DIV4);
  rcc_set_ppre(RCC_CFGR_PPRE_NODIV);

  flash_set_ws(FLASH_ACR_LATENCY_000_024MHZ);

  /* HSI is 8MHz, divided by 4. */
  rcc_apb1_frequency  = 2000000;
  rcc_ahb_frequency   = 2000000;

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
}
