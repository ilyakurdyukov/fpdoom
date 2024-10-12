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

NAME = wolf3d
GAMESRC = Wolf4SDL
OBJDIR = obj$(CHIP)
FPDOOM = ../fpdoom
HOSTCC = cc
NM = nm

ifeq ($(GAMEVER),)
GAMEVER = CARMACIZED GOODTIMES # v1.4
ifneq ($(filter %_apo,$(NAME)),)
GAMEVER = CARMACIZED # v1.4 apogee
else ifneq ($(filter %_sw,$(NAME)),)
GAMEVER = CARMACIZED UPLOAD # sw v1.4
else ifneq ($(filter %_sod,$(NAME)),)
GAMEVER = CARMACIZED GOODTIMES SPEAR
else ifneq ($(filter %_sdm,$(NAME)),)
GAMEVER = CARMACIZED SPEAR SPEARDEMO
endif
endif
ifneq ($(GAMEVER),)
GAME_CFLAGS += -DVERSIONALREADYCHOSEN $(GAMEVER:%=-D%)
endif

GAME_SRCS = \
	id_ca id_in id_pm id_sd id_us id_vh id_vl \
	signon wl_act1 wl_act2 wl_agent wl_atmos \
	wl_cloudsky wl_debug wl_draw wl_game wl_inter \
	wl_main wl_menu wl_parallax wl_plane wl_play \
	wl_scale wl_shade wl_state wl_text wl_utils

SRCS += extra scr_update wlsys_fp
ifneq ($(USE_ASM), 0)
SRCS += wolfasm
endif

GAME_OBJS = $(GAME_SRCS:%=$(OBJDIR)/game/%.o)

ifeq ($(TWO_STAGE), 0)
SRCS := start entry $(SRCS)
OBJS = $(SRCS:%=$(OBJDIR)/sys/%.o) $(GAME_OBJS)
else
SRCS1 = start1 entry $(SRCS)
SRCS2 = start2 entry2 $(SRCS)
OBJS1 = $(SRCS1:%=$(OBJDIR)/sys/%.o)
OBJS2 = $(SRCS2:%=$(OBJDIR)/sys/%.o) $(GAME_OBJS)
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

GAME_CFLAGS += -DEMBEDDED=2 -DNO_SOUND=1 -DNO_MOUSE=1 -DNO_JOYSTICK=1
GAME_CFLAGS += -DVIEWMAP -DREVEALMAP
#GAME_CFLAGS += -DTRACE_UPDATE=1
ifeq ($(LIBC_SDIO), 0)
GAME_CFLAGS += -DLINUX_FN_FIX=1
endif
ifneq ($(USE_ASM), 0)
GAME_CFLAGS += -DUSE_ASM
endif
ifneq ($(NO_FLOAT), 0)
GAME_CFLAGS += -DNO_FLOAT=1 -I$(OBJDIR)
endif
GAME_CFLAGS += -I$(GAMESRC) -I.
GAME_CFLAGS += -Wno-unused -Wno-unused-parameter
GAME_CFLAGS += -Wno-strict-prototypes

.PHONY: all clean map

all: $(NAME).bin map
map: $(OBJDIR)/$(NAME)_map.txt

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

$(OBJDIR):
	mkdir -p $@/sys $@/game

%.asm: %.c
	$(CC) $(CFLAGS) $< -S -o $@

ifneq ($(NO_FLOAT), 0)
$(OBJDIR)/game/wl_draw.o: $(OBJDIR)/wolf3d_tables.h

$(OBJDIR)/calctables: calctables.c | $(OBJDIR)
	$(HOSTCC) -O2 $< -o $@ -lm

$(OBJDIR)/wolf3d_tables.h: $(OBJDIR)/calctables
	$(OBJDIR)/calctables $@
endif

-include $(OBJS:.o=.d)

compile_fn = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/game/%.o: $(GAMESRC)/%.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS))

$(OBJDIR)/sys/%.o: %.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS) -I$(FPDOOM))

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

