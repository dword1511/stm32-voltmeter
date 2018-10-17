# STM32F0 (any package >= TSSOP-20), 7-segment 4-digit LED display, common cathode, using GPIOA.
# e.g. STM32F0-NUCLEO paired with QDSP-6064

# Using minimum memory

LDSCRIPT = libopencm3/lib/stm32/f0/stm32f03xz6.ld

CFLAGS  += -DDISP_GPIO_BANK=GPIOA -DDISP_PIN_COM0=9 -DDISP_PIN_COM1=10 -DDISP_PIN_COM2=11 -DDISP_PIN_COM3=12 \
           -DADC_INPUT_BANK=GPIOB -DADC_INPUT_PIN=GPIO1 -DADC_INPUT_CHANNEL=9

CFLAGS  += -DTICK_PWROFF_SECS=20


include board-stm32f0.mk
include disp-led4d.mk

flash: flash-stlink
