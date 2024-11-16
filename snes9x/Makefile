# 0 - detect, 1 - SC6531E, 2 - SC6531DA, 3 - SC6530
CHIP = 0
TWO_STAGE = 1
LIBC_SDIO = 0
USE_ASM = 1
NO_FLOAT = 1
PACK_RELOC = ../pack_reloc/pack_reloc
SRCS = asmcode usbio common libc syscomm syscode
ifneq ($(LIBC_SDIO), 0)
SRCS += sdio microfat
endif

ifneq ($(filter %_16bit,$(NAME)),)
USE_16BIT = 1
else
USE_16BIT = 0
endif
ifeq ($(USE_16BIT), 0)
NAME = snes9x
else
NAME = snes9x_16bit
endif
SNES9X = snes9x_src
OBJDIR = obj$(CHIP)
FPDOOM = ../fpdoom
HOSTCC = cc
NM = nm

DSP1_NEW = 1
NO_SA1 = 1
NO_C4 = $(NO_FLOAT)
ifeq ($(DSP1_NEW), 1)
NO_DSP1 = 0
else
NO_DSP1 = $(NO_FLOAT)
endif
NO_DSP4 = $(NO_FLOAT)

SRCS += snes9x_fp extra scr_update
ifneq ($(USE_ASM), 0)
SRCS += snesasm
endif

SNES_SRCS = apu cheats2 \
	clip cpu cpuexec cpuops data dma dsp1 \
	fxemu fxinst gfx globals memmap \
	obc1 ppu sdd1 seta seta010 seta011 seta018 \
	snapshot soundux spc700 spc7110 srtc tile

ifeq ($(NO_SA1), 0)
SNES_SRCS += sa1 sa1cpu
endif
ifeq ($(NO_C4), 0)
SNES_SRCS += c4 c4emu
endif

SNES_OBJS = $(SNES_SRCS:%=$(OBJDIR)/snes/%.o)

ifeq ($(TWO_STAGE), 0)
SRCS := start entry $(SRCS)
OBJS = $(SRCS:%=$(OBJDIR)/sys/%.o) $(SNES_OBJS)
else
SRCS1 = start1 entry $(SRCS)
SRCS2 = start2 entry2 $(SRCS)
OBJS1 = $(SRCS1:%=$(OBJDIR)/sys/%.o)
OBJS2 = $(SRCS2:%=$(OBJDIR)/sys/%.o) $(SNES_OBJS)
OBJS = $(OBJS1) $(OBJS2)
endif

LDSCRIPT = $(FPDOOM)/sc6531e_fdl.ld

ifdef TOOLCHAIN
CC = "$(TOOLCHAIN)"-gcc
CXX = "$(TOOLCHAIN)"-g++
OBJCOPY = "$(TOOLCHAIN)"-objcopy
endif

COMPILER = $(findstring clang,$(notdir $(CC)))
ifeq ($(COMPILER), clang)
# Clang
CFLAGS = -Oz
CXX = "$(CC)++"
else
# GCC
CFLAGS = -Os
endif

CFLAGS += -Wall -Wextra -fsigned-char
CFLAGS += -fno-PIE -ffreestanding -march=armv5te -mthumb $(EXTRA_CFLAGS) -fno-strict-aliasing
CFLAGS += -fomit-frame-pointer
CFLAGS += -ffunction-sections -fdata-sections
LFLAGS = -pie -nostartfiles -nodefaultlibs -nostdlib -Wl,-T,$(LDSCRIPT) -Wl,--gc-sections -Wl,-z,notext
ifeq ($(CHIP), 2)
LFLAGS += -Wl,--defsym,IMAGE_START=0x34000000
else
LFLAGS += -Wl,--defsym,IMAGE_START=0x14000000
endif
# -Wl,--no-dynamic-linker
LFLAGS += $(LD_EXTRA)
CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
CFLAGS += -isystem $(FPDOOM)/include
ifneq ($(CHIP), 0)
CFLAGS += -DCHIP=$(CHIP)
endif
CFLAGS += -DTWO_STAGE=$(TWO_STAGE)
ifneq ($(LIBC_SDIO), 0)
CFLAGS += -DLIBC_SDIO=$(LIBC_SDIO) -DFAT_WRITE=1
#CFLAGS += -DFAT_DEBUG=1
endif
CFLAGS += -DAPP_MEM_RESERVE -DAPP_DATA_EXCEPT

ifeq ($(COMPILER), clang)
# Clang's bug workaround
CFLAGS += -mcpu=+nodsp
endif

LTO = 1
ifneq ($(LTO), 0)
# Clang's LTO doesn't work with the GCC toolchain
ifeq ($(findstring -gcc-toolchain,$(notdir $(CC))),)
CFLAGS += -flto
endif
endif

ifdef SYSROOT
CFLAGS += --sysroot="$(SYSROOT)"
endif

CFLAGS += -DHAVE_STRINGS_H -DHAVE_STDINT_H -DRIGHTSHIFT_IS_SAR
CFLAGS += -DNOSOUND -DEMBEDDED=2 -DNDEBUG
CFLAGS += -DUSE_16BIT=$(USE_16BIT)
CFLAGS += -DVAR_CYCLES -DCPU_SHUTDOWN -DSPC700_C
#CFLAGS += -DSPC700_SHUTDOWN
CFLAGS += -I$(SNES9X) -I$(SNES9X)/unzip
ifneq ($(USE_ASM), 0)
CFLAGS += -DUSE_ASM
endif
ifneq ($(NO_FLOAT), 0)
CFLAGS += -DNO_FLOAT=1
endif
ifneq ($(NO_SA1), 0)
CFLAGS += -DNO_SA1=1
endif
ifneq ($(NO_C4), 0)
CFLAGS += -DNO_C4=1
endif
ifneq ($(NO_DSP1), 0)
CFLAGS += -DNO_DSP1=1
else ifneq ($(DSP1_NEW), 0)
CFLAGS += -DDSP1_NEW=1
endif
ifneq ($(NO_DSP4), 0)
CFLAGS += -DNO_DSP4=1
endif
CFLAGS += -Wno-unused -Wno-unused-parameter
CFLAGS += -Wno-deprecated-register -Wno-null-pointer-subtraction
CFLAGS += -Wno-misleading-indentation

CXXFLAGS := $(CFLAGS) -std=c++14 -fno-exceptions -fno-rtti -Wno-narrowing
CFLAGS += -std=c99 -pedantic

.PHONY: all clean map

all: $(NAME).bin map
map: $(OBJDIR)/$(NAME)_map.txt

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

$(OBJDIR):
	mkdir -p $@/sys $@/snes

%.asm: %.c
	$(CC) $(CFLAGS) $< -S -o $@

-include $(OBJS:.o=.d)

compile_fn = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/snes/%.o: $(SNES9X)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/sys/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/sys/%.o: %.c | $(OBJDIR)
	$(call compile_fn,-I$(FPDOOM))

$(OBJDIR)/sys/%.o: $(FPDOOM)/%.c | $(OBJDIR)
	$(call compile_fn,-DCXX_SUPPORT)

$(OBJDIR)/sys/%.o: $(FPDOOM)/%.s | $(OBJDIR)
	$(CC) $< -c -o $@

$(OBJDIR)/sys/%.o: %.s | $(OBJDIR)
	$(CC) $< -c -o $@

%.rel: %.elf
	$(PACK_RELOC) $< $@

ifeq ($(TWO_STAGE), 0)
$(OBJDIR)/$(NAME).elf: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

$(OBJDIR)/$(NAME)_map.txt: $(OBJDIR)/$(NAME).elf
	$(NM) -n $< > $@

$(NAME).bin: $(OBJDIR)/$(NAME).elf $(OBJDIR)/$(NAME).rel
	$(OBJCOPY) -O binary -j .text $< $@
	cat $(OBJDIR)/$(NAME).rel >> $@
else
$(OBJDIR)/$(NAME)_part1.elf: $(OBJS1)
	$(CC) $(LFLAGS) $(OBJS1) -o $@

$(OBJDIR)/$(NAME)_part2.elf: $(OBJS2)
	$(CC) $(LFLAGS) $(OBJS2) -o $@

$(OBJDIR)/$(NAME)_map.txt: $(OBJDIR)/$(NAME)_part2.elf
	$(NM) -n $< > $@

%.bin: %.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(NAME).bin: $(patsubst %,$(OBJDIR)/$(NAME)_part%,2.bin 1.bin 1.rel 2.rel)
	cat $^ > $@
endif

