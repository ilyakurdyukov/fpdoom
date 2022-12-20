
CFLAGS = -O2 -Wall -Wextra -std=c99 -pedantic -Wno-unused
APPNAME = pack_reloc

.PHONY: all clean
all: $(APPNAME)

clean:
	$(RM) $(APPNAME)

$(APPNAME): $(APPNAME).c
	$(CC) -s $(CFLAGS) -o $@ $^
