#include <libopencm3/cm3/scb.h>

#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/gpio.h>

#include "power.h"


void power_off(void) {
  /* Stop GPIO (and display) */
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  //gpio_mode_setup(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  //gpio_mode_setup(GPIOD, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  //gpio_mode_setup(GPIOE, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  //gpio_mode_setup(GPIOF, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  //gpio_mode_setup(GPIOG, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);
  //gpio_mode_setup(GPIOH, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_ALL);

  pwr_disable_power_voltage_detect();

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
