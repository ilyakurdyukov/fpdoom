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
ifneq ($(USE_ASM), 0)
SRCS += doomasm
endif

NO_DEH = 1
SRCDIR = chocolate-doom
OBJDIR = obj$(CHIP)
GAME = doom
ifneq ($(words $(GAME)), 1)
$(error GAME must not contain spaces in the name)
endif
NAME = chocolate-$(GAME)
FPDOOM = ../fpdoom
NM = nm

SRCS += port_hacks port_sys scr_update

COMMON_SRCS = i_system m_argv m_misc d_event \
	d_iwad d_loop d_mode gusconf i_cdmus i_flmusic \
	i_glob i_musicpack m_bbox m_cheat m_config m_controls \
	m_fixed memio tables v_diskicon v_video w_main \
	w_wad w_file w_file_stdc w_merge z_zone
ifeq ($(NO_DEH), 0)
COMMON_SRCS += deh_str deh_io deh_main deh_mapping deh_text
endif

ifeq ($(GAME), doom)
GAME_CFLAGS += -DGAME_DOOM
GAME_SRCS = am_map deh_misc d_items d_main d_net doomdef doomstat dstrings \
	f_finale f_wipe g_game hu_lib hu_stuff info m_menu m_random p_ceilng \
	p_doors p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj \
	p_plats p_pspr p_saveg p_setup p_sight p_spec p_switch p_telept \
	p_tick p_user r_bsp r_data r_draw r_main r_plane r_segs r_sky \
	r_things s_sound sounds statdump st_lib st_stuff wi_stuff
ifeq ($(NO_DEH), 0)
GAME_SRCS += deh_ammo deh_bexstr deh_cheat deh_doom \
	deh_frame deh_ptr deh_sound deh_thing deh_weapon
endif
else ifeq ($(GAME), heretic)
GAME_CFLAGS += -DGAME_HERETIC
GAME_SRCS = am_map ct_chat d_main d_net \
	f_finale g_game info in_lude m_random mn_menu p_ceilng \
	p_doors p_enemy p_floor p_inter p_lights p_map p_maputl \
	p_mobj p_plats p_pspr p_saveg p_setup p_sight p_spec \
	p_switch p_telept p_tick p_user r_bsp r_data r_draw \
	r_main r_plane r_segs r_things sb_bar sounds s_sound
ifeq ($(NO_DEH), 0)
GAME_SRCS += deh_ammo deh_frame deh_htext \
	deh_htic deh_sound deh_thing deh_weapon
endif
else ifeq ($(GAME), hexen)
GAME_CFLAGS += -DGAME_HEXEN
GAME_SRCS = a_action am_map ct_chat d_net f_finale g_game \
	h2_main info in_lude m_random mn_menu p_acs p_anim p_ceilng \
	p_doors p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj \
	po_man p_plats p_pspr p_setup p_sight p_spec p_switch p_telept \
	p_things p_tick p_user r_bsp r_data r_draw r_main r_plane r_segs \
	r_things s_sound sb_bar sc_man sn_sonix sounds st_start sv_save
else ifeq ($(GAME), strife)
GAME_CFLAGS += -DGAME_STRIFE
GAME_SRCS = am_map deh_misc d_items d_main \
	d_net doomdef doomstat dstrings f_finale f_wipe g_game hu_lib \
	hu_stuff info m_menu m_random m_saves p_ceilng p_dialog p_doors \
	p_enemy p_floor p_inter p_lights p_map p_maputl p_mobj p_plats \
	p_pspr p_saveg p_setup p_sight p_spec p_switch p_telept p_tick \
	p_user r_bsp r_data r_draw r_main r_plane r_segs r_sky \
	r_things s_sound sounds st_lib st_stuff wi_stuff
ifeq ($(NO_DEH), 0)
GAME_SRCS += deh_ammo deh_cheat deh_strife deh_frame \
	deh_ptr deh_sound deh_thing deh_weapon
endif
endif

GAME_OBJS = $(patsubst %,$(OBJDIR)/common/%.o,$(COMMON_SRCS))
GAME_OBJS += $(patsubst %,$(OBJDIR)/$(GAME)/%.o,$(GAME_SRCS))

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

GAME_CFLAGS += -DDISABLE_SDL2NET=1 -DDISABLE_SDL2MIXER=1
GAME_CFLAGS += -DEMBEDDED=2 -DNO_SOUND=1 -DNO_MOUSE=1 -DNO_NET=1
GAME_CFLAGS += -DNORANGECHECKING
ifneq ($(NO_DEH), 0)
GAME_CFLAGS += -DNO_DEH=1
endif
ifneq ($(USE_ASM), 0)
CFLAGS += -DUSE_ASM
endif
ifneq ($(NO_FLOAT), 0)
GAME_CFLAGS += -DNO_FLOAT=1
endif
GAME_CFLAGS += -I$(SRCDIR)/src -I$(SRCDIR)/textscreen
GAME_CFLAGS += -I. -ISDL2_dummy
GAME_CFLAGS += -Wno-unused -Wno-sign-compare
GAME_CFLAGS += -Wno-unused-parameter -Wno-strict-prototypes -Wno-newline-eof

.PHONY: all clean map

all: $(NAME).bin map
map: $(OBJDIR)/$(NAME)_map.txt

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

$(OBJDIR):
	mkdir -p $@/sys $@/common $@/$(GAME)

%.asm: %.c
	$(CC) $(CFLAGS) $< -S -o $@

-include $(OBJS:.o=.d)

compile_fn = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/common/%.o: $(SRCDIR)/src/%.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS))

$(OBJDIR)/$(GAME)/%.o: $(SRCDIR)/src/$(GAME)/%.c | $(OBJDIR)
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

