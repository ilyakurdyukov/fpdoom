# 0 - detect, 1 - SC6531E, 2 - SC6531DA, 3 - SC6530
CHIP = 0
TWO_STAGE = 1
LIBC_SDIO = 0
USE_ASM = 1
NO_FLOAT = 1
PACK_RELOC = ../pack_reloc/pack_reloc
SRCS = asmcode usbio common libc syscomm
ifeq ($(CHIP), 0)
SRCS += chip1_syscode chip2_syscode
else
SRCS += syscode
endif
ifneq ($(LIBC_SDIO), 0)
SRCS += sdio microfat
endif

GAME = duke3d
NAME = fp$(GAME)
GAMESRC = jf$(GAME)/src
OBJDIR = obj$(CHIP)_$(GAME)
ENGINEROOT = jfbuild
MACTROOT = jfmact
AUDIOLIBROOT = jfaudiolib
FPDOOM = ../fpdoom
HOSTCC = cc
NM = nm

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

SRCS += fp_layer unistd
ifeq ($(USE_ASM), 0)
BUILD_SRCS += a-c
else
SRCS += buildasm a-new
endif

GAME_OBJS = $(GAME_SRCS:%=$(OBJDIR)/game/%.o)
GAME_OBJS += $(BUILD_SRCS:%=$(OBJDIR)/jfbuild/%.o)
GAME_OBJS += $(MACT_SRCS:%=$(OBJDIR)/jfmact/%.o)

ifeq ($(TWO_STAGE), 0)
SRCS := start entry $(SRCS)
OBJS = $(SRCS:%=$(OBJDIR)/sys/%.o) $(GAME_OBJS)
else
SRCS1 = start1 entry $(SRCS)
SRCS2 = start2 entry2 $(SRCS)
OBJS1 = $(SRCS1:%=$(OBJDIR)/sys/%.o)
OBJS2 = $(SRCS2:%=$(OBJDIR)/sys/%.o) $(GAME_OBJS)
endif

LDSCRIPT = $(FPDOOM)/sc6531e_fdl.ld

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

CXXFLAGS := $(CFLAGS) -std=c++14 -fno-exceptions -fno-rtti -Wno-narrowing
CFLAGS += -std=c99 -pedantic

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

.PHONY: all clean map

all: $(NAME).bin map
map: $(OBJDIR)/$(NAME)_map.txt

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

$(OBJDIR):
	mkdir -p $@/sys $@/game $@/jfbuild $@/jfmact

%.asm: %.c
	$(CC) $(CFLAGS) $< -S -o $@

ifneq ($(NO_FLOAT), 0)
$(OBJDIR)/jfbuild/engine.o: $(OBJDIR)/engine_tables.h

$(OBJDIR)/calctables: calctables.c | $(OBJDIR)
	$(HOSTCC) -O2 $< -o $@ -lm

$(OBJDIR)/engine_tables.h: $(OBJDIR)/calctables
	$(OBJDIR)/calctables $@
endif

-include $(OBJS:.o=.d)

compile_fn = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/game/%.o: $(GAMESRC)/%.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS))

$(OBJDIR)/game/%.o: $(GAMESRC)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(GAME_CFLAGS) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

# code from a-c.c is important for performance
$(OBJDIR)/jfbuild/a-c.o: CFLAGS := $(filter-out -mthumb,$(CFLAGS)) -O2

$(OBJDIR)/jfbuild/%.o: $(ENGINEROOT)/src/%.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS))

$(OBJDIR)/jfmact/%.o: $(MACTROOT)/%.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS))

$(OBJDIR)/sys/%.o: %.c | $(OBJDIR)
	$(call compile_fn,$(GAME_CFLAGS) -I$(ENGINEROOT)/src -I$(FPDOOM))

$(OBJDIR)/sys/%.o: $(FPDOOM)/%.c | $(OBJDIR)
ifeq ($(GAME), blood)
	$(call compile_fn,-DCXX_SUPPORT)
else
	$(call compile_fn,)
endif

$(OBJDIR)/sys/chip1_%.o: $(FPDOOM)/%.c | $(OBJDIR)
	$(call compile_fn,-DCHIP=1 -DCHIP_AUTO)

$(OBJDIR)/sys/chip2_%.o: $(FPDOOM)/%.c | $(OBJDIR)
	$(call compile_fn,-DCHIP=2 -DCHIP_AUTO)

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

