#include <stdbool.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>

#include <libopencm3/stm32/rcc.h>


#include "tick.h"
#include "disp.h"


/*
#define TICK_HZ           200
#define TICK_PWROFF_SECS  20
*/


#if TICK_PWROFF_SECS != 0
#define TICK_AUTO_POWEROFF
#endif

#define SYSCLK_PERIOD_MS  (1000 / (TICK_HZ))
#define SYSCLK_PERIOD     ((rcc_ahb_frequency) * (SYSCLK_PERIOD_MS) / 1000 - 1)


#ifdef TICK_AUTO_POWEROFF
static volatile uint32_t  uptime_ms = 0;
#endif


void sys_tick_handler(void) {
  uptime_ms += SYSCLK_PERIOD_MS;

#ifdef DISP_NEEDS_REFRESH
  disp_refresh();
#endif
}

void tick_setup(void) {
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_clear();

  systick_set_reload(SYSCLK_PERIOD);
  systick_interrupt_enable();
  systick_counter_enable();
}

bool tick_auto_poweroff(void) {
#ifdef TICK_AUTO_POWEROFF
  return uptime_ms > (TICK_PWROFF_SECS * 1000);
#else
  return false;
#endif
}

void tick_delay(uint32_t ms) {
  //uint32_t target1 = ((uptime_ms + ms) > SYSCLK_PERIOD_MS) ? (uptime_ms + ms - SYSCLK_PERIOD_MS) : 0;
  //uint32_t target2 = uptime_ms + ms; /* Warps every 49 days... */
  uint32_t target = uptime_ms + ms;

  while (uptime_ms < target) {
    asm("wfi");
  }
}
