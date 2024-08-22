
export

BINDIR = release
ZIPDIR = zipdir

ifneq ($(words $(BINDIR)), 1)
$(error BINDIR must not contain spaces in the name)
endif
ifneq ($(words $(ZIPDIR)), 1)
$(error ZIPDIR must not contain spaces in the name)
endif

BIN_NAMES = usb/fptest sdcard/fpbin/fpmain \
	$(patsubst %,usb/%,fpdoom fpduke3d fpsw fpblood infones) \
	$(patsubst %,sdcard/fpbin/%,fpdoom fpduke3d fpsw fpblood infones) \
	$(patsubst %,sdcard/sdboot%,1 2 3)

BINS = $(patsubst %,$(BINDIR)/%.bin,$(BIN_NAMES))

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

define makebin
	cd $(1) && $(MAKE) clean $(2)
	cd $(1) && $(MAKE) all $(2)
	mkdir -p $(BINDIR)/$(4)
	mv $(1)/$(3).bin $(BINDIR)/$(4)/
	cd $(1) && $(MAKE) clean $(2)
endef

# USB mode

$(BINDIR)/usb/fptest.bin:
	$(call makebin,fptest,LIBC_SDIO=0,fptest,usb)

$(BINDIR)/usb/fpdoom.bin: doom_src
	$(call makebin,fpdoom,LIBC_SDIO=0,fpdoom,usb)

$(BINDIR)/usb/fpduke3d.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=0 GAME=duke3d,fpduke3d,usb)

$(BINDIR)/usb/fpsw.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=0 GAME=sw,fpsw,usb)

$(BINDIR)/usb/fpblood.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=0 GAME=blood,fpblood,usb)

$(BINDIR)/usb/infones.bin: infones/InfoNES
	$(call makebin,infones,LIBC_SDIO=0,infones,usb)

# SD card mode

$(BINDIR)/sdcard/sdboot1.bin:
	$(call makebin,sdboot,CHIP=1,sdboot1,sdcard)

$(BINDIR)/sdcard/sdboot2.bin:
	$(call makebin,sdboot,CHIP=2,sdboot2,sdcard)

$(BINDIR)/sdcard/sdboot3.bin:
	$(call makebin,sdboot,CHIP=3,sdboot3,sdcard)

$(BINDIR)/sdcard/fpbin/fpmain.bin:
	$(call makebin,fpmenu,LIBC_SDIO=3 NAME=fpmain,fpmain,sdcard/fpbin)

$(BINDIR)/sdcard/fpbin/fpdoom.bin: doom_src
	$(call makebin,fpdoom,LIBC_SDIO=3,fpdoom,sdcard/fpbin)

$(BINDIR)/sdcard/fpbin/fpduke3d.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=3 GAME=duke3d,fpduke3d,sdcard/fpbin)

$(BINDIR)/sdcard/fpbin/fpsw.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=3 GAME=sw,fpsw,sdcard/fpbin)

$(BINDIR)/sdcard/fpbin/fpblood.bin: fpbuild/jfbuild
	$(call makebin,fpbuild,LIBC_SDIO=3 GAME=blood,fpblood,sdcard/fpbin)

$(BINDIR)/sdcard/fpbin/infones.bin: infones/InfoNES
	$(call makebin,infones,LIBC_SDIO=3,infones,sdcard/fpbin)

