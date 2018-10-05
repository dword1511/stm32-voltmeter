OBJS    += disp-led4d.o

# Tell tick system that display needs refresh
# 400HZ tick = 100HZ refresh rate (as we have 4 digits)
CFLAGS  += -DDISP_NEEDS_REFRESH -DTICK_HZ=400
