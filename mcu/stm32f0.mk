# General setup for STM32F0

ARCH_FLAGS  = -DSTM32F0 -mthumb -mcpu=cortex-m0 -msoft-float
OPENCM3_MK  = lib/stm32/f0
LIBOPENCM3  = libopencm3/lib/libopencm3_stm32f0.a

OBJS       += power-stm32f0.o analog-stm32f0.o

CFLAGS     += -DANALOG_OVERSAMPLE=16 -DANALOG_VCAL_MUV=3300000
