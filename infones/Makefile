NAME = infones
INFONES = InfoNES/src
USE_ASM = 1

APP_SRCS = InfoNES_System scr_update
ifneq ($(USE_ASM), 0)
APP_SRCS += nesasm
endif
NES_SRCS = K6502 InfoNES InfoNES_Mapper InfoNES_pAPU

APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(NES_SRCS:%=$(OBJDIR)/nes/%.o)

include ../buildinc.make

APP_CFLAGS += -std=c99 -pedantic
APP_CFLAGS += -Wno-strict-prototypes

NES_CFLAGS := $(APP_CFLAGS) -Wno-unused-parameter -Wno-extra-semi
NES_CFLAGS += -DAPU_Mute=1

APP_CFLAGS += -I$(SYSDIR) -I$(INFONES)
ifneq ($(USE_ASM), 0)
APP_CFLAGS += -DUSE_ASM
endif

$(OBJDIR)/nes/%.o: $(INFONES)/%.c | objdir
	$(call compile_cc,$(NES_CFLAGS))


