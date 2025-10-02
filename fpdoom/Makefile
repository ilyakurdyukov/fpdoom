NAME = fpdoom
DOOM_DIR = ../doom_src
USE_ASM = 1
LTO = 0

APP_SRCS = keytrn scr_update
ifneq ($(USE_ASM), 0)
APP_SRCS += doomasm
endif

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

APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(DOOM_SRCS:%=$(OBJDIR)/doom/%.o)

include ../buildinc.make

CFLAGS := $(filter-out -funsigned-char, $(CFLAGS)) -fsigned-char

APP_CFLAGS = -std=c99 -pedantic
ifneq ($(USE_ASM), 0)
APP_CFLAGS += -DUSE_ASM
endif

DOOM_CFLAGS := $(APP_CFLAGS) -DEMBEDDED=2 -DNORMALUNIX -DLINUX
DOOM_CFLAGS += -DNORANGECHECKING
DOOM_CFLAGS += -Wno-unused-parameter -Wno-unused -Wno-missing-field-initializers
DOOM_CFLAGS += -Wno-strict-prototypes

APP_CFLAGS += -I$(SYSDIR)

$(OBJDIR)/doom/%.o: $(DOOM_DIR)/%.c | objdir
	$(call compile_cc,$(DOOM_CFLAGS))

