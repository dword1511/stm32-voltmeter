/* Generic 7-segment 4-digit LED display driver. Takes a whole 16-line GPIO bank. */
/* COM0-7 are MSD to LSD. SEG 7 is DP, 6-0 is a to g. */


#include <stdbool.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>


#include "disp.h"


/*
#define DISP_GPIO_BANK  GPIOA
#define DISP_PIN_COM0   9
#define DISP_PIN_COM1   10
#define DISP_PIN_COM2   13
#define DISP_PIN_COM3   14
#define DISP_COM_ANODE
#define DISP_DUTY       100 // Additional dimming
*/


#define DISP_COM_0      (1 << DISP_PIN_COM0)
#define DISP_COM_1      (1 << DISP_PIN_COM1)
#define DISP_COM_2      (1 << DISP_PIN_COM2)
#define DISP_COM_3      (1 << DISP_PIN_COM3)
#define DISP_CATHODE(c) ((~c) & 0xff00)

#define DISP_FONT_DP    (1 <<  7)
#define DISP_FONT_E     (0x79)
#define DISP_FONT_BLANK (0x00)

#if (DISP_DUTY > 100) || (DISP_DUTY < 0)
#error "DISP_DUTY shoud be in [0, 100]"
#endif
#ifndef DISP_DUTY
#define DISP_DUTY       100
#endif
#define DISP_EXTRAS     (4 * (100 - DISP_DUTY) / DISP_DUTY)

#define DISP_GPIO_ALL   (DISP_COM_0 | DISP_COM_1 | DISP_COM_2 | DISP_COM_3 | 0x00ff)


static const    uint8_t   segment_font[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f}; /* LED font 0-9, active high */
static const    uint16_t  coms[4]          = {
#ifdef DISP_COM_ANODE
  DISP_COM_0, DISP_COM_1, DISP_COM_2, DISP_COM_3,
#else
  DISP_CATHODE(DISP_COM_0), DISP_CATHODE(DISP_COM_1), DISP_CATHODE(DISP_COM_2), DISP_CATHODE(DISP_COM_3),
#endif
};
static volatile uint8_t   digit[4]         = {DISP_FONT_BLANK, DISP_FONT_BLANK, DISP_FONT_BLANK, DISP_FONT_BLANK}; /* Rendered */
static volatile uint8_t   current_digit    = 0;


void disp_setup(void) {
  gpio_port_write(DISP_GPIO_BANK, DISP_FONT_BLANK);
  gpio_set_output_options(DISP_GPIO_BANK, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, DISP_GPIO_ALL);
  gpio_mode_setup(DISP_GPIO_BANK, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DISP_GPIO_ALL);
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

void disp_refresh(void) {
  current_digit ++;
  if (current_digit > (3 + DISP_EXTRAS)) {
    current_digit = 0;
  }
  if (current_digit < 4) {
#ifdef DISP_COM_ANODE
    gpio_port_write(DISP_GPIO_BANK, ((~digit[current_digit]) & 0x00ff) | coms[current_digit]);
#else /* DISP_COM_ANODE */
    gpio_port_write(DISP_GPIO_BANK, digit[current_digit] | coms[current_digit]);
#endif /* DISP_COM_ANODE */
  } else {
    gpio_port_write(DISP_GPIO_BANK, DISP_FONT_BLANK);
  }
}
