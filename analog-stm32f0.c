#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/gpio.h>

#include "tick.h"
#include "analog.h"


/*
#define ADC_INPUT_BANK      GPIOB
#define ADC_INPUT_PIN       GPIO1
#define ADC_INPUT_CHANNEL   9
#define ADC_OVERSAMPLE      16
*/

#if ADC_OVERSAMPLE > 16
#error "ADC_OVERSAMPLE should not exceed 16!"
#endif

#ifndef ADC_RDIV_LOWER
/* ADC_RDIV_LOWER is inf */
#define ADC_RDIV_UPPER      0
#define ADC_RDIV_LOWER      1
#endif
#define ADC_NUMERATOR       (ADC_RDIV_UPPER + ADC_RDIV_LOWER)
#define ADC_DENUMERATOR     (ADC_RDIV_LOWER)

#define PWR_CSR_VREFINTRDY  (1 << 3)


static uint16_t adc_read_raw(uint8_t ch) {
  adc_set_regular_sequence(ADC1, 1, &ch);
  adc_start_conversion_regular(ADC1);
  while (!adc_eoc(ADC1)) {
    asm("wfi"); /* Wait for one tick */
  }
  return adc_read_regular(ADC1);
}

void adc_setup(void) {
  gpio_mode_setup(ADC_INPUT_BANK, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_INPUT_PIN);

  rcc_periph_clock_enable(RCC_ADC1);
  adc_enable_vrefint();
  adc_disable_temperature_sensor();

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
  adc_power_on(ADC1);

  /* Wait for ADC to start up. */
  tick_delay(100);

  /* Wait for internal reference to start up. */
  while (!(PWR_CSR & PWR_CSR_VREFINTRDY));
}

uint32_t adc_read(void) {
  uint8_t   i;
  uint16_t  sample, muv_per_lsb;
  uint32_t  total = 0, vref_muv, vref_lsbs = 0;

  /* Establish LSB to uv mapping */
  vref_muv = ST_VREFINT_CAL * ADC_OVERSAMPLE;
  for (i = 0; i < ADC_OVERSAMPLE; i ++) {
    vref_lsbs += adc_read_raw(ADC_CHANNEL_VREF);
  }
  /* VDDA = 3.3 V (3,300,000 uV) x VREFINT_CAL / VREFINT_DATA; uV/LSB = VDDA / (2 ^ #BITS) */
  /* NOTE: ST's manual has a typo here. This is at 30 degree Celcius and 3.3V +/- 10mV. */
  muv_per_lsb = ((uint64_t)vref_muv) * 3300000 / (1 << 12) / vref_lsbs;

  for (i = 0; i < ADC_OVERSAMPLE; i ++) {
    sample = adc_read_raw(ADC_INPUT_CHANNEL);
    total += sample;
  }

  return ((uint64_t)total) * muv_per_lsb * ADC_NUMERATOR / ADC_OVERSAMPLE / 1000 / ADC_DENUMERATOR;
}
