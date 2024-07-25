# 0 - detect, 1 - SC6531E, 2 - SC6531DA, 3 - SC6530
CHIP = 0
TWO_STAGE = 0
LIBC_SDIO = 0
PACK_RELOC = ../pack_reloc/pack_reloc
OBJDIR = obj$(CHIP)
SRCS = asmcode usbio common libc syscomm
ifeq ($(CHIP), 0)
SRCS += chip1_syscode chip2_syscode
else
SRCS += syscode
endif
ifneq ($(LIBC_SDIO), 0)
SRCS += microfat
endif

NAME = fptest
FPDOOM = ../fpdoom

SRCS += main sdio sfc

ifeq ($(TWO_STAGE), 0)
SRCS := start entry $(SRCS)
OBJS = $(SRCS:%=$(OBJDIR)/sys/%.o)
else
SRCS1 = start1 entry $(SRCS)
SRCS2 = start2 entry2 $(SRCS)
OBJS1 = $(SRCS1:%=$(OBJDIR)/sys/%.o)
OBJS2 = $(SRCS2:%=$(OBJDIR)/sys/%.o)
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

CFLAGS += -Wall -Wextra -funsigned-char
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
#CFLAGS += -DCHIPRAM_ARGS=1
endif

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

CFLAGS += -std=c99 -pedantic
#CFLAGS += -Wno-unused -Wno-unused-parameter

.PHONY: all clean

all: $(NAME).bin

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

$(OBJDIR):
	mkdir -p $@/sys

-include $(OBJS:.o=.d)

compile_fn = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/sys/%.o: %.c | $(OBJDIR)
	$(call compile_fn, -I$(FPDOOM))

$(OBJDIR)/sys/%.o: $(FPDOOM)/%.c | $(OBJDIR)
	$(call compile_fn,)

$(OBJDIR)/sys/chip1_%.o: $(FPDOOM)/%.c | $(OBJDIR)
	$(call compile_fn,-DCHIP=1 -DCHIP_AUTO)

$(OBJDIR)/sys/chip2_%.o: $(FPDOOM)/%.c | $(OBJDIR)
	$(call compile_fn,-DCHIP=2 -DCHIP_AUTO)

$(OBJDIR)/sys/%.o: %.s | $(OBJDIR)
	$(CC) $< -c -o $@

$(OBJDIR)/sys/%.o: $(FPDOOM)/%.s | $(OBJDIR)
	$(CC) $< -c -o $@

%.rel: %.elf
	$(PACK_RELOC) $< $@

ifeq ($(TWO_STAGE), 0)
$(OBJDIR)/$(NAME).elf: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

$(NAME).bin: $(OBJDIR)/$(NAME).elf $(OBJDIR)/$(NAME).rel
	$(OBJCOPY) -O binary -j .text $< $@
	cat $(OBJDIR)/$(NAME).rel >> $@
else
$(OBJDIR)/$(NAME)_part1.elf: $(OBJS1)
	$(CC) $(LFLAGS) $(OBJS1) -o $@

$(OBJDIR)/$(NAME)_part2.elf: $(OBJS2)
	$(CC) $(LFLAGS) $(OBJS2) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(NAME).bin: $(patsubst %,$(OBJDIR)/$(NAME)_part%,2.bin 1.bin 1.rel 2.rel)
	cat $^ > $@
endif

