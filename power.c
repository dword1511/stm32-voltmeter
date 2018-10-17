#include <libopencm3/cm3/scb.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rtc.h>

#include "power.h"


void power_off(void) {
  /* Stop GPIO (and display) */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_GPIOD);
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  gpio_mode_setup(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  gpio_mode_setup(GPIOD, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);

  rcc_periph_clock_enable(RCC_PWR);
  pwr_disable_power_voltage_detect();
  pwr_disable_backup_domain_write_protect();
  RCC_CSR &= ~RCC_CSR_RTCEN;
  rtc_wait_for_synchro();
  rcc_osc_off(RCC_LSI);
  pwr_enable_backup_domain_write_protect();

  /* According to ST App note... */
  SCB_SCR |= SCB_SCR_SLEEPDEEP;
  /* Only sets PDDS... Well, others are universal to CM3 anyway. */
  pwr_set_standby_mode();
  pwr_clear_wakeup_flag();
  /* Enter standby */
  asm("wfi");
}

void nmi_handler(void)
__attribute__ ((alias ("power_off")));

void hard_fault_handler(void)
__attribute__ ((alias ("power_off")));
