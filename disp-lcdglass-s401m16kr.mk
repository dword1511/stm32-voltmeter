# Board-independent LCD glass data for LUMEX LCD-S401M16KR

# 4-mux, 4-digit, 8-segment
CFLAGS  += -DDISP_LCD_NCOMS=4 -DDISP_LCD_NDIGITS=4 -DDISP_LCD_NSEGS=8

# Element map, treating COL as 4-DP:
# SEGS: 0   1    2   3    4   5    6   7
# COM0: 1d, 1dp, 2d, 2dp, 3d, 3dp, 4d, 4dp
# COM1: 1e, 1c , 2e, 2c , 3e, 3c , 4e, 4c
# COM2: 1g, 1b , 2g, 2b , 3g, 3b , 4g, 4b
# COM3: 1f, 1a , 2f, 2a , 3f, 3a , 4f, 4a
# In the order of {D1[segments], D2[segments], D3[segments], D4[segments]}, segments in the order of {a, b, c, d, e, f, g, dp}
CFLAGS  += -DDISP_LCD_DIGIT_COMS='{3, 2, 1, 0, 1, 3, 2, 0, 3, 2, 1, 0, 1, 3, 2, 0, 3, 2, 1, 0, 1, 3, 2, 0, 3, 2, 1, 0, 1, 3, 2, 0}' \
           -DDISP_LCD_DIGIT_SEGS='{1, 1, 1, 0, 0, 0, 0, 1, 3, 3, 3, 2, 2, 2, 2, 3, 5, 5, 5, 4, 4, 4, 4, 5, 7, 7, 7, 6, 6, 6, 6, 7}'

# Low-power setting produces enough contrast on this glass
CFLAGS  += -DDISP_LCD_REFRESH_HZ=30 -DDISP_LCD_CONTRAST=LCD_FCR_CC_0
