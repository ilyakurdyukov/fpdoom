# 0 - detect, 1 - SC6531E, 2 - SC6531, 3 - SC6530
CHIP = 0
TWO_STAGE ?= 1
LIBC_SDIO ?= 0
PACK_RELOC = ../pack_reloc/pack_reloc
OBJDIR = obj$(CHIP)
SYSDIR = ../fpdoom
HOSTCC = cc
NM = nm

SYS_SRCS += asmcode usbio common libc syscomm syscode
ifneq ($(LIBC_SDIO), 0)
SYS_SRCS += sdio microfat
endif

# for fptest
ifneq ($(SYS_EXTRA),)
SYS_SRCS := $(filter-out $(SYS_EXTRA), $(SYS_SRCS)) $(SYS_EXTRA)
endif

ifeq ($(TWO_STAGE), 0)
SYS_SRCS2 = start entry $(SYS_SRCS)
OBJS = $(SYS_SRCS2:%=$(OBJDIR)/sys/%.o) $(APP_OBJS1) $(APP_OBJS2)
else ifeq ($(TWO_STAGE), 1)
SYS_SRCS1 = start1 entry $(SYS_SRCS)
SYS_SRCS2 = start2 entry2 $(SYS_SRCS)
OBJS1 = $(SYS_SRCS1:%=$(OBJDIR)/sys/%.o) $(APP_OBJS1)
OBJS2 = $(SYS_SRCS2:%=$(OBJDIR)/sys/%.o) $(APP_OBJS1) $(APP_OBJS2)
OBJS = $(OBJS1) $(OBJS2)
endif

LDSCRIPT = $(SYSDIR)/sc6531e_fdl.ld

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
CFLAGS += -isystem $(SYSDIR)/include
ifneq ($(CHIP), 0)
CFLAGS += -DCHIP=$(CHIP)
endif
ifneq ($(TWO_STAGE), -1)
CFLAGS += -DTWO_STAGE=$(TWO_STAGE)
endif
ifneq ($(LIBC_SDIO), 0)
CFLAGS += -DLIBC_SDIO=$(LIBC_SDIO) -DFAT_WRITE=1
#CFLAGS += -DFAT_DEBUG=1
#CFLAGS += -DCHIPRAM_ARGS=1
endif

SYS_CFLAGS = -std=c99 -pedantic

ifeq ($(COMPILER), clang)
# Clang's bug workaround
CFLAGS += -mcpu=+nodsp
endif

LTO ?= 1
ifneq ($(LTO), 0)
# Clang's LTO doesn't work with the GCC toolchain
ifeq ($(findstring -gcc-toolchain,$(notdir $(CC))),)
CFLAGS += -flto
endif
endif

ifdef SYSROOT
CFLAGS += --sysroot="$(SYSROOT)"
endif

.PHONY: all clean map objdir

all: $(NAME).bin
map: $(OBJDIR)/$(NAME)_map.txt

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

objdir:
	mkdir -p $(sort $(dir $(OBJS)))

-include $(OBJS:.o=.d)

compile_cc = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@
compile_asm = $(CC) $(1) $< -c -o $@
compile_cxx = $(CXX) $(CXXFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

%.rel: %.elf
	$(PACK_RELOC) $< $@

$(OBJDIR)/app/%.o: %.c | objdir
	$(call compile_cc,$(APP_CFLAGS))

$(OBJDIR)/app/%.o: %.s | objdir
	$(call compile_asm,)

$(OBJDIR)/sys/%.o: $(SYSDIR)/%.c | objdir
	$(call compile_cc,$(SYS_CFLAGS))

$(OBJDIR)/sys/%.o: $(SYSDIR)/%.s | objdir
	$(call compile_asm,)

ifeq ($(TWO_STAGE), 0)
$(OBJDIR)/$(NAME).elf: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

$(OBJDIR)/$(NAME)_map.txt: $(OBJDIR)/$(NAME).elf
	$(NM) -n $< > $@

$(NAME).bin: $(OBJDIR)/$(NAME).elf $(OBJDIR)/$(NAME).rel
	$(OBJCOPY) -O binary -j .text $< $@
	cat $(OBJDIR)/$(NAME).rel >> $@

else ifeq ($(TWO_STAGE), 1)
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
