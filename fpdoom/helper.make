
ZIPDIR = .
DOOM_hash = a77dfb96cb91780ca334d0d4cfd86957558007e0
DOOM_list = linuxdoom-1.10/*
DOOM_DIR = ../doom_src

.PHONY: all patch gitinit diff
all: $(DOOM_DIR)
download: $(ZIPDIR)/DOOM.zip

$(ZIPDIR)/DOOM.zip:
	wget -O $@ "https://github.com/id-Software/DOOM/archive/$(DOOM_hash).zip"

$(DOOM_DIR): $(ZIPDIR)/DOOM.zip
	name="DOOM-$(DOOM_hash)"; unzip -q $< $(patsubst %,"$$name/%",$(DOOM_list)) && mv $$name/linuxdoom-1.10 $@; $(RM) -d $$name

patch: $(DOOM_DIR)
	patch -p1 -d $(DOOM_DIR) < ../doom.patch

gitinit: $(DOOM_DIR)
	cd $(DOOM_DIR) && git init && git add . && git commit -m "init"

diff: $(DOOM_DIR)/.git
	cd $(DOOM_DIR) && git diff > ../doom.patch2

