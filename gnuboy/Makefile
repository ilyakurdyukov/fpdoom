# 0 - detect, 1 - SC6531E, 2 - SC6531DA, 3 - SC6530
CHIP = 0
TWO_STAGE = 1
LIBC_SDIO = 0
USE_ASM = 1
NO_FLOAT = 1
NO_GZ = 1
NO_XZ = 1
NO_ZIP = 1
NO_MENU = 1
PACK_RELOC = ../pack_reloc/pack_reloc
SRCS = asmcode usbio common libc syscomm syscode
ifneq ($(LIBC_SDIO), 0)
SRCS += sdio microfat
endif
ifneq ($(USE_ASM), 0)
SRCS += gbasm
endif

NO_DEH = 1
SRCDIR = gnuboy
OBJDIR = obj$(CHIP)
NAME = gnuboy
FPDOOM = ../fpdoom
NM = nm

SRCS += port_hacks port_sys scr_update

GNUBOY_SRCS = lcd refresh lcdc palette cpu mem rtc hw sound \
	events keytable loader save debug emu main \
	rccmds rckeys rcvars rcfile exports \
	split path

ifeq ($(NO_NOMENU), 0)
GNUBOY_SRCS = menu
endif
ifeq ($(NO_GZ), 0)
GNUBOY_SRCS = inflate
endif
ifeq ($(NO_XZ), 0)
GNUBOY_SRCS = xz_crc32 xz_crc64 xz_dec_lzma2 xz_dec_stream xz_dec_bcj
endif
ifeq ($(NO_ZIP), 0)
GNUBOY_SRCS = miniz_tinfl
endif

GNUBOY_OBJS = $(patsubst %,$(OBJDIR)/gnuboy/%.o,$(GNUBOY_SRCS))

ifeq ($(TWO_STAGE), 0)
SRCS := start entry $(SRCS)
OBJS = $(SRCS:%=$(OBJDIR)/sys/%.o) $(GNUBOY_OBJS)
else
SRCS1 = start1 entry $(SRCS)
SRCS2 = start2 entry2 $(SRCS)
OBJS1 = $(SRCS1:%=$(OBJDIR)/sys/%.o)
OBJS2 = $(SRCS2:%=$(OBJDIR)/sys/%.o) $(GNUBOY_OBJS)
OBJS = $(OBJS1) $(OBJS2)
endif

LDSCRIPT = $(FPDOOM)/sc6531e_fdl.ld

ifdef TOOLCHAIN
CC = "$(TOOLCHAIN)"-gcc
OBJCOPY = "$(TOOLCHAIN)"-objcopy
endif

COMPILER = $(findstring clang,$(notdir $(CC)))
ifeq ($(COMPILER), clang)
# Clang
CFLAGS = -Oz
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

CFLAGS += -std=c99 -pedantic
CFLAGS += -DATEXIT_MAX=2

GAME_CFLAGS += -DEMBEDDED=2 -DIS_LITTLE_ENDIAN
ifneq ($(NO_MENU), 0)
CFLAGS += -DNO_MENU
endif
ifneq ($(NO_GZ), 0)
GAME_CFLAGS += -DNO_GZ
endif
ifneq ($(NO_XZ), 0)
GAME_CFLAGS += -DNO_XZ
endif
ifneq ($(NO_ZIP), 0)
GAME_CFLAGS += -DNO_ZIP
endif
ifneq ($(USE_ASM), 0)
PORT_CFLAGS += -DUSE_ASM
endif
ifneq ($(NO_FLOAT), 0)
GAME_CFLAGS += -DNO_FLOAT=1
endif
GAME_CFLAGS += -Wno-unused -Wno-sign-compare
GAME_CFLAGS += -Wno-unused-parameter -Wno-strict-prototypes -Wno-newline-eof

.PHONY: all clean map

all: $(NAME).bin map
map: $(OBJDIR)/$(NAME)_map.txt

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

$(OBJDIR):
	mkdir -p $@/sys $@/gnuboy

%.asm: %.c
	$(CC) $(CFLAGS) $< -S -o $@

-include $(OBJS:.o=.d)

compile_fn = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/gnuboy/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS))

$(OBJDIR)/sys/%.o: %.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS) $(PORT_CFLAGS) -I$(FPDOOM) -I$(SRCDIR))

$(OBJDIR)/sys/%.o: $(FPDOOM)/%.c | $(OBJDIR)
	$(call compile_fn,)

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

