NAME = gnuboy
GNUBOYDIR = gnuboy
USE_FILEMAP = 1
USE_ASM = 1
NO_FLOAT = 1
NO_GZ = 1
NO_XZ = 1
NO_ZIP = 1
NO_MENU = 1
NO_DEH = 1

APP_SRCS = port_hacks port_sys scr_update
ifneq ($(USE_ASM), 0)
APP_SRCS += gbasm
endif

GNUBOY_SRCS = lcd refresh lcdc palette cpu mem rtc hw sound \
	events keytable loader save debug emu main \
	rccmds rckeys rcvars rcfile exports \
	split path

ifeq ($(NO_NOMENU), 0)
GNUBOY_SRCS += menu
endif
ifeq ($(NO_GZ), 0)
GNUBOY_SRCS += inflate
endif
ifeq ($(NO_XZ), 0)
GNUBOY_SRCS += xz_crc32 xz_crc64 xz_dec_lzma2 xz_dec_stream xz_dec_bcj
endif
ifeq ($(NO_ZIP), 0)
GNUBOY_SRCS += miniz_tinfl
endif

APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(GNUBOY_SRCS:%=$(OBJDIR)/gnuboy/%.o)

include ../buildinc.make

CFLAGS := $(filter-out -funsigned-char, $(CFLAGS)) -fsigned-char
SYS_CFLAGS += -DATEXIT_MAX=2

APP_CFLAGS += -std=c99 -pedantic
GNUBOY_CFLAGS = -DIS_LITTLE_ENDIAN
ifneq ($(NO_MENU), 0)
GNUBOY_CFLAGS += -DNO_MENU
endif
ifneq ($(NO_GZ), 0)
GNUBOY_CFLAGS += -DNO_GZ
endif
ifneq ($(NO_XZ), 0)
GNUBOY_CFLAGS += -DNO_XZ
endif
ifneq ($(NO_ZIP), 0)
GNUBOY_CFLAGS += -DNO_ZIP
endif
ifneq ($(NO_FLOAT), 0)
GNUBOY_CFLAGS += -DNO_FLOAT=1
endif
GNUBOY_CFLAGS += -Wno-unused -Wno-sign-compare
GNUBOY_CFLAGS += -Wno-unused-parameter -Wno-strict-prototypes -Wno-newline-eof
GNUBOY_CFLAGS := $(APP_CFLAGS) $(GNUBOY_CFLAGS)

APP_CFLAGS := $(GNUBOY_CFLAGS) -I$(SYSDIR) -I$(GNUBOYDIR)
ifneq ($(USE_ASM), 0)
APP_CFLAGS += -DUSE_ASM
endif

$(OBJDIR)/gnuboy/%.o: $(GNUBOYDIR)/%.c | objdir
	$(call compile_cc,$(GNUBOY_CFLAGS))

