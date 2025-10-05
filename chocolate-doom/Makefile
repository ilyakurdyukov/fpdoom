GAME = doom
NAME = chocolate-$(GAME)
OBJDIR = obj$(CHIP)_$(GAME)
SRCDIR = chocolate-doom
FPDOOM_DIR = ../fpdoom
USE_ASM = 1
NO_FLOAT = 1
NO_DEH = 1
KEYTRN_PC = 1

APP_SRCS = port_hacks port_sys scr_update
ifneq ($(USE_ASM), 0)
APP_SRCS += doomasm
endif

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
else
$(error unknown GAME)
endif

APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(COMMON_SRCS:%=$(OBJDIR)/common/%.o)
APP_OBJS2 += $(GAME_SRCS:%=$(OBJDIR)/$(GAME)/%.o)

include ../buildinc.make

CFLAGS := $(filter-out -funsigned-char, $(CFLAGS)) -fsigned-char

APP_CFLAGS += -std=c99 -pedantic
ifneq ($(USE_ASM), 0)
APP_CFLAGS += -DUSE_ASM
endif

GAME_CFLAGS += -DDISABLE_SDL2NET=1 -DDISABLE_SDL2MIXER=1
GAME_CFLAGS += -DNO_SOUND=1 -DNO_MOUSE=1 -DNO_NET=1
GAME_CFLAGS += -DNORANGECHECKING
ifneq ($(NO_DEH), 0)
GAME_CFLAGS += -DNO_DEH=1
endif
ifneq ($(NO_FLOAT), 0)
GAME_CFLAGS += -DNO_FLOAT=1
endif
GAME_CFLAGS += -I$(SRCDIR)/src -I$(SRCDIR)/textscreen
GAME_CFLAGS += -I. -ISDL2_dummy
GAME_CFLAGS += -Wno-unused -Wno-sign-compare
GAME_CFLAGS += -Wno-unused-parameter -Wno-strict-prototypes -Wno-newline-eof

GAME_CFLAGS := $(APP_CFLAGS) $(GAME_CFLAGS)
APP_CFLAGS := $(GAME_CFLAGS) -I$(SYSDIR)

$(OBJDIR)/app/%.o: $(FPDOOM_DIR)/%.c | objdir
	$(call compile_cc,$(APP_CFLAGS))

$(OBJDIR)/app/%.o: $(FPDOOM_DIR)/%.s | objdir
	$(call compile_asm,)

$(OBJDIR)/common/%.o: $(SRCDIR)/src/%.c | objdir
	$(call compile_cc,$(GAME_CFLAGS))

$(OBJDIR)/$(GAME)/%.o: $(SRCDIR)/src/$(GAME)/%.c | objdir
	$(call compile_cc,$(GAME_CFLAGS))

