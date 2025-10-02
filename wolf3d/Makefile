NAME = wolf3d
GAMESRC = Wolf4SDL
USE_ASM = 1
NO_FLOAT = 1

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

APP_SRCS = extra scr_update wlsys_fp
ifneq ($(USE_ASM), 0)
APP_SRCS += wolfasm
endif

GAME_SRCS = \
	id_ca id_in id_pm id_sd id_us id_vh id_vl \
	signon wl_act1 wl_act2 wl_agent wl_atmos \
	wl_cloudsky wl_debug wl_draw wl_game wl_inter \
	wl_main wl_menu wl_parallax wl_plane wl_play \
	wl_scale wl_shade wl_state wl_text wl_utils

APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(GAME_SRCS:%=$(OBJDIR)/game/%.o)

include ../buildinc.make

CFLAGS := $(filter-out -funsigned-char, $(CFLAGS)) -fsigned-char

APP_CFLAGS = -std=c99 -pedantic

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

GAME_CFLAGS := $(APP_CFLAGS) $(GAME_CFLAGS)
APP_CFLAGS := $(GAME_CFLAGS) -I$(SYSDIR)

ifneq ($(NO_FLOAT), 0)
$(OBJDIR)/game/wl_draw.o: $(OBJDIR)/wolf3d_tables.h

$(OBJDIR)/calctables: calctables.c | $(OBJDIR)
	$(HOSTCC) -O2 $< -o $@ -lm

$(OBJDIR)/wolf3d_tables.h: $(OBJDIR)/calctables
	$(OBJDIR)/calctables $@
endif

$(OBJDIR)/game/%.o: $(GAMESRC)/%.c | objdir
	$(call compile_cc,$(GAME_CFLAGS))

