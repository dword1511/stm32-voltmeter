#include <stdbool.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>

#include "disp.h"
#include "tick.h"
#include "power.h"
#include "analog.h"


int main(void) {
  bool     ticker = false;
  uint32_t mvolts = 0;

  power_setup();
  disp_setup();
  disp_update(0, true);
  tick_setup();
  adc_setup();

  /* Power on display test */
  disp_update( 8888, true);
  tick_delay(500);
  disp_update(88888, true); /* Also tests rounding */
  tick_delay(500);
  disp_update(mvolts, false);

#ifdef DISP_DO_TEST
  while (true) {
    for (mvolts = 0; mvolts < 10000; mvolts += 1111) {
      disp_update(mvolts, ticker);
      tick_delay(1500);
      ticker = !ticker;
    }
    disp_update(100000, ticker); /* Test "E" */
  }
#else /* DISP_DO_TEST */
  /* Main loop */
  while (!tick_auto_poweroff()) {
    disp_update(mvolts, ticker);
    mvolts = adc_read();
    disp_update(mvolts, ticker);
    ticker = !ticker;
  }

  power_off();
#endif /* DISP_DO_TEST */

  return 0;
}
