
export

BINDIR = release
ZIPDIR = ../zipdir

ifneq ($(words $(BINDIR)), 1)
$(error BINDIR must not contain spaces in the name)
endif
ifneq ($(words $(ZIPDIR)), 1)
$(error ZIPDIR must not contain spaces in the name)
endif

FPBIN = $(BINDIR)/sdcard/fpbin
FPBIN_T117 = $(BINDIR)/sdcard/fpbin_t117
APPS = fpdoom fpduke3d fpsw fpblood infones \
	wolf3d wolf3d_sw snes9x snes9x_16bit \
	chocolate-doom chocolate-heretic chocolate-hexen \
	gnuboy
BINS = \
	$(patsubst %,$(BINDIR)/usb/%.bin,fptest $(APPS)) \
	$(patsubst %,$(BINDIR)/usb_t117/%.bin,fptest $(APPS)) \
	$(patsubst %,$(FPBIN)/%.bin,fpmain $(APPS)) $(FPBIN)/config.txt \
	$(patsubst %,$(FPBIN_T117)/%.bin,fpmain $(APPS)) $(FPBIN_T117)/config.txt \
	$(patsubst %,$(BINDIR)/sdboot%.bin,1 2 3 _t117) $(BINDIR)/jump4m.bin \

.PHONY: all clean
all: $(BINS)

clean:
	$(RM) $(BINS)

define getsrc
	test -d $@ || ( cd $(1) && mkdir -p $(ZIPDIR) && $(MAKE) -f helper.make ZIPDIR="$(ZIPDIR)" all patch )
endef

doom_src:
	$(call getsrc,fpdoom)

fpbuild/jfbuild:
	$(call getsrc,fpbuild)

infones/InfoNES:
	$(call getsrc,infones)

wolf3d/Wolf4SDL:
	$(call getsrc,wolf3d)

snes9x/snes9x_src:
	$(call getsrc,snes9x)

chocolate-doom/chocolate-doom:
	$(call getsrc,chocolate-doom)

gnuboy/gnuboy:
	$(call getsrc,gnuboy)

define makebin
	cd $(1) && $(MAKE) clean $(2)
	cd $(1) && $(MAKE) all $(2)
	mkdir -p $(dir $@)
	mv $(1)/$(notdir $@) $@
	cd $(1) && $(MAKE) clean $(2)
endef

# PCTEST build

$(BINDIR)/sdl1/%: OPTS += PCTEST=1 GFX_MODE=SDL1
$(BINDIR)/sdl2/%: OPTS += PCTEST=1 GFX_MODE=SDL2

# USB mode

$(BINDIR)/usb/%: OPTS += LIBC_SDIO=0
$(BINDIR)/usb_t117/%: OPTS += T117=1 LIBC_SDIO=0

# SD card mode

$(FPBIN)/%: OPTS += LIBC_SDIO=3
$(FPBIN_T117)/%: OPTS += T117=1 LIBC_SDIO=3

$(BINDIR)/sdboot1.bin:
	$(call makebin,sdboot,CHIP=1)

$(BINDIR)/sdboot2.bin:
	$(call makebin,sdboot,CHIP=2)

$(BINDIR)/sdboot3.bin:
	$(call makebin,sdboot,CHIP=3)

$(BINDIR)/jump4m.bin:
	$(call makebin,sdboot,NAME=jump4m)

$(BINDIR)/sdboot_t117.bin:
	$(call makebin,sdboot_t117,)

$(FPBIN)/config.txt:
	mkdir -p $(dir $@)
	cp fpmenu/$(notdir $@) $@

$(FPBIN_T117)/config.txt:
	mkdir -p $(dir $@)
	sed 's|fpbin/|fpbin_t117/|' fpmenu/$(notdir $@) > $@

%/fpmenu.bin:
	$(call makebin,fpmenu,$(OPTS))

%/fpmain.bin:
	$(call makebin,fpmenu,$(OPTS) NAME=fpmain)

%/fptest.bin:
	$(call makebin,fptest,$(OPTS))

%/fpdoom.bin: doom_src
	$(call makebin,fpdoom,$(OPTS))

%/fpduke3d.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,$(OPTS) GAME=duke3d)

%/fpsw.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,$(OPTS) GAME=sw)

%/fpblood.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,$(OPTS) GAME=blood)

%/infones.bin: infones/InfoNES
	$(call makebin,infones,$(OPTS))

%/wolf3d.bin: wolf3d/Wolf4SDL
	$(call makebin,wolf3d,$(OPTS))

%/wolf3d_sw.bin: wolf3d/Wolf4SDL
	$(call makebin,wolf3d,$(OPTS) NAME=wolf3d_sw)

%/snes9x.bin: snes9x/snes9x_src
	$(call makebin,snes9x,$(OPTS))

%/snes9x_16bit.bin: snes9x/snes9x_src
	$(call makebin,snes9x,$(OPTS) NAME=snes9x_16bit)

%/chocolate-doom.bin: chocolate-doom/chocolate-doom
	$(call makebin,chocolate-doom,$(OPTS) GAME=doom)

%/chocolate-heretic.bin: chocolate-doom/chocolate-doom
	$(call makebin,chocolate-doom,$(OPTS) GAME=heretic)

%/chocolate-hexen.bin: chocolate-doom/chocolate-doom
	$(call makebin,chocolate-doom,$(OPTS) GAME=hexen)

%/gnuboy.bin: gnuboy/gnuboy
	$(call makebin,gnuboy,$(OPTS))

