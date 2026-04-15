
ZIPDIR = .
retris_hash = b81fc06381fc648ed8cb491c2018c5dd009c20c3
retris_list = *
retris_DIR = retris

.PHONY: all patch gitinit diff
all: $(retris_DIR)
download: $(ZIPDIR)/retris.zip

$(ZIPDIR)/retris.zip:
	wget -O $@ "https://github.com/ilyakurdyukov/retris/archive/$(retris_hash).zip"

$(retris_DIR): $(ZIPDIR)/retris.zip
	name="retris-$(retris_hash)"; unzip -q $< $(patsubst %,"$$name/%",$($@_list)) && mv $$name $@

patch: $(retris_DIR)
	patch -p1 -d $(retris_DIR) < retris.patch

gitinit: $(retris_DIR)
	cd $(retris_DIR) && git init && git add . && git commit -m "init"

diff: $(retris_DIR)/.git
	cd $(retris_DIR) && git diff > retris.patch2

