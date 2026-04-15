NAME = retris
SRC = retris
KEYTRN_PC = 1
TILES = tiles.pgm

APP_SRCS = keytrn scr_update
GAME_SRCS = retris

APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(GAME_SRCS:%=$(OBJDIR)/game/%.o)

include ../buildinc.make

APP_CFLAGS += -std=c99 -pedantic
APP_CFLAGS += -I$(SYSDIR)

GAME_CFLAGS := $(APP_CFLAGS)
GAME_CFLAGS += -DUSE_CURTAIN=0 -DUSE_GAMEPAD=0

$(OBJDIR)/app/scr_update.o: $(OBJDIR)/tiles.h
$(OBJDIR)/app/scr_update.o: CFLAGS += -I $(OBJDIR)

$(OBJDIR)/calctables: calctables.c resize.h | $(OBJDIR)
	$(HOSTCC) -O2 -std=c99 $< -o $@ -lm

$(OBJDIR)/tiles.h: $(OBJDIR)/calctables $(TILES)
	$(OBJDIR)/calctables $(TILES) > $@

$(OBJDIR)/game/%.o: $(SRC)/%.c | objdir
	$(call compile_cc,$(GAME_CFLAGS))


