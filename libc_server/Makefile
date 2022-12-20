
LIBUSB = 1
CFLAGS = -O2 -Wall -Wextra -std=c99 -pedantic -Wno-unused
CFLAGS += -DUSE_LIBUSB=$(LIBUSB)
APPNAME = libc_server

ifeq ($(LIBUSB), 1)
LIBS = -lusb-1.0
endif

.PHONY: all clean
all: $(APPNAME)

clean:
	$(RM) $(APPNAME)

$(APPNAME): $(APPNAME).c cmd_def.h
	$(CC) -s $(CFLAGS) -o $@ $^ $(LIBS)
