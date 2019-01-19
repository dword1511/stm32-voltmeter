PROGRAM     = stm32-voltmeter
SRCS        = main.c tick.c power.c
CROSS      ?= arm-none-eabi-

###############################################################################

.SUFFIXES:

AR         := $(CROSS)ar
CC         := $(CROSS)gcc
LD         := $(CROSS)ld
OBJCOPY    := $(CROSS)objcopy
OBJDUMP    := $(CROSS)objdump
SIZE       := $(CROSS)size
NM         := $(CROSS)nm
GDB         = gdb-multiarch

ELF        := $(PROGRAM).elf
BIN        := $(PROGRAM).bin
HEX        := $(PROGRAM).hex
MAP        := $(PROGRAM).map
DMP        := $(PROGRAM).out


TOPDIR     := $(shell pwd)
DEPDIR     := $(TOPDIR)/.dep
OBJS        = $(SRCS:.c=.o)

# Debugging
CFLAGS     += -Wall -Wdouble-promotion -gdwarf-4 -g3
# Optimizations
# NOTE: without optimization everything will fail... Computational power is marginal.
CFLAGS     += -O3 -fbranch-target-load-optimize -fipa-pta -frename-registers -fgcse-sm -fgcse-las -fsplit-loops -fstdarg-opt
# Use these for debugging-friendly binary
#CFLAGS     += -O0
#CFLAGS     += -Og
# Disabling aggressive loop optimizations since it does not work for loops longer than certain iterations
CFLAGS     += -fno-aggressive-loop-optimizations
# Aggressive optimizations (unstable or causes huge binaries, e.g. peel-loops gives huge gpio_mode_setup, which is rarely used)
#CFLAGS     += -finline-functions -funroll-loops -fbranch-target-load-optimize2
# Architecture-dependent
CFLAGS     += $(ARCH_FLAGS) -Ilibopencm3/include/ $(EXTRA_CFLAGS)
# For MCUs
CFLAGS     += -fsingle-precision-constant -ffast-math -flto --specs=nano.specs -fno-common -ffunction-sections -fdata-sections

# Generate dependency information
CFLAGS     += -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
$(shell mkdir -p $(DEPDIR) >/dev/null)
PRECOMPILE  = mkdir -p $(dir $(DEPDIR)/$*.Td)
POSTCOMPILE = mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

# LDPATH is required for libopencm3's ld scripts to work.
LDPATH     := libopencm3/lib/
# NOTE: the rule will ensure CFLAGS are added during linking
LDFLAGS    += -nostdlib -L$(LDPATH) -T$(LDSCRIPT) -Wl,-Map -Wl,$(MAP) -Wl,--gc-sections -Wl,--relax
LDLIBS     += $(LIBOPENCM3) -lc -lgcc


BOARDMK     = board.mk
BOARDS     := $(sort $(notdir $(basename $(wildcard boards/*.mk))))


# Pass these to libopencm3's Makefile
PREFIX     := $(patsubst %-,%,$(CROSS))
V          := 0
export FP_FLAGS CFLAGS PREFIX V


ifeq (,$(wildcard ./board.mk))
default:
	@echo "Run \"make select\" to activiate one board."

else
default: $(BIN) $(HEX) $(DMP) size

include $(BOARDMK)


$(ELF): $(LDSCRIPT) $(OBJS) $(LIBOPENCM3) Makefile $(BOARDMK)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS)

$(DMP): $(ELF)
	$(OBJDUMP) -d $< > $@

%.hex: %.elf
	$(OBJCOPY) -S -O ihex   $< $@

%.bin: %.elf
	$(OBJCOPY) -S -O binary $< $@

%.o: %.c $(LIBOPENCM3) Makefile $(BOARDMK)
	$(PRECOMPILE)
	$(CC) $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

# NOTE: libopencm3's Makefile is unaware of top-level Makefile changes, so force remake
# ld script also obtained here. However, has to touch it to prevent unnecessary remakes
$(LIBOPENCM3) $(LDSCRIPT): Makefile $(BOARDMK)
	git submodule update --init
	make -B -C libopencm3 CFLAGS="$(CFLAGS)" PREFIX=$(patsubst %-,%,$(CROSS)) $(OPENCM3_MK)
	touch $(LDSCRIPT)


.PHONY: clean distclean select size symbols symbols_bss flash flash-dfu flash-stlink flash-isp debug

# These targets want clean terminal output
size: $(ELF)
	@echo ""
	@$(SIZE) $<
	@echo ""

symbols: $(ELF)
	@$(NM) --demangle --size-sort -S $< | grep -v ' [bB] '

symbols_bss: $(ELF)
	@$(NM) --demangle --size-sort -S $< | grep ' [bB] '

flash-dfu: $(BIN)
	@dfu-util -a 0 -d 0483:df11 -s 0x08000000:leave -D $<

flash-stlink: $(HEX)
	@killall st-util || echo
	@st-flash --reset --format ihex write $<

flash-isp: $(HEX)
	@stm32flash -w $< -v $(SERIAL)

debug: $(ELF) flash-stlink
	@killall st-util || echo
	@setsid st-util &
	@-$(GDB) $< -q -ex 'target extended-remote localhost:4242'

# Dependencies
$(DEPDIR)/%.d:
.PRECIOUS: $(DEPDIR)/%.d
include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))

endif

clean:
	rm -f $(OBJS) $(ELF) $(HEX) $(BIN) $(MAP) $(DMP)
	rm -rf $(DEPDIR)

distclean: clean
	make -C libopencm3 clean
	rm -f *.o *~ *.swp *.hex board.mk

# Funky things can happen, reset terminal first.
select: clean
	@ \
	reset; \
	ITEMS=`n=0; for t in $(BOARDS); do echo $$n $$t; n=$$(($$n+1)); done`; \
	exec 3>&1; \
	CHOICE=$$(dialog --clear --no-shadow --title 'Board' --menu 'Choose one to build firmware for:' 12 40 4 $$ITEMS 2>&1 1>&3); \
	exec 3>&-; clear; \
	if [ -z "$${CHOICE}" ]; then exit 1; fi; \
	TARGET=`echo $$ITEMS | cut -d ' ' -f $$(($$CHOICE*2+2))`; \
	ln -sfT boards/$$TARGET.mk $(BOARDMK);\
	echo "Board set to $$TARGET."
