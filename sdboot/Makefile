# 0 - detect, 1 - SC6531E, 2 - SC6531DA, 3 - SC6530
CHIP = 0
LIBC_SDIO = 3
SRCS = asmcode usbio common libc syscomm

ifneq ($(CHIP), 0)
NAME = sdboot$(CHIP)
else
NAME = sdboot
endif
FPDOOM = ../fpdoom
OBJDIR = obj$(CHIP)

SRCS := start entry sdio microfat main $(SRCS)
OBJS = $(SRCS:%=$(OBJDIR)/sys/%.o)

LDSCRIPT = $(FPDOOM)/sc6531e_fdl1.ld

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
LFLAGS = -nostartfiles -nodefaultlibs -nostdlib -Wl,-T,$(LDSCRIPT) -Wl,--gc-sections -Wl,-z,notext
# -Wl,--no-dynamic-linker
#LFLAGS += -Wl,--defsym,IMAGE_ALIGN=0x1000
LFLAGS += $(LD_EXTRA)
CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
CFLAGS += -isystem $(FPDOOM)/include
ifneq ($(CHIP), 0)
CFLAGS += -DCHIP=$(CHIP)
endif
CFLAGS += -DSDIO_NO_PINMAP=1
ifneq ($(LIBC_SDIO), 0)
CFLAGS += -DLIBC_SDIO=$(LIBC_SDIO)
ifeq ($(LIBC_SDIO), 3)
CFLAGS += -DSDIO_VERBOSE=0 -DSDIO_SHL=CHIPRAM_ADDR
endif
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
CFLAGS += -Wno-unused -Wno-unused-parameter
CFLAGS += -Wno-strict-prototypes
CFLAGS += -DSDBOOTKEY=-1

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

$(OBJDIR)/sys/%.o: %.s | $(OBJDIR)
	$(CC) $< -c -o $@

$(OBJDIR)/sys/%.o: $(FPDOOM)/%.s | $(OBJDIR)
	$(CC) $< -c -o $@

$(OBJDIR)/$(NAME).elf: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

jump4m.bin: $(OBJDIR)/sys/jump4m.o
	$(OBJCOPY) -O binary $< $@

%.bin: $(OBJDIR)/%.elf
	$(OBJCOPY) -O binary -j .text $< $@

