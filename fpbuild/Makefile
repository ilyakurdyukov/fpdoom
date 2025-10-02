GAME = duke3d
NAME = fp$(GAME)
OBJDIR = obj$(CHIP)_$(GAME)
GAMESRC = jf$(GAME)/src
ENGINEROOT = jfbuild
MACTROOT = jfmact
AUDIOLIBROOT = jfaudiolib
USE_ASM = 1
NO_FLOAT = 1

ifeq ($(GAME), duke3d)
GAME_SRCS = \
	game actors gamedef global menues \
	player premap sector config \
	osdfuncs osdcmds grpscan sounds \
	version
else ifeq ($(GAME), sw)
GAME_SRCS = \
	actor ai anim border break bunny \
	cache cheats colormap config console \
	coolg coolie copysect demo draw eel \
	game girlninj goro grpscan hornet \
	interp interpsh inv jplayer jsector \
	jweapon lava light mclip menus miscactr \
	morph net ninja osdcmds osdfuncs panel \
	player predict quake ripper ripper2 rooms \
	rotator save scrip2 sector serp \
	setup skel skull slidor sounds spike \
	sprite sumo swconfig sync text track \
	vator vis wallmove warp weapon zilla \
	zombie saveable version
else ifeq ($(GAME), blood)
GAMESRC = NBlood/source/blood/src
GAME_SRCS = \
	actor ai aibat aibeast aiboneel aiburn \
	aicaleb aicerber aicult aigarg aighost \
	aigilbst aihand aihound aiinnoc aipod \
	airat aispid aitchern aizomba aizombf \
	blood callback choke common config \
	db demo dude endgame eventq fire fx \
	gamemenu getopt gfx gib globals gui \
	inifile iob levels loadsave map2d \
	messages misc network player qav qheap \
	sectorfx seq sound trig triggers \
	warp weapon controls credits gameutil \
	menu mirrors osdcmd replace resource \
	tile view screen compat version
else
$(error unknown GAME)
endif

BUILD_SRCS = \
	asmprot baselayer cache1d \
	compat crc32 defs engine \
	mmulti_null osd pragmas scriptfile \
	textfont talltextfont smalltextfont \
	version

MACT_SRCS = \
	util_lib file_lib control keyboard \
	mouse mathutil scriplib animlib

APP_SRCS = fp_layer unistd
ifeq ($(USE_ASM), 0)
BUILD_SRCS += a-c
else
APP_SRCS += buildasm a-new
endif

APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(GAME_SRCS:%=$(OBJDIR)/game/%.o)
APP_OBJS2 += $(BUILD_SRCS:%=$(OBJDIR)/jfbuild/%.o)
APP_OBJS2 += $(MACT_SRCS:%=$(OBJDIR)/jfmact/%.o)

include ../buildinc.make

ifeq ($(GAME), blood)
SYS_CFLAGS += -DCXX_SUPPORT
endif

CXXFLAGS := $(CFLAGS) -std=c++14 -fno-exceptions -fno-rtti -Wno-narrowing

GAME_CFLAGS = -DEMBEDDED=2 -DNO_SOUND=1 -DNO_MOUSE=1 -DNO_NET=1 -DNO_OSD=1
ifneq ($(USE_ASM), 0)
GAME_CFLAGS += -DUSE_ASM
endif
ifneq ($(NO_FLOAT), 0)
GAME_CFLAGS += -DNO_FLOAT=1 -I$(OBJDIR)
endif
GAME_CFLAGS += -DMAXXDIM=480 -DMAXYDIM=320
GAME_CFLAGS += -DNO_NEWSTYLE=1
# saves 256KB (slower voxels)
GAME_CFLAGS += -DNO_DISTRECIP=1
ifeq ($(GAME), duke3d)
GAME_CFLAGS += -DGAME_DUKE3D
# MAXYDIM >= 640 for ROTSCR
GAME_CFLAGS += -DNO_ROTSCR=1
# names=4944+5
GAME_CFLAGS += -DMAXTILES=5120 -DMAXCACHEOBJECTS=MAXTILES
# numwalls=5471 (E2L8), numsprites=1278 (E4L11)
GAME_CFLAGS += -DMAXWALLS=6144 -DMAXSPRITES=2048
# duke3d doesn't use voxels
GAME_CFLAGS += -DNO_VOXELS=1
else ifeq ($(GAME), sw)
GAME_CFLAGS += -DGAME_SW
GAME_CFLAGS += -DMAXTILES=6144 -DMAXCACHEOBJECTS=MAXTILES
# numwalls=7492 (wanton:$shore), numsprites=1961 ($yamato)
GAME_CFLAGS += -DMAXSPRITES=2048
GAME_CFLAGS += -DNO_CONSOLE=1
# saves 162KB
GAME_CFLAGS += -DMAXWALLSB=2048 -DMAXSPRITESONSCREEN=1024
else ifeq ($(GAME), blood)
GAME_CFLAGS += -DGAME_BLOOD
GAME_CFLAGS += -DMAXTILES=6144 -DMAXCACHEOBJECTS=MAXTILES
# numwalls=8002 (cp09), numsprites=1200 (cp09)
GAME_CFLAGS += -DMAXSPRITES=2048
GAME_CFLAGS += -DMAXWALLSB=2048 -DMAXSPRITESONSCREEN=1024
GAME_CFLAGS += -DNETCODE_DISABLE -DSMACKER_DISABLE
GAME_CFLAGS += -D__AMIGA__ -U__ANDROID__
CXXFLAGS += -Wno-char-subscripts -Wno-missing-braces
endif
GAME_CFLAGS += -I$(GAMESRC) -I$(ENGINEROOT)/include -I$(MACTROOT) -I$(AUDIOLIBROOT)/include
GAME_CFLAGS += -Wno-unused -Wno-unused-parameter
GAME_CFLAGS += -Wno-strict-prototypes
GAME_CXXFLAGS := $(GAME_CFLAGS)
GAME_CFLAGS += -std=c99 -pedantic
APP_CFLAGS := -I$(ENGINEROOT)/src -I$(SYSDIR) $(GAME_CFLAGS)

ifneq ($(NO_FLOAT), 0)
$(OBJDIR)/jfbuild/engine.o: $(OBJDIR)/engine_tables.h

$(OBJDIR)/calctables: calctables.c | $(OBJDIR)
	$(HOSTCC) -O2 $< -o $@ -lm

$(OBJDIR)/engine_tables.h: $(OBJDIR)/calctables
	$(OBJDIR)/calctables $@
endif

$(OBJDIR)/game/%.o: $(GAMESRC)/%.c | objdir
	$(call compile_cc,$(GAME_CFLAGS))

$(OBJDIR)/game/%.o: $(GAMESRC)/%.cpp | objdir
	$(call compile_cxx,$(GAME_CXXFLAGS))

# code from a-c.c is important for performance
$(OBJDIR)/jfbuild/a-c.o: CFLAGS := $(filter-out -mthumb,$(CFLAGS)) -O2

$(OBJDIR)/jfbuild/%.o: $(ENGINEROOT)/src/%.c | objdir
	$(call compile_cc,$(GAME_CFLAGS))

$(OBJDIR)/jfmact/%.o: $(MACTROOT)/%.c | objdir
	$(call compile_cc,$(GAME_CFLAGS))

