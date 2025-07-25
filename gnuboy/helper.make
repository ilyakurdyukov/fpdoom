
ZIPDIR = .
gnuboy_hash = c367bb4ba96fb07cd62f72f5ecb43aeff7012564
gnuboy_list = *
gnuboy_DIR = gnuboy

.PHONY: all patch gitinit diff
all: $(gnuboy_DIR)
download: $(ZIPDIR)/gnuboy.zip

$(ZIPDIR)/gnuboy.zip:
	wget -O $@ "https://github.com/rofl0r/gnuboy/archive/$(gnuboy_hash).zip"

$(gnuboy_DIR): $(ZIPDIR)/gnuboy.zip
	name="gnuboy-$(gnuboy_hash)"; unzip -q $< $(patsubst %,"$$name/%",$($@_list)) && mv $$name $@

patch: $(gnuboy_DIR)
	patch -p1 -d $(gnuboy_DIR) < gnuboy.patch

gitinit: $(gnuboy_DIR)
	cd $(gnuboy_DIR) && git init && git add . && git commit -m "init"

diff: $(gnuboy_DIR)/.git
	cd $(gnuboy_DIR) && git diff > gnuboy.patch2

