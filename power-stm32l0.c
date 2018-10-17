#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/pwr.h>

#include "power.h"


void power_setup(void) {
  /* Setup clock to around 62kHz, any lower LED segment display may not work. */
  /* MCU is on HSI after reset */
  RCC_ICSCR = (RCC_ICSCR & (~(RCC_ICSCR_MSIRANGE_MASK << RCC_ICSCR_MSIRANGE_SHIFT))) | (RCC_ICSCR_MSIRANGE_65KHZ << RCC_ICSCR_MSIRANGE_SHIFT); /* 65.536kHz MSI */

  rcc_set_hpre(RCC_CFGR_HPRE_NODIV);
  rcc_set_ppre1(RCC_CFGR_PPRE1_NODIV);
  rcc_set_ppre2(RCC_CFGR_PPRE2_NODIV);

  rcc_periph_clock_enable(RCC_PWR);
  pwr_set_vos_scale(PWR_SCALE3);
  pwr_voltage_regulator_low_power_in_stop();
  PWR_CR |= PWR_CR_LPSDSR;
  //PWR_CR |= PWR_CR_LPRUN; /* NOTE: this does not work... */

  //flash_prefetch_enable();
  //flash_set_ws(FLASH_ACR_LATENCY_0WS);

  rcc_ahb_frequency   = 65536;
  rcc_apb1_frequency  = 65536;
  rcc_apb2_frequency  = 65536;

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
}
