/* LCD driver for STM32L0/L1 LCDC */
/* NOTE: AN4654 shows STM32L0's LCDC is fully compatible with STM32L1's. */

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>

/* STM32L0's LCD driver isn't there yet... But it is identical to L1 */
#ifdef STM32L0
#define RCC_CSR_RTCSEL_HSI -1
#endif
#include <libopencm3/stm32/l1/lcd.h>


#include "disp.h"


#ifndef DISP_LCD_REFRESH_HZ
#define DISP_LCD_REFRESH_HZ 100
#endif
#ifndef DISP_LCD_CONTRAST
#define DISP_LCD_CONTRAST   LCD_FCR_CC_5
#endif


/* LCDC to GPIO mapping */
#define DISP_LCDC_GPIO_AF   GPIO_AF1
/* 8 commons */
/*                                               0      1      2       3      4       5       6       7 */
/*                                             PA8,   PA9,  PA10,    PB9,  PC10,   PC11,   PC12,    PD2 */
static const uint32_t lcdc_coms_bank[]    = {GPIOA, GPIOA, GPIOA,  GPIOB, GPIOC,  GPIOC,  GPIOC,  GPIOD};
static const uint16_t lcdc_coms_pin[]     = {GPIO8, GPIO9, GPIO10, GPIO9, GPIO10, GPIO11, GPIO12, GPIO2};
/* 28 segments + 4 additional multiplexed with commons */
/*                                               0      1      2      3      4      5      6      7      8      9     10      11      12      13      14      15      16     17      18     19     20     21     22     23     24     25     26     27     28      29      30      31 */
/*                                             PA1,   PA2,   PA3,   PA6,   PA7,   PB0,   PB1,   PB3,   PB4,   PB5,  PB10,   PB11,   PB12,   PB13,   PB14,   PB15,    PB8,  PA15,    PC0,   PC1,   PC2,   PC3,   PC4,   PC5,   PC6,   PC7,   PC8,   PC9,  PC10,   PC11,   PC12,    PD2 */
static const uint32_t lcdc_segs_bank[]    = {GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB,  GPIOB,  GPIOB,  GPIOB,  GPIOB,  GPIOB,  GPIOB, GPIOA,  GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC,  GPIOC,  GPIOC,  GPIOD};
static const uint16_t lcdc_segs_pin[]     = {GPIO1, GPIO2, GPIO3, GPIO6, GPIO7, GPIO0, GPIO1, GPIO3, GPIO4, GPIO5, GPIO10, GPIO11, GPIO12, GPIO13, GPIO14, GPIO15, GPIO8, GPIO15, GPIO0, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO9, GPIO10, GPIO11, GPIO12, GPIO2};

/* In the order of {D1[segments], D2[segments], D3[segments], D4[segments]}, segments in the order of {a, b, c, d, e, f, g, dp} */
static const uint8_t  digit_coms[DISP_LCD_NDIGITS * 8] = DISP_LCD_DIGIT_COMS;
static const uint8_t  digit_segs[DISP_LCD_NDIGITS * 8] = DISP_LCD_DIGIT_SEGS;

/* Hardware setup defined in board.mk */
static const uint8_t  lcdc_coms[DISP_LCD_NCOMS] = DISP_LCD_COMS;
static const uint8_t  lcdc_segs[DISP_LCD_NSEGS] = DISP_LCD_SEGS;

/* Use MMIO64 to read/write */
#define DISP_LCD_REG_RW(x)    MMIO64(LCD_RAM_BASE + (8 * (x)))

// TODO: move to disp-common.c
/* Segment font, MSB to LSB: dp, g, f, e, d, c, b, a */
#define DISP_FONT_DP          (1 <<  7)
#define DISP_FONT_E           (0x79)
#define DISP_FONT_BLANK       (0x00)
static const uint8_t  segment_font[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};


void disp_setup(void) {
  uint8_t i;

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_GPIOD);

  for (i = 0; i < DISP_LCD_NCOMS; i ++) {
    uint32_t bank = lcdc_coms_bank[lcdc_coms[i]];
    uint16_t pin  = lcdc_coms_pin [lcdc_coms[i]];
    gpio_set_af(bank, DISP_LCDC_GPIO_AF, pin);
    gpio_mode_setup(bank, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
  }

  for (i = 0; i < DISP_LCD_NSEGS; i ++) {
    uint32_t bank = lcdc_segs_bank[lcdc_segs[i]];
    uint16_t pin  = lcdc_segs_pin [lcdc_segs[i]];
    gpio_set_af(bank, DISP_LCDC_GPIO_AF, pin);
    gpio_mode_setup(bank, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
  }

  rcc_periph_clock_disable(RCC_GPIOA);
  rcc_periph_clock_disable(RCC_GPIOB);
  rcc_periph_clock_disable(RCC_GPIOC);
  rcc_periph_clock_disable(RCC_GPIOD);

  /* NOTE: majority of the LCDC is in the RTC domain */
  rcc_periph_clock_enable(RCC_PWR);
  pwr_disable_backup_domain_write_protect();

  /* LCDC gets clock from RTC. Use LSI clock */
  rcc_osc_on(RCC_LSI);
  rcc_wait_for_osc_ready(RCC_LSI);
  RCC_CSR |= RCC_CSR_RTCRST;
  RCC_CSR &= ~RCC_CSR_RTCRST;
  //RCC_CSR &= ~(RCC_CSR_RTCSEL_MASK << RCC_CSR_RTCSEL_SHIFT);
  RCC_CSR |= (RCC_CSR_RTCSEL_LSI << RCC_CSR_RTCSEL_SHIFT);
  RCC_CSR |= RCC_CSR_RTCEN;
  rtc_unlock();
  RTC_ISR |= RTC_ISR_INIT;
  while ((RTC_ISR & RTC_ISR_INITF) == 0);
  /* Async divider is 6-bit, LSI is 37kHz */
  rtc_set_prescaler((37000 >> 7), (1 << 7) - 1);
  RTC_ISR &= ~(RTC_ISR_INIT);
  rtc_lock();
  RCC_CSR |= RCC_CSR_RTCEN;
  rtc_wait_for_synchro();

  rcc_periph_clock_enable(RCC_LCD);
  lcd_set_duty(LCD_CR_DUTY_1_4);
  lcd_set_bias(LCD_CR_BIAS_1_3);
  lcd_set_refresh_frequency(DISP_LCD_REFRESH_HZ);
  lcd_set_contrast(DISP_LCD_CONTRAST);
  /* NOTE: charge pump is enabled by default */
  lcd_enable();
  lcd_wait_for_lcd_enabled();
  lcd_wait_for_step_up_ready();
  /* LCD_FCR is in RTC domain and requires synchronization */
  while ((LCD_SR & LCD_SR_FCRSF) == 0);

  /* NOTE: leave RTC domain write-protect disabled, otherwise cannot update LCD! */
  rcc_periph_clock_disable(RCC_PWR);
}

void disp_update(uint32_t voltage_mv, bool blinker) {
  uint8_t i, j;
  // TODO: generalize and move first part to disp-common.c
  uint8_t digit[] = {DISP_FONT_BLANK, DISP_FONT_BLANK, DISP_FONT_BLANK, DISP_FONT_BLANK};
  // TODO: generalize
  uint16_t lcd_ram[] = {0, 0, 0, 0};

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

  digit[3] |= blinker ? DISP_FONT_DP : DISP_FONT_BLANK;

  for (i = 0; i < 4; i ++) {
    for (j = 0; j < 8; j ++) {
      uint8_t element = i * 8 + j;
      uint8_t glass_com = digit_coms[element];
      uint8_t glass_seg = digit_segs[element];
      uint8_t lcdc_com = lcdc_coms[glass_com];
      uint8_t lcdc_seg = lcdc_segs[glass_seg];
      /* Font segment -> LCD segment -> GPIO */
      lcd_ram[lcdc_com] |= (((digit[i] >> j) & 1) << lcdc_seg);
    }
  }

  // TODO: generalize
  LCD_RAM_COM0 = lcd_ram[0];
  LCD_RAM_COM1 = lcd_ram[1];
  LCD_RAM_COM2 = lcd_ram[2];
  LCD_RAM_COM3 = lcd_ram[3];
  lcd_update();
  lcd_wait_for_update_ready();
}
