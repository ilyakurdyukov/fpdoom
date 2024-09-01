
ZIPDIR = .
Wolf4SDL_hash = dc8b250af35fb0ace68db5eb879490b50068c20e
Wolf4SDL_list = *

.PHONY: all patch gitinit diff
all: Wolf4SDL
download: $(ZIPDIR)/Wolf4SDL.zip

$(ZIPDIR)/Wolf4SDL.zip:
	wget -O $@ "https://github.com/KS-Presto/Wolf4SDL/archive/$(Wolf4SDL_hash).zip"

%: $(ZIPDIR)/%.zip
	name="$@-$($@_hash)"; unzip -q $< $(patsubst %,"$$name/%",$($@_list)) && mv $$name $@

patch: Wolf4SDL
	patch -p1 -d Wolf4SDL < wolf3d.patch

gitinit: Wolf4SDL
	cd Wolf4SDL && git init && git add . && git commit -m "init"

diff: Wolf4SDL/.git
	cd Wolf4SDL && git diff > ../wolf3d.patch2

