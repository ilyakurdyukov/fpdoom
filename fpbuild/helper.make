
jfbuild_hash = efd88d9cc24f753038a28479c9d8e7ac398909c8
jfmact_hash = 1f0746a3b9704906669d8aaed2bbb982053a393e
jfduke3d_hash = 41cd46bc00633e7457d07d88c8add9f99a7d9d41
jfsw_hash = 1282878348bff97c5cf92401c1253f81da290cc4

jfbuild_list = src/* include/*
jfmact_list = *
jfduke3d_list = src/*
jfsw_list = src/*

.PHONY: all patch gitinit diff
all: jfbuild jfmact jfduke3d jfsw

.PRECIOUS: jf%.zip
jf%.zip:
	wget -O $@ "https://github.com/jonof/$(@:%.zip=%)/archive/$($(@:%.zip=%)_hash).zip"

%: %.zip
	name="$@-$($@_hash)"; unzip -q $< $(patsubst %,"$$name/%",$($@_list)) && mv $$name $@

for_fn = \
	for n in jfbuild jfmact jfduke3d jfsw; do \
		test ! -d $$n || ($(1);); \
	done

patch:
	$(call for_fn, patch -p1 -d $$n < $$n.patch)

gitinit:
	$(call for_fn, cd $$n && git init && git add . && git commit -m "init")

diff:
	$(call for_fn, cd $$n && git diff > ../$$n.patch2)

