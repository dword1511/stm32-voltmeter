# TODO: extend to multiple digit
OBJS    += disp/gpioled.o

# Tell tick system that display needs refresh
# 400HZ tick = 100HZ refresh rate (as we have 4 digits)
# This requires at least ~40kHz CPU clock
# NOTE: lower duty requires higher tick freq
CFLAGS  += -DDISP_NEEDS_REFRESH -DTICK_HZ=400 -DDISP_DUTY=80
