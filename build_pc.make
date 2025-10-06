OBJDIR ?= obj
SYSDIR = ../pctest
HOSTCC = cc
NM = nm

BITS = 0
ASAN = 1
GFX_MODE = SDL2
KEYTRN_PC ?= 0

USE_ASM = 0
APP_OBJS1 := $(filter-out %asm.o, $(APP_OBJS1))

ifeq ($(KEYTRN_PC), 1)
APP_OBJS1 += $(OBJDIR)/app/keytrn_pc.o
endif

SYS_SRCS += syscode
OBJS = $(SYS_SRCS:%=$(OBJDIR)/sys/%.o) $(APP_OBJS1) $(APP_OBJS2)

APP_CFLAGS += -DEMBEDDED=1
CFLAGS = -O0 -Wall -Wextra -Wno-unused -Wno-sign-compare
CFLAGS += -ffunction-sections -fdata-sections
LFLAGS += -Wl,--gc-sections
LFLAGS += -Wl,--wrap=main
ifneq ($(BITS), 0)
CFLAGS += -m$(BITS)
LFLAGS += -m$(BITS)
endif
ifeq ($(USE_FILEMAP), 1)
# unsupported
#CFLAGS += -DAPP_MEM_RESERVE -DAPP_DATA_EXCEPT
endif

ifeq ($(GFX_MODE), SDL1)
GFX_LIB = -lSDL
GFX_INC = -I/usr/include/SDL
else ifeq ($(GFX_MODE), SDL2)
GFX_LIB = -lSDL2
GFX_INC = -I/usr/include/SDL2
else
$(error unknown GFX_MODE)
endif

LFLAGS += $(GFX_LIB)

ifneq ($(ASAN), 0)
CFLAGS += -fsanitize=address -static-libasan
LFLAGS += -fsanitize=address -static-libasan
endif

ifneq ($(NO_FLOAT), 0)
CFLAGS += -mgeneral-regs-only
else
LFLAGS += -lm
endif

SYS_CFLAGS = -std=gnu99 -pedantic
ifeq ($(KEYTRN_PC), 1)
SYS_CFLAGS += -DNO_SYSEVENT=1
endif

.PHONY: all clean map objdir

all: $(NAME).bin
map: $(OBJDIR)/$(NAME)_map.txt

clean:
	$(RM) -r $(OBJDIR) $(NAME).bin

objdir:
	mkdir -p $(sort $(dir $(OBJS)))

-include $(OBJS:.o=.d)

compile_cc = $(CC) $(CFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@
compile_asm = $(CC) $(1) $< -c -o $@
compile_cxx = $(CXX) $(CXXFLAGS) $(1) -MMD -MP -MF $(@:.o=.d) $< -c -o $@

$(OBJDIR)/app/keytrn_pc.o: CFLAGS += $(GFX_INC)

$(OBJDIR)/app/%.o: %.c | objdir
	$(call compile_cc,$(APP_CFLAGS))

$(OBJDIR)/sys/%.o: $(SYSDIR)/%.c | objdir
	$(call compile_cc,$(SYS_CFLAGS) -I$(SYSDIR) $(GFX_INC))

$(NAME).bin: $(OBJS)
	$(CC) $^ -o $@ $(LFLAGS)

$(OBJDIR)/$(NAME)_map.txt: $(NAME)
	$(NM) -n $< > $@

