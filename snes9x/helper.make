
ZIPDIR = .
LINK_IDX = 2
LINK0 = https://www.lysator.liu.se/snes9x/1.43/snes9x-1.43-src.tar.gz
LINK1 = http://ftp.lip6.fr/pub/minix/distfiles/backup/snes9x-1.43-src.tar.gz
CHECKSUM = cb60baaeabc28b68f7dfc8fd54453f6268b66aae33ea64eb1788c19df09be6f1
LINK2 = https://old-releases.ubuntu.com/ubuntu/pool/multiverse/s/snes9x/snes9x_1.43.orig.tar.gz
CHECKSUM2 = c1afb7804ca27af5ee580d4a45a197241bbb13b8d321ee1bf8b3abf516ff7ec1
snes9x_list = snes9x-1.43-src/snes9x/*

.PHONY: all patch gitinit diff
all: snes9x_src snes9x_src/dsp1new.cpp

LINK = $(LINK$(LINK_IDX))
ARCHIVE = $(notdir $(LINK))
ifeq ($(LINK_IDX),2)
CHECKSUM = $(CHECKSUM2)
snes9x_src: $(ZIPDIR)/$(ARCHIVE)
	mkdir -p $@
	tar -xzf $< -C $@ --strip-components 3 --wildcards "snes9x-1.43.orig/$(snes9x_list)"
else
snes9x_src: $(ZIPDIR)/$(ARCHIVE)
	mkdir -p $@
	tar -xzf $< -C $@ --strip-components 2 --wildcards "$(snes9x_list)"
endif
download: $(ZIPDIR)/$(ARCHIVE)
$(ZIPDIR)/$(ARCHIVE):
	wget -O $@.tmp $(LINK)
	test "$$(openssl sha256 $@.tmp | cut -d " " -f 2)" = $(CHECKSUM)
	mv $@.tmp $@

$(ZIPDIR)/snes9x_dsp1.cpp:
	wget -O $@ https://raw.githubusercontent.com/snes9xgit/snes9x/96059dd45aed03859bff5a3e30f1d1b13136a8f9/dsp1.cpp
snes9x_src/dsp1new.cpp: $(ZIPDIR)/snes9x_dsp1.cpp | snes9x_src
	cp $< $@

patch: snes9x_src
	patch -p1 -d snes9x_src < snes9x.patch

gitinit: snes9x_src
	cd snes9x_src && git init && git add . && git commit -m "init"

diff: snes9x_src/.git
	cd snes9x_src && git diff > ../snes9x.patch2

