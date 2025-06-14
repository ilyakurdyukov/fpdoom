
ZIPDIR = .
chocolate_doom_hash = 0b3cb528c3f53c61d7a4ebe13a7d522570b98d83
chocolate_doom_list = src/* textscreen/*
chocolate_doom_DIR = chocolate-doom

.PHONY: all patch gitinit diff
all: $(chocolate_doom_DIR)
download: $(ZIPDIR)/chocolate-doom.zip

$(ZIPDIR)/chocolate-doom.zip:
	wget -O $@ "https://github.com/chocolate-doom/chocolate-doom/archive/$(chocolate_doom_hash).zip"

$(chocolate_doom_DIR): $(ZIPDIR)/chocolate-doom.zip
	name="chocolate-doom-$(chocolate_doom_hash)"; unzip -q $< $(patsubst %,"$$name/%",$(chocolate_doom_list)) && mv $$name $@

patch: $(chocolate_doom_DIR)
	patch -p1 -d $(chocolate_doom_DIR) < chocolate-doom.patch

gitinit: $(chocolate_doom_DIR)
	cd $(chocolate_doom_DIR) && git init && git add . && git commit -m "init"

diff: $(chocolate_doom_DIR)/.git
	cd $(chocolate_doom_DIR) && git diff > chocolate-doom.patch2

