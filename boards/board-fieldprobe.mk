LDSCRIPT = libopencm3/lib/stm32/l0/stm32l0xx6.ld

# DISP_LCD_COMS / DISP_LCD_SEGS: mapping between LCD's COM/SEG to STM32 LCDC's COM/SEG.
# COM in 0-to-3 (PIN1-to-PIN4) order. SEG in PIN6-to-PIN12 order.
CFLAGS  += -DDISP_LCD_COMS='{2, 1, 0, 3}' -DDISP_LCD_SEGS='{15, 14, 13, 12, 11, 10, 6, 5}'
CFLAGS  += -DADC_INPUT_BANK=GPIOA -DADC_INPUT_PIN=GPIO0 -DADC_INPUT_CHANNEL=0 \
           -DADC_RDIV_UPPER=2000 -DADC_RDIV_LOWER=100


include board-stm32l0.mk
include disp-lcdglass-s401m16kr.mk
include disp-stlcdc.mk

flash: flash-stlink
