#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>

#include "disp.h"
#include "tick.h"
#include "power.h"


/*
 * Quick proto (later get LP version w/ STM32L0 LCD line)
 *
 * Pin assignment:
 * PA0-7:                 segments
 * PA9, PA10, PA13, PA14: commons
 * PB1:                   voltage in (ADC_IN9)
 * PB8:                   range
 *
 * Power consumption:
 * 1.0 mA for CPU
 * ~1.5 mA for each LED (12 mA total)
 * 5 mA for LDO (wut?!)
 * 6F22 ~ 400mAh (~ 20 hrs)
 */



/* BEGIN ADC */

#define ADC_RANGE_BANK      (GPIOB)
#define ADC_RANGE_PIN       (GPIO8)
#define ADC_INPUT_BANK      (GPIOB)
#define ADC_INPUT_PIN       (GPIO1)
#define ADC_INPUT_CHANNEL   (9)

#define ADC_OVERSAMPLE      (16)
#define ADC_RANGE1_MAX      ((1 << 12) - (1 << 8))  /* Avoid top 5% */

#define ADC_TO_MV_RANGE1(x) ((x))                   /* PA13 open  */
#define ADC_TO_MV_RANGE2(x) ((x))                   /* PA13 gnd   */

#define PWR_CSR_VREFINTRDY  (1 << 3)

static volatile uint16_t muv_per_lsb;
static volatile uint32_t voltage_mv;

static uint16_t adc_read_raw(uint8_t ch);

static void adc_setup(void) {
  uint16_t  i;
  uint32_t  vref_muv, vref_lsbs = 0;

  gpio_mode_setup(ADC_INPUT_BANK, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_INPUT_PIN);

  rcc_periph_clock_enable(RCC_ADC1);
  adc_enable_vrefint();
  adc_disable_temperature_sensor();

  adc_power_off(ADC1);
  adc_set_clk_source(ADC1, ADC_CLKSOURCE_PCLK_DIV4); /* Set ADC clock to 500 kHz */
  adc_calibrate(ADC1);
  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_239DOT5); /* (239.5 + 1.5 ~= 2.1 ksps) */

  adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);
  adc_set_right_aligned(ADC1);
  adc_set_single_conversion_mode(ADC1);
  ADC_CFGR1(ADC1) |= ADC_CFGR1_AUTDLY; /* Do not start conversion until DR is read. Avoids race conditions. */
  adc_disable_analog_watchdog(ADC1);
  adc_disable_external_trigger_regular(ADC1);
  adc_power_on(ADC1);

  /* Wait for ADC starting up. */
  for (i = 0; i < 40000; i ++) {
    asm("nop");
  }

  /* Establish LSB to uv mapping */
  while (!(PWR_CSR & PWR_CSR_VREFINTRDY));
  vref_muv = ST_VREFINT_CAL;
  vref_muv *= 1000 * ADC_OVERSAMPLE;
  for (i = 0; i < ADC_OVERSAMPLE; i ++) {
    vref_lsbs += adc_read_raw(ADC_CHANNEL_VREF);
  }
  muv_per_lsb = vref_muv / vref_lsbs;
}

static void adc_set_range1(void) {
  gpio_mode_setup(ADC_RANGE_BANK, GPIO_MODE_INPUT, GPIO_PUPD_NONE, ADC_RANGE_PIN);
}

static void adc_set_range2(void) {
  gpio_clear(ADC_RANGE_BANK, ADC_RANGE_PIN);
  gpio_set_output_options(ADC_RANGE_BANK, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, ADC_RANGE_PIN);
  gpio_mode_setup(ADC_RANGE_BANK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ADC_RANGE_PIN);
}

static uint16_t adc_read_raw(uint8_t ch) {
  adc_set_regular_sequence(ADC1, 1, &ch);
  adc_start_conversion_regular(ADC1);
  return adc_read_regular(ADC1);
}

static void adc_read(void) {
  uint8_t   i;
  uint16_t  sample;
  uint32_t  total;
  bool      need_range2 = false;

  adc_set_range1();
  total = 0;
  for (i = 0; i < ADC_OVERSAMPLE; i ++) {
    sample = adc_read_raw(ADC_INPUT_CHANNEL);
    if (sample > ADC_RANGE1_MAX) {
      need_range2 = true;
      break;
    }
    total += sample;
  }

  if (need_range2) {
    adc_set_range2();
    total = 0;
    for (i = 0; i < ADC_OVERSAMPLE; i ++) {
      total += adc_read_raw(ADC_INPUT_CHANNEL);
    }
    voltage_mv = ADC_TO_MV_RANGE2(total / ADC_OVERSAMPLE);
  } else {
    voltage_mv = ADC_TO_MV_RANGE1(total / ADC_OVERSAMPLE);
  }
}

/* END ADC */


static void post_delay(void) {
  static uint32_t i;

  for (i = 0; i < 50000; i ++) {
    asm("nop");
  }
}

static void __attribute__((unused)) disp_test(void) {
  while (true) {
    voltage_mv = 01;
    post_delay();
    voltage_mv = 23;
    post_delay();
    voltage_mv = 45;
    post_delay();
    voltage_mv = 67;
    post_delay();
    voltage_mv = 89;
    post_delay();
  }
}

int main(void) {
  power_setup();
  disp_setup();
  tick_setup();
  adc_setup();

  /* Power on display test */
  voltage_mv =  8888;
  post_delay();
  voltage_mv = 88888;
  post_delay();
  voltage_mv =     0;

  //disp_test();

  /* Main loop */
  while (!tick_auto_poweroff()) {
    adc_read();
    post_delay();
  }

  power_off();
  return 0;
}

