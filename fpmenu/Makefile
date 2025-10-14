NAME = fpmenu
TWO_STAGE = -1

ifeq ($(PCTEST), 1)
APP_SRCS = main
APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 =
include ../buildinc.make
APP_CFLAGS += -std=c99 -pedantic
APP_CFLAGS += -I$(SYSDIR)
APP_CFLAGS += -DMENU_DEBUG=1
else

SYS_EXTRA = microfat sdio

APP_SRCS = main
ifeq ($(T117), 1)
EXTRA_SRCS = start3_t117 readbin
else
EXTRA_SRCS = start3 readbin
endif
APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 = $(EXTRA_SRCS:%=$(OBJDIR)/app/%.o)

SYS_SRCS1 = start entry $(SYS_SRCS)
SYS_SRCS2 = asmcode common
OBJS1 = $(SYS_SRCS1:%=$(OBJDIR)/sys/%.o) $(APP_OBJS1)
OBJS2 = $(SYS_SRCS2:%=$(OBJDIR)/sys/%.o) $(APP_OBJS2)
OBJS = $(OBJS1) $(OBJS2)

include ../buildinc.make

CFLAGS += -DSDIO_VERBOSE=0 -DSDIO_SHL=CHIPRAM_ADDR
CFLAGS += -DMENU_DEBUG=0
CFLAGS := $(filter-out -DFAT_WRITE=1, $(CFLAGS))

APP_CFLAGS += -std=c99 -pedantic -I$(SYSDIR)

$(OBJDIR)/$(NAME)_part1.elf: $(OBJS1)
	$(CC) $(LFLAGS) $(OBJS1) -o $@

$(OBJDIR)/$(NAME)_part2.elf: $(OBJS2)
	$(CC) $(LFLAGS) $(OBJS2) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary -j .text $< $@

$(NAME).bin: $(patsubst %,$(OBJDIR)/$(NAME)_part%,2.bin 1.bin 1.rel)
	cat $^ > $@

endif
