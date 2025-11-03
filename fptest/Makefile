NAME = fptest
TWO_STAGE = 0

SYS_EXTRA = sdio microfat sfc

APP_SRCS = main
APP_OBJS1 = $(APP_SRCS:%=$(OBJDIR)/app/%.o)
APP_OBJS2 =

include ../buildinc.make

#CFLAGS += -DSDIO_VERBOSE=2

APP_CFLAGS += -std=c99 -pedantic
APP_CFLAGS += -I$(SYSDIR)
