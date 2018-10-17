/* Code snippets for ADC (dependencies not included) */

#ifndef __VOLTMETER_ANALOG_COMMON_H__
#define __VOLTMETER_ANALOG_COMMON_H__

#ifndef ANALOG_RDIV_LOWER
/* ANALOG_RDIV_LOWER is inf */
#define ANALOG_RDIV_UPPER   0
#define ANALOG_RDIV_LOWER   1
#endif
#define ANALOG_NUMERATOR    (ANALOG_RDIV_UPPER + ANALOG_RDIV_LOWER)
#define ANALOG_DENUMERATOR  (ANALOG_RDIV_LOWER)

#ifndef ANALOG_OVERSAMPLE
#define ANALOG_OVERSAMPLE   1
#endif


#if (defined(ANALOG_GUARDPIN1_BANK) && defined(ANALOG_GUARDPIN1_PIN)) || (defined(ANALOG_GUARDPIN2_BANK) && defined(ANALOG_GUARDPIN2_PIN))
static void __analog_setup_guard(uint32_t gpioport, uint32_t gpiopin) {
  gpio_clear(gpioport, gpiopin);
  gpio_set_output_options(gpioport, GPIO_OTYPE_OD, GPIO_OSPEED_LOW, gpiopin);
  gpio_mode_setup(gpioport, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, gpiopin);
}
#endif /* ANALOG_GUARDPIN*_* */

static void __analog_setup_gpio(void) {
  /* ADC inputs are spread over GPIOA, GPIOB, and GPIOC (mostly GPIOA) */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);

  gpio_mode_setup(ANALOG_INPUT_BANK, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ANALOG_INPUT_PIN);

  /* Grounded guard pins (at most have 2) */
#if defined(ANALOG_GUARDPIN1_BANK) && defined(ANALOG_GUARDPIN1_PIN)
  __analog_setup_guard(ANALOG_GUARDPIN1_BANK, ANALOG_GUARDPIN1_PIN);
#endif
#if defined(ANALOG_GUARDPIN2_BANK) && defined(ANALOG_GUARDPIN2_PIN)
  __analog_setup_guard(ANALOG_GUARDPIN2_BANK, ANALOG_GUARDPIN2_PIN);
#endif

  rcc_periph_clock_disable(RCC_GPIOA);
  rcc_periph_clock_disable(RCC_GPIOB);
  rcc_periph_clock_disable(RCC_GPIOC);
}

static uint16_t __analog_read_raw(uint8_t ch) {
  adc_set_regular_sequence(ADC1, 1, &ch);
  adc_start_conversion_regular(ADC1);
  while (!adc_eoc(ADC1)) {
    /* According to AN2834, we should avoid such change */
    //asm("wfi"); /* Wait for one tick */
    asm("nop");
  }
  return adc_read_regular(ADC1);
}

/* Assume 12-bit mode. */
static uint16_t __analog_get_muv_per_lsb(uint32_t vref_lsbs) {
  uint64_t vref_muv = ST_VREFINT_CAL * ANALOG_OVERSAMPLE;
  /* VDDA = VCAL (e.g. 3,000,000 uV, find in DS/manual) x VREFINT_CAL / VREFINT_DATA; uV/LSB = VDDA / (2 ^ #BITS) */
  return vref_muv * ANALOG_VCAL_MUV / (1 << 12) / vref_lsbs;
}

static uint32_t __analog_get_mv(uint32_t vref_lsbs, uint32_t vin_lsbs) {
  return ((uint64_t)vin_lsbs) * __analog_get_muv_per_lsb(vref_lsbs) * ANALOG_NUMERATOR / ANALOG_OVERSAMPLE / 1000 / ANALOG_DENUMERATOR;
}

#endif /* __VOLTMETER_ANALOG_COMMON_H__ */
