/* 4-Mux 7-Segment LCD driver for STM32L0 LCDC and S401M16KR LCD glass. */
/* NOTE: AN4654 shows STM32L0's LCDC is fully compatible with STM32L1's. */

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>


#include "disp.h"


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
static const uint32_t lcdc_segs_port[]    = {GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB,  GPIOB,  GPIOB,  GPIOB,  GPIOB,  GPIOB,  GPIOB, GPIOA,  GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC,  GPIOC,  GPIOC,  GPIOD};
static const uint16_t lcdc_segs_pin[]     = {GPIO1, GPIO2, GPIO3, GPIO6, GPIO7, GPIO0, GPIO1, GPIO3, GPIO4, GPIO5, GPIO10, GPIO11, GPIO12, GPIO13, GPIO14, GPIO15, GPIO8, GPIO15, GPIO0, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO9, GPIO10, GPIO11, GPIO12, GPIO2};

/* S401M16KR element map, treating COL as 4-DP */
/* SEGS:  0   1    2   3    4   5    6   7  */
/* COM0: 1d, 1dp, 2d, 2dp, 3d, 3dp, 4d, 4dp */
/* COM1: 1e, 1c , 2e, 2c , 3e, 3c , 4e, 4c  */
/* COM2: 1g, 1b , 2g, 2b , 3g, 3b , 4g, 4b  */
/* COM3: 1f, 1a , 2f, 2a , 3f, 3a , 4f, 4a  */
/*                                          dp, a, b, c, d, e, f, g */
static const uint8_t  s401m16kr_d0_coms[] = {0, 3, 2, 1, 0, 1, 3, 2};
static const uint8_t  s401m16kr_d0_segs[] = {1, 1, 1, 1, 0, 0, 0, 0};
static const uint8_t  s401m16kr_d1_coms[] = {0, 3, 2, 1, 0, 1, 3, 2};
static const uint8_t  s401m16kr_d1_segs[] = {3, 3, 3, 3, 2, 2, 2, 2};
static const uint8_t  s401m16kr_d2_coms[] = {0, 3, 2, 1, 0, 1, 3, 2};
static const uint8_t  s401m16kr_d2_segs[] = {5, 5, 5, 5, 4, 4, 4, 4};
static const uint8_t  s401m16kr_d3_coms[] = {0, 3, 2, 1, 0, 1, 3, 2};
static const uint8_t  s401m16kr_d3_segs[] = {7, 7, 7, 7, 6, 6, 6, 6};

/* Hardware setup defined in board.mk */
#define DISP_S401M16KR_NCOMS  4
#define DISP_S401M16KR_NSEGS  8
static const uint8_t  s401m16kr_coms[DISP_S401M16KR_NCOMS] = DISP_LCD_COMS;
static const uint8_t  s401m16kr_segs[DISP_S401M16KR_NSEGS] = DISP_LCD_SEGS;

/* Segment font, MSB to LSB: dp, a, b, c, d, e, f, g */
#define DISP_FONT_DP          (1 <<  7)
#define DISP_FONT_E           (0x79)
#define DISP_FONT_BLANK       (0x00)
static const uint8_t  segment_font[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};

/* Relevant LCD configurations from STM32 manual:     */
/* LCDCLK     PS[3:0]   DIV[3:0] Ratio Duty   f_frame */
/* 32.768kHz        4          1   272  1/4  30.12 Hz */
/* 32.768kHz        2          4    80  1/4 102.40 Hz */

void disp_setup(void) {
  uint8_t i;

  for (i = 0; i < DISP_S401M16KR_NCOMS; i ++) {
    uint32_t bank = lcdc_coms_bank[s401m16kr_coms[i]];
    uint16_t pin  = lcdc_coms_pin [s401m16kr_coms[i]];
    gpio_set_af(bank, DISP_LCDC_GPIO_AF, pin);
    gpio_mode_setup(bank, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
  }

  for (i = 0; i < DISP_S401M16KR_NSEGS; i ++) {
    uint32_t bank = lcdc_segs_bank[s401m16kr_segs[i]];
    uint16_t pin  = lcdc_segs_pin [s401m16kr_segs[i]];
    gpio_set_af(bank, DISP_LCDC_GPIO_AF, pin);
    gpio_mode_setup(bank, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
  }

  rcc_periph_clock_enable(RCC_LCD);
}

void disp_update(uint32_t voltage_mv, bool blinker) {
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
}
