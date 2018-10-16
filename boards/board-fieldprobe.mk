LDSCRIPT = libopencm3/lib/stm32/l0/stm32l0xx6.ld

# COM in 0-to-3 (PIN1-to-PIN4) order. SEG in PIN6-to-PIN12 order.
CFLAGS  += -DDISP_LCD_COMS='{2, 1, 0, 3}' -DDISP_LCD_SEGS='{15, 14, 13, 12, 11, 10, 6, 5}' \
           -DADC_INPUT_BANK=GPIOA -DADC_INPUT_PIN=GPIO0 -DADC_INPUT_CHANNEL=0


include board-stm32l0.mk
include disp-l0lcd-s401m16kr.mk

flash: flash-stlink
