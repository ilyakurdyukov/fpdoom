USE_16BIT := $(if $(filter %_16bit,$(NAME)),1,0)
NAME := snes9x$(if $(filter 0,$(USE_16BIT)),,_16bit)
SNES9X = snes9x_src
USE_FILEMAP = 1
USE_ASM = 1
NO_FLOAT = 1
DSP1_NEW = 1
NO_SA1 = 1
NO_C4 = $(NO_FLOAT)
NO_DSP1 = $(if $(filter 1,$(DSP1_NEW)),0,$(NO_FLOAT))
NO_DSP4 = $(NO_FLOAT)

APP_SRCS = snes9x_fp extra scr_update
ifneq ($(USE_ASM), 0)
APP_SRCS += snesasm
endif

SNES_SRCS = apu cheats2 \
	clip cpu cpuexec cpuops data dma dsp1 \
	fxemu fxinst gfx globals memmap \
	obc1 ppu sdd1 seta seta010 seta011 seta018 \
	snapshot soundux spc700 spc7110 srtc tile

ifeq ($(NO_SA1), 0)
SNES_SRCS += sa1 sa1cpu
endif
ifeq ($(NO_C4), 0)
SNES_SRCS += c4 c4emu
endif

APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(SNES_SRCS:%=$(OBJDIR)/snes/%.o)

include ../buildinc.make

SYS_CFLAGS += -DCXX_SUPPORT
CFLAGS := $(filter-out -funsigned-char, $(CFLAGS)) -fsigned-char
CXXFLAGS := $(CFLAGS) -std=c++14 -fno-exceptions -fno-rtti -Wno-narrowing

APP_CFLAGS += -DHAVE_STRINGS_H -DHAVE_STDINT_H -DRIGHTSHIFT_IS_SAR
APP_CFLAGS += -DNOSOUND -DNDEBUG
APP_CFLAGS += -DUSE_16BIT=$(USE_16BIT)
APP_CFLAGS += -DVAR_CYCLES -DCPU_SHUTDOWN -DSPC700_C
#APP_CFLAGS += -DSPC700_SHUTDOWN
APP_CFLAGS += -I$(SNES9X) -I$(SNES9X)/unzip
ifneq ($(USE_ASM), 0)
APP_CFLAGS += -DUSE_ASM
endif
ifneq ($(NO_FLOAT), 0)
APP_CFLAGS += -DNO_FLOAT=1
endif
ifneq ($(NO_SA1), 0)
APP_CFLAGS += -DNO_SA1=1
endif
ifneq ($(NO_C4), 0)
APP_CFLAGS += -DNO_C4=1
endif
ifneq ($(NO_DSP1), 0)
APP_CFLAGS += -DNO_DSP1=1
else ifneq ($(DSP1_NEW), 0)
APP_CFLAGS += -DDSP1_NEW=1
endif
ifneq ($(NO_DSP4), 0)
APP_CFLAGS += -DNO_DSP4=1
endif
APP_CFLAGS += -Wno-unused -Wno-unused-parameter
APP_CFLAGS += -Wno-deprecated-register -Wno-null-pointer-subtraction
APP_CFLAGS += -Wno-misleading-indentation
APP_CFLAGS += -I$(SYSDIR)
APP_CXXFLAGS := $(APP_CFLAGS)
APP_CFLAGS += -std=c99 -pedantic

$(OBJDIR)/app/%.o: %.cpp | objdir
	$(call compile_cxx,$(APP_CXXFLAGS))

$(OBJDIR)/snes/%.o: $(SNES9X)/%.cpp | objdir
	$(call compile_cxx,$(APP_CXXFLAGS))

