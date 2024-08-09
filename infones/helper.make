
InfoNES_hash = 363bac8bbb030c3d3708ab32cd719ae6b2919971
InfoNES_list = src/*

.PHONY: all patch gitinit diff
all: InfoNES

InfoNES.zip:
	wget -O $@ "https://github.com/jay-kumogata/InfoNES/archive/$(InfoNES_hash).zip"

%: %.zip
	name="$@-$($@_hash)"; unzip -q $< $(patsubst %,"$$name/%",$($@_list)) && mv $$name $@

patch: InfoNES
	patch -p1 -d InfoNES < InfoNES.patch

gitinit: InfoNES
	cd InfoNES && git init && git add . && git commit -m "init"

diff: InfoNES/.git
	cd InfoNES && git add . && git diff --staged > ../InfoNES.patch2

