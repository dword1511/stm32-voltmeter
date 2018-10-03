# STM32F0 (any package >= TSSOP-20), 7-segment 4-digit LED display, common cathode, using GPIOA.
# e.g. STM32F0-NUCLEO paired with QDSP-6064

LDSCRIPT = libopencm3/lib/stm32/f0/stm32f04xz6.ld

CFLAGS  += -DDISP_GPIO_BANK=GPIOA -DDISP_PIN_COM0=9 -DDISP_PIN_COM1=10 -DDISP_PIN_COM2=13 -DDISP_PIN_COM3=14
OBJS    += disp-led4d.o

# Tell tick system that display needs refresh
CFLAGS  += -DDISP_NEEDS_REFRESH -DTICK_HZ=200

include board-stm32f0.mk
