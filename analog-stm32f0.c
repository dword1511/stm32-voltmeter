#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/gpio.h>

#include "tick.h"
#include "analog.h"
#include "analog-common.h"


#if ADC_OVERSAMPLE > 16
#error "ADC_OVERSAMPLE should not exceed 16!"
#endif


/* BEGIN: Missing bits */
#define PWR_CSR_VREFINTRDY  (1 << 3)
/* END: Missing bits */


void analog_setup(void) {
  __analog_setup_gpio();

  rcc_periph_clock_enable(RCC_ADC1);
  adc_enable_vrefint();

  adc_power_off(ADC1);
  adc_set_clk_source(ADC1, ADC_CLKSOURCE_PCLK_DIV4); /* Set ADC clock to ~15 kHz */
  adc_calibrate(ADC1);
  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_239DOT5); /* (239.5 + 1.5 ~= 62 sps) */

  adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);
  adc_set_right_aligned(ADC1);
  adc_set_single_conversion_mode(ADC1);
  //ADC_CFGR1(ADC1) |= ADC_CFGR1_AUTDLY; /* Do not start conversion until DR is read. Avoids race conditions. */
  adc_disable_analog_watchdog(ADC1);
  adc_disable_external_trigger_regular(ADC1);
  adc_disable_temperature_sensor();
  adc_power_on(ADC1);

  /* Wait for ADC to start up. */
  tick_delay(100);

  /* Wait for internal reference to start up. */
  while (!(PWR_CSR & PWR_CSR_VREFINTRDY));
}

uint32_t analog_read(void) {
  uint8_t   i;
  uint32_t  total = 0, vref_lsbs = 0;

  /* Establish LSB to uv mapping */
  for (i = 0; i < ANALOG_OVERSAMPLE; i ++) {
    vref_lsbs += __analog_read_raw(ADC_CHANNEL_VREF);
  }

  for (i = 0; i < ANALOG_OVERSAMPLE; i ++) {
    total     += __analog_read_raw(ANALOG_INPUT_CHANNEL);
  }

  return __analog_get_mv(vref_lsbs, total);
}
