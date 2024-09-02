
export

BINDIR = release
ZIPDIR = zipdir

ifneq ($(words $(BINDIR)), 1)
$(error BINDIR must not contain spaces in the name)
endif
ifneq ($(words $(ZIPDIR)), 1)
$(error ZIPDIR must not contain spaces in the name)
endif

FPBIN = $(BINDIR)/sdcard/fpbin
APPS = fpdoom fpduke3d fpsw fpblood infones wolf3d wolf3d_sw
BINS = \
	$(patsubst %,$(BINDIR)/usb/%.bin,fptest $(APPS)) \
	$(patsubst %,$(FPBIN)/%.bin,fpmain $(APPS)) \
	$(patsubst %,$(BINDIR)/sdboot%.bin,1 2 3) \
	$(FPBIN)/config.txt

.PHONY: all clean
all: $(BINS)

clean:
	$(RM) $(BINS)

define getsrc
	test -d $@ || ( mkdir -p $(ZIPDIR); cd $(1) && $(MAKE) -f helper.make ZIPDIR="../$(ZIPDIR)" all patch )
endef

doom_src:
	$(call getsrc,fpdoom)

fpbuild/jfbuild:
	$(call getsrc,fpbuild)

infones/InfoNES:
	$(call getsrc,infones)

wolf3d/Wolf4SDL:
	$(call getsrc,wolf3d)

define makebin
	cd $(1) && $(MAKE) clean $(2)
	cd $(1) && $(MAKE) all $(2)
	mkdir -p $(dir $@)
	mv $(1)/$(notdir $@) $@
	cd $(1) && $(MAKE) clean $(2)
endef

# USB mode

$(BINDIR)/usb/fptest.bin:
	$(call makebin,fptest,LIBC_SDIO=0)

$(BINDIR)/usb/fpdoom.bin: doom_src
	$(call makebin,fpdoom,LIBC_SDIO=0)

$(BINDIR)/usb/fpduke3d.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=0 GAME=duke3d)

$(BINDIR)/usb/fpsw.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=0 GAME=sw)

$(BINDIR)/usb/fpblood.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=0 GAME=blood)

$(BINDIR)/usb/infones.bin: infones/InfoNES
	$(call makebin,infones,LIBC_SDIO=0)

$(BINDIR)/usb/wolf3d.bin: wolf3d/Wolf4SDL
	$(call makebin,wolf3d,LIBC_SDIO=0)

$(BINDIR)/usb/wolf3d_sw.bin: wolf3d/Wolf4SDL
	$(call makebin,wolf3d,LIBC_SDIO=0 NAME=wolf3d_sw)

# SD card mode

$(BINDIR)/sdboot1.bin:
	$(call makebin,sdboot,CHIP=1)

$(BINDIR)/sdboot2.bin:
	$(call makebin,sdboot,CHIP=2)

$(BINDIR)/sdboot3.bin:
	$(call makebin,sdboot,CHIP=3)

$(FPBIN)/fpmain.bin:
	$(call makebin,fpmenu,LIBC_SDIO=3 NAME=fpmain)

$(FPBIN)/config.txt:
	mkdir -p $(dir $@)
	cp fpmenu/$(notdir $@) $@

$(FPBIN)/fpdoom.bin: doom_src
	$(call makebin,fpdoom,LIBC_SDIO=3)

$(FPBIN)/fpduke3d.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=3 GAME=duke3d)

$(FPBIN)/fpsw.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=3 GAME=sw)

$(FPBIN)/fpblood.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=3 GAME=blood)

$(FPBIN)/infones.bin: infones/InfoNES
	$(call makebin,infones,LIBC_SDIO=3)

$(FPBIN)/wolf3d.bin: wolf3d/Wolf4SDL
	$(call makebin,wolf3d,LIBC_SDIO=3)

$(FPBIN)/wolf3d_sw.bin: wolf3d/Wolf4SDL
	$(call makebin,wolf3d,LIBC_SDIO=3 NAME=wolf3d_sw)

