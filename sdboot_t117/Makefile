NAME = sdboot_t117
TWO_STAGE = -1
LIBC_SDIO = 3

OBJS = $(patsubst %,$(OBJDIR)/app/%.o,start entry main)
OBJS += $(patsubst %,$(OBJDIR)/sys/%.o,asmcode usbio common libc syscomm syscode sdio microfat sdram_init)

include ../build_t117.make

LDSCRIPT = $(SYSDIR_T117)/ums9117_fdl1.ld

CFLAGS += -DNO_ADI_LOG=1
CFLAGS += -DSDIO_NO_PINMAP=1
ifeq ($(LIBC_SDIO), 3)
CFLAGS += -DSDIO_VERBOSE=0 -DSDIO_SHL=CHIPRAM_ADDR
endif
CFLAGS := $(filter-out -DFAT_WRITE=1, $(CFLAGS))

APP_CFLAGS += -I$(SYSDIR_T117) -I$(SYSDIR)

$(OBJDIR)/$(NAME).elf: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

%.bin: $(OBJDIR)/%.elf
	$(OBJCOPY) -O binary -j .text $< $@

