# 0 - detect, 1 - SC6531E, 2 - SC6531DA, 3 - SC6530
CHIP = 0
TWO_STAGE = 1
LIBC_SDIO = 0
USE_ASM = 1
PACK_RELOC = ../pack_reloc/pack_reloc
OBJDIR = obj$(CHIP)
SRCS = asmcode usbio common libc syscomm syscode
ifneq ($(LIBC_SDIO), 0)
SRCS += sdio microfat
endif
SRCS += keytrn scr_update
ifneq ($(USE_ASM), 0)
SRCS += doomasm
endif

DOOM_DIR = ../doom_src
NAME = fpdoom
NM = nm

DOOM_SRCS = \
	doomdef doomstat dstrings tables f_finale f_wipe \
	d_main d_net d_items g_game m_menu m_misc m_argv m_bbox \
	m_fixed m_swap m_cheat m_random am_map p_ceilng p_doors \
	p_enemy p_floor p_inter p_lights p_map p_maputl p_plats \
	p_pspr p_setup p_sight p_spec p_switch p_mobj p_telept \
	p_tick p_saveg p_user r_bsp r_data r_draw r_main r_plane \
	r_segs r_sky r_things w_wad wi_stuff v_video st_lib \
	st_stuff hu_stuff hu_lib s_sound z_zone info sounds \
	i_system i_sound i_net i_video

ifeq ($(TWO_STAGE), 0)
SRCS := start entry $(SRCS) $(DOOM_SRCS)
OBJS = $(SRCS:%=$(OBJDIR)/%.o)
else
SRCS1 = start1 entry $(SRCS)
SRCS2 = start2 entry2 $(SRCS) $(DOOM_SRCS)
OBJS1 = $(SRCS1:%=$(OBJDIR)/%.o)
OBJS2 = $(SRCS2:%=$(OBJDIR)/%.o)
OBJS = $(OBJS1) $(OBJS2)
endif

LDSCRIPT = sc6531e_fdl.ld

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

CFLAGS += -Wall -Wextra -pedantic -std=c99 -fsigned-char
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
CFLAGS += -isystem include
ifneq ($(CHIP), 0)
CFLAGS += -DCHIP=$(CHIP)
endif
CFLAGS += -DTWO_STAGE=$(TWO_STAGE)
ifneq ($(LIBC_SDIO), 0)
CFLAGS += -DLIBC_SDIO=$(LIBC_SDIO) -DFAT_WRITE=1
#CFLAGS += -DFAT_DEBUG=1
#CFLAGS += -DCHIPRAM_ARGS=1
endif

DOOM_CFLAGS = -DEMBEDDED=2 -DNORMALUNIX -DLINUX
ifneq ($(USE_ASM), 0)
CFLAGS += -DUSE_ASM
endif
DOOM_CFLAGS += -Wno-unused-parameter -Wno-unused -Wno-missing-field-initializers
DOOM_CFLAGS += -Wno-strict-prototypes

# Clang's LTO doesn't work with the GCC toolchain
ifeq ($(findstring -gcc-toolchain,$(notdir $(CC))),)
#CFLAGS += -flto
endif

ifdef SYSROOT
CFLAGS += --sysroot="$(SYSROOT)"
endif

.PHONY: all clean

all: $(NAME).bin map
map: $(OBJDIR)/$(NAME)_map.txt

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

$(OBJDIR):
	mkdir -p $@

-include $(OBJS:.o=.d)

%.asm: %.c
	$(CC) $(CFLAGS) $< -S -o $@

compile_fn = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(call compile_fn,)

$(OBJDIR)/%.o: $(DOOM_DIR)/%.c | $(OBJDIR)
	$(call compile_fn,$(DOOM_CFLAGS))

$(OBJDIR)/%.o: %.s | $(OBJDIR)
	$(CC) $< -c -o $@

$(OBJDIR)/%.o: %.S | $(OBJDIR)
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

