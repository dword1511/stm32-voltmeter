#include <stdbool.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>

#include "disp/disp.h"
#include "tick.h"
#include "power.h"
#include "analog.h"


int main(void) {
  uint32_t mvolts = 0;

  power_setup();
  disp_setup();
  disp_update(0, true);
  tick_setup();
  analog_setup();

  /* Power on display test */
  disp_update( 8888, true);
  tick_delay(500);
  disp_update(88888, true); /* Also tests rounding */
  tick_delay(500);
  disp_update(mvolts, false);

#ifdef DISP_DO_TEST
  while (true) {
    bool ticker = false;

    for (mvolts = 0; mvolts < 10000; mvolts += 1111) {
      disp_update(mvolts, ticker);
      tick_delay(500);
      ticker = !ticker;
    }

    disp_update(100000, ticker); /* Test "E" */
    tick_delay(500);
  }
#else /* DISP_DO_TEST */
  /* Main loop */
  while (!tick_auto_poweroff()) {
    disp_update(mvolts, true);
    mvolts = analog_read();
    disp_update(mvolts, false);
    tick_delay(500);
  }

  power_off();
#endif /* DISP_DO_TEST */

  return 0;
}
