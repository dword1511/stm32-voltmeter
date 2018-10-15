# General setup for STM32L0

ARCH_FLAGS  = -DSTM32L0 -mthumb -mcpu=cortex-m0plus -msoft-float
OPENCM3_MK  = lib/stm32/l0
LIBOPENCM3  = libopencm3/lib/libopencm3_stm32l0.a

OBJS       += power-stm32l0.o analog-stm32l0.o
