CHIP = 0
NAME := sdboot$(if $(filter 0,$(CHIP)),,$(CHIP))
TWO_STAGE = -1
LIBC_SDIO = 3

OBJS = $(patsubst %,$(OBJDIR)/app/%.o,start entry)
OBJS += $(patsubst %,$(OBJDIR)/sys/%.o,sdio microfat)
OBJS += $(OBJDIR)/app/main.o
OBJS += $(patsubst %,$(OBJDIR)/sys/%.o,asmcode usbio common libc syscomm syscode)

include ../build_sc6531.make

LDSCRIPT = $(SYSDIR)/sc6531e_fdl1.ld

CFLAGS += -DSDIO_NO_PINMAP=1
ifeq ($(LIBC_SDIO), 3)
CFLAGS += -DSDIO_VERBOSE=0 -DSDIO_SHL=CHIPRAM_ADDR
endif
CFLAGS := $(filter-out -DFAT_WRITE=1, $(CFLAGS))
CFLAGS += -DSDBOOTKEY=-1

APP_CFLAGS += -I$(SYSDIR)

$(OBJDIR)/$(NAME).elf: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

jump4m.bin: $(OBJDIR)/app/jump4m.o
	$(OBJCOPY) -O binary $< $@

%.bin: $(OBJDIR)/%.elf
	$(OBJCOPY) -O binary -j .text $< $@

