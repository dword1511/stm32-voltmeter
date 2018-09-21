#include <stdbool.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>


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

#define SYSCLK_FREQ       (2000000)
#define PWR_TIME_OUT_SECS (20)


/* BEGIN SYSTICK */

#define SYSCLK_PERIOD_MS  (2)
#define SYSCLK_PERIOD     ((SYSCLK_FREQ) * (SYSCLK_PERIOD_MS) / 1000 - 1)

static volatile uint32_t  uptime_ms = 0;

static void disp_update(void);

/* Use systick interrupt to update display */
static void systick_setup(void) {
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_clear();

  systick_set_reload(SYSCLK_PERIOD);
  systick_interrupt_enable();
  systick_counter_enable();
}

void sys_tick_handler(void) {
  uptime_ms += SYSCLK_PERIOD_MS;
  disp_update();
}


/* END SYSTICK */

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

/* BEGIN DISP */

#define DISP_COM_0        (1 <<  9)
#define DISP_COM_1        (1 << 10)
#define DISP_COM_2        (1 << 13)
#define DISP_COM_3        (1 << 14)

#define DISP_FONT_DP      (1 <<  7)
#define DISP_FONT_E       (0x79)
#define DISP_FONT_BLANK   (0x00)

//#define DISP_COM_ANODE

static const    uint8_t   segment_font[10]  = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f}; /* 0-9, active high */
static volatile uint8_t   blinker           = 0;

static void disp_setup(void) {
  gpio_port_write(GPIOA, DISP_FONT_BLANK);
  gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, GPIO_ALL);
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_ALL);
}

static void disp_delay(void) {
  static uint32_t i;

  for (i = 0; i < 100; i ++) {
    asm("nop");
  }
}

static void disp_update(void) {
  static uint8_t digit[4] = {0, 0, 0, 0};

  /* Rounding */
  if ((voltage_mv > 9999) && ((voltage_mv % 10) > 4)) {
    voltage_mv ++;
  }

  while (true) {
    if (voltage_mv > 99999) {
      digit[0] = DISP_FONT_E;
      digit[1] = DISP_FONT_BLANK;
      digit[2] = DISP_FONT_BLANK;
      digit[3] = DISP_FONT_BLANK;
      break;
    }
    if (voltage_mv > 9999) {
      /* AB.CD */
      digit[0] = segment_font[(voltage_mv / 10000) % 10];
      digit[1] = segment_font[(voltage_mv /  1000) % 10] | DISP_FONT_DP;
      digit[2] = segment_font[(voltage_mv /   100) % 10];
      digit[3] = segment_font[(voltage_mv /    10) % 10];
      break;
    }
    /* Everything else */
    {
      /* A.BCD */
      digit[0] = segment_font[(voltage_mv /  1000) % 10] | DISP_FONT_DP;
      digit[1] = segment_font[(voltage_mv /   100) % 10];
      digit[2] = segment_font[(voltage_mv /    10) % 10];
      digit[3] = segment_font[(voltage_mv /     1) % 10];
      break;
    }
  }

  blinker ++;
  digit[3] |= ((blinker > 200) ? DISP_FONT_DP : DISP_FONT_BLANK);

#ifdef DISP_COM_ANODE
  gpio_port_write(GPIOA, ((~digit[0]) & 0x00ff) | DISP_COM_0);
  disp_delay();
  gpio_port_write(GPIOA, ((~digit[1]) & 0x00ff) | DISP_COM_1);
  disp_delay();
  gpio_port_write(GPIOA, ((~digit[2]) & 0x00ff) | DISP_COM_2);
  disp_delay();
  gpio_port_write(GPIOA, ((~digit[3]) & 0x00ff) | DISP_COM_3);
  disp_delay();
#else
  gpio_port_write(GPIOA, digit[0] | ((~DISP_COM_0) & 0xff00));
  disp_delay();
  gpio_port_write(GPIOA, digit[1] | ((~DISP_COM_1) & 0xff00));
  disp_delay();
  gpio_port_write(GPIOA, digit[2] | ((~DISP_COM_2) & 0xff00));
  disp_delay();
  gpio_port_write(GPIOA, digit[3] | ((~DISP_COM_3) & 0xff00));
  disp_delay();
#endif
  gpio_port_write(GPIOA, DISP_FONT_BLANK);
}

/* END DISP */

/* BEGIN PWR */

static void pwr_setup(void) {
  /* Setup clock to conserve power (no use... LD1117 is so hungry) */
  rcc_osc_on(RCC_HSI);
  rcc_wait_for_osc_ready(RCC_HSI);
  rcc_set_sysclk_source(RCC_HSI);

  rcc_set_hpre(RCC_CFGR_HPRE_DIV4);
  rcc_set_ppre(RCC_CFGR_PPRE_NODIV);

  flash_set_ws(FLASH_ACR_LATENCY_000_024MHZ);

  rcc_apb1_frequency = SYSCLK_FREQ;
  rcc_ahb_frequency  = SYSCLK_FREQ;

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
}

static void pwr_turn_off(void) {
  pwr_disable_power_voltage_detect();

  /* According to ST App note... */
  SCB_SCR |= SCB_SCR_SLEEPDEEP;
  /* Only sets PDDS... Well, others are universal to CM3 anyway. */
  pwr_set_standby_mode();
  pwr_clear_wakeup_flag();
  /* Enter standby */
  asm("wfi");
}

/* END PWR */

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
  pwr_setup();
  disp_setup();
  systick_setup();
  adc_setup();

  /* Power on display test */
  voltage_mv =  8888;
  post_delay();
  voltage_mv = 88888;
  post_delay();
  voltage_mv =     0;

  //disp_test();

  /* Main loop */
  while (uptime_ms < (PWR_TIME_OUT_SECS * 1000)) {
    adc_read();
    post_delay();
  }

  pwr_turn_off();
  return 0;
}

void nmi_handler(void)
__attribute__ ((alias ("pwr_turn_off")));

void hard_fault_handler(void)
__attribute__ ((alias ("pwr_turn_off")));
