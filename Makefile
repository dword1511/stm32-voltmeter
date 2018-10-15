PROGRAM     = stm32-voltmeter
OBJS        = main.o tick.o power.o
CROSS      ?= arm-none-eabi-
SERIAL     ?= /dev/ttyUSB0

###############################################################################

.SUFFIXES:

AR          = $(CROSS)ar
CC          = $(CROSS)gcc
LD          = $(CROSS)ld
OBJCOPY     = $(CROSS)objcopy
OBJDUMP     = $(CROSS)objdump
SIZE        = $(CROSS)size
NM          = $(CROSS)nm
#GDB         = $(CROSS)gdb
GDB         = gdb-multiarch

ELF         = $(PROGRAM).elf
BIN         = $(PROGRAM).bin
HEX         = $(PROGRAM).hex
MAP         = $(PROGRAM).map
DMP         = $(PROGRAM).out


CFLAGS     += -O2 -Wall -g3 -gdwarf
# Turning aggressive loop optimizations off since it does not work for loops longer than certain iterations
CFLAGS     += -fno-common -ffunction-sections -fdata-sections -fno-aggressive-loop-optimizations
CFLAGS     += -fbranch-target-load-optimize
# Aggressive optimizations
#CFLAGS     += -O3 -fgcse-sm -fgcse-las -fgcse-after-reload -funroll-loops -funswitch-loops
# Actually won't wotk
#CFLAGS     += -flto -fipa-pta
# Architecture-dependent
CFLAGS     += $(ARCH_FLAGS) -Ilibopencm3/include/ $(EXTRA_CFLAGS)

# LDPATH is required for libopencm3's ld scripts to work.
LDPATH      = libopencm3/lib/
LDFLAGS    += $(ARCH_FLAGS) -nostdlib -L$(LDPATH) -T$(LDSCRIPT) -Wl,-Map -Wl,$(MAP) -Wl,--gc-sections -Wl,--relax
LDLIBS     += $(LIBOPENCM3) -lc -lgcc

default: $(BIN) $(HEX) $(DMP) size

include config.mk
include board.mk

$(ELF): $(LDSCRIPT) $(OBJS) $(LIBOPENCM3)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS)

$(DMP): $(ELF)
	$(OBJDUMP) -d $< > $@

%.hex: %.elf
	$(OBJCOPY) -S -O ihex   $< $@

%.bin: %.elf
	$(OBJCOPY) -S -O binary $< $@

%.o: %.c *.h *.mk $(LIBOPENCM3)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBOPENCM3):
	# TODO: pass toolchain prefix
	git submodule update --init
	make -C libopencm3 CFLAGS="$(CFLAGS)" PREFIX=$(patsubst %-,%,$(CROSS)) $(OPENCM3_MK) V=1


.PHONY: clean distclean size symbols flash flash-dfu flash-stlink flash-isp debug

clean:
	rm -f $(OBJS) $(ELF) $(HEX) $(BIN) $(MAP) $(DMP)

distclean: clean
	make -C libopencm3 clean
	rm -f *.o *~ *.swp *.hex

# These targets want clean terminal output
size: $(ELF)
	@echo ""
	@$(SIZE) $<
	@echo ""

symbols: $(ELF)
	@$(NM) --demangle --size-sort -S $< | grep -v ' [bB] '

flash-dfu: $(BIN)
	@dfu-util -a 0 -d 0483:df11 -s 0x08000000:leave -D $<

flash-stlink: $(HEX)
	@killall st-util || echo
	@st-flash --reset erase
	@st-flash --reset --format ihex write $<

flash-isp: $(HEX)
	@stm32flash -w $< -v $(SERIAL)

debug: $(ELF) flash-stlink
	@killall st-util || echo
	@setsid st-util &
	@-$(GDB) $< -q -ex 'target extended-remote localhost:4242'
