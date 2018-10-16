#include <libopencm3/cm3/assert.h>

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

#ifdef ADC_OVERSAMPLE
#if ADC_OVERSAMPLE != 16
#error "ADC_OVERSAMPLE has to be 16!"
#endif
#endif

#ifndef ADC_RDIV_LOWER
/* ADC_RDIV_LOWER is inf */
#define ADC_RDIV_UPPER      0
#define ADC_RDIV_LOWER      1
#endif
#define ADC_NUMERATOR       (ADC_RDIV_UPPER + ADC_RDIV_LOWER)
#define ADC_DENUMERATOR     (ADC_RDIV_LOWER)


/* BEGIN: Missing bits */
#define PWR_CSR_VREFINTRDY      (1 << 3)

#define ADC_CCR_LFMEN           (1 << 25)

#define ADC_CFGR2_OVSS_SHIFT    5
#define ADC_CFGR2_OVSS          (15 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_NOSHIFT  (0 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_SHIFT_1  (1 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_SHIFT_2  (2 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_SHIFT_3  (3 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_SHIFT_4  (4 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_SHIFT_5  (5 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_SHIFT_6  (6 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_SHIFT_7  (7 << ADC_CFGR2_OVSS_SHIFT)
#define ADC_CFGR2_OVSS_SHIFT_8  (8 << ADC_CFGR2_OVSS_SHIFT)

#define ADC_CFGR2_OVSR_SHIFT    2
#define ADC_CFGR2_OVSR          (7 << ADC_CFGR2_OVSR_SHIFT)
#define ADC_CFGR2_OVSR_2X       (1 << ADC_CFGR2_OVSR_SHIFT)
#define ADC_CFGR2_OVSR_4X       (2 << ADC_CFGR2_OVSR_SHIFT)
#define ADC_CFGR2_OVSR_8X       (3 << ADC_CFGR2_OVSR_SHIFT)
#define ADC_CFGR2_OVSR_16X      (4 << ADC_CFGR2_OVSR_SHIFT)
#define ADC_CFGR2_OVSR_32X      (5 << ADC_CFGR2_OVSR_SHIFT)
#define ADC_CFGR2_OVSR_64X      (6 << ADC_CFGR2_OVSR_SHIFT)
#define ADC_CFGR2_OVSR_128X     (7 << ADC_CFGR2_OVSR_SHIFT)
#define ADC_CFGR2_OVSR_256X     (8 << ADC_CFGR2_OVSR_SHIFT)

#define ADC_CFGR2_TOVS          (1 << 9)
#define ADC_CFGR2_OVSE          (1 << 0)

#define ADC_SMPR1_SMP           7

#define ADC_CLKSOURCE_ADC       ADC_CFGR2_CKMODE_CK_ADC
#define ADC_CLKSOURCE_PCLK      ADC_CFGR2_CKMODE_PCLK
#define ADC_CLKSOURCE_PCLK_DIV2 ADC_CFGR2_CKMODE_PCLK_DIV2
#define ADC_CLKSOURCE_PCLK_DIV4 ADC_CFGR2_CKMODE_PCLK_DIV4

#define ADC_RESOLUTION_12BIT    ADC_CFGR1_RES_12_BIT
#define ADC_RESOLUTION_10BIT    ADC_CFGR1_RES_10_BIT
#define ADC_RESOLUTION_8BIT     ADC_CFGR1_RES_8_BIT
#define ADC_RESOLUTION_6BIT     ADC_CFGR1_RES_6_BIT


static void adc_set_clk_source(uint32_t adc, uint32_t source) {
  ADC_CFGR2(adc) = ((ADC_CFGR2(adc) & ~ADC_CFGR2_CKMODE) | source);
}

void adc_set_sample_time_on_all_channels(uint32_t adc, uint8_t time) {
  ADC_SMPR1(adc) = time & ADC_SMPR1_SMP;
}

void adc_set_regular_sequence(uint32_t adc, uint8_t length, uint8_t channel[]) {
  uint32_t reg32 = 0;
  uint8_t i = 0;
  bool stepup = false, stepdn = false;

  if (length == 0) {
    ADC_CHSELR(adc) = 0;
    return;
  }

  reg32 |= (1 << channel[0]);

  for (i = 1; i < length; i++) {
    reg32 |= (1 << channel[i]);
    stepup |= channel[i - 1] < channel[i];
    stepdn |= channel[i - 1] > channel[i];
  }

  /* Check, if the channel list is in order */
  if (stepup && stepdn) {
    cm3_assert_not_reached();
  }

  /* Update the scan direction flag */
  if (stepdn) {
    ADC_CFGR1(adc) |= ADC_CFGR1_SCANDIR;
  } else {
    ADC_CFGR1(adc) &= ~ADC_CFGR1_SCANDIR;
  }

  ADC_CHSELR(adc) = reg32;
}

/* END: Missing bits */


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
  //adc_disable_temperature_sensor();

  adc_power_off(ADC1);
  ADC_CCR(ADC1) |= ADC_CCR_LFMEN; /* Mandatory for fADC < 3.5MHz */
  adc_set_clk_source(ADC1, ADC_CLKSOURCE_PCLK_DIV4); /* Set ADC clock to ~15 kHz */
  adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_160DOT5CYC); /* (160.5 + 1.5 ~= 100 sps) */

  /* Setup 16X HW oversample */
  ADC_CFGR2(ADC1) = (ADC_CFGR2(ADC1) & (~ADC_CFGR2_OVSS) & (~ADC_CFGR2_OVSR) & (~ADC_CFGR2_TOVS)) | ADC_CFGR2_OVSS_NOSHIFT | ADC_CFGR2_OVSR_16X | ADC_CFGR2_OVSE;

  adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);
  adc_set_right_aligned(ADC1);
  adc_set_single_conversion_mode(ADC1);
  //ADC_CFGR1(ADC1) |= ADC_CFGR1_AUTDLY; /* Do not start conversion until DR is read. Avoids race conditions. */
  //adc_disable_analog_watchdog(ADC1);
  //adc_disable_external_trigger_regular(ADC1);

  adc_calibrate(ADC1);
  adc_power_on(ADC1);

  /* Wait for ADC to start up. */
  tick_delay(100);

  /* Wait for internal reference to start up. */
  while (!(PWR_CSR & PWR_CSR_VREFINTRDY));
}

uint32_t adc_read(void) {
  uint16_t  muv_per_lsb;
  uint32_t  sample, vref_muv, vref_lsbs;

  /* Establish LSB to uv mapping */
  vref_muv = ST_VREFINT_CAL * ADC_OVERSAMPLE;
  vref_lsbs = adc_read_raw(ADC_CHANNEL_VREF);
  /* VDDA = 3.0 V (3,000,000 uV) x VREFINT_CAL / VREFINT_DATA; uV/LSB = VDDA / (2 ^ #BITS) */
  muv_per_lsb = ((uint64_t)vref_muv) * 3000000 / (1 << 12) / vref_lsbs;

  sample = adc_read_raw(ADC_INPUT_CHANNEL);
  return ((uint64_t)sample) * muv_per_lsb * ADC_NUMERATOR / ADC_OVERSAMPLE / 1000 / ADC_DENUMERATOR;
}
