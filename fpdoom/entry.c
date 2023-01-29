#include "common.h"
#include "cmd_def.h"
#include "usbio.h"
#include "syscode.h"

#if !CHIP || CHIP == 2
#include "init_sc6531da.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void apply_reloc(uint32_t *image, const uint8_t *rel, uint32_t diff);

void _debug_msg(const char *msg);
void _malloc_init(void *addr, size_t size);
void _stdio_init(void);
int _argv_init(char ***argvp, int skip);

int main(int argc, char **argv);

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w1, h = disp->h1;
	switch (w) {
	case 240: case 320:
		w = 320; h = 200; break;
	case 128: case 160:
		w = 160; h = 100; break;
	default:
		fprintf(stderr, "!!! unsupported resolution (%dx%d)\n", w, h);
		exit(1);
	}
	disp->w2 = w;
	disp->h2 = h;
}

static uint32_t get_ram_size(uint32_t addr) {
	/* simple check for 4/8 MB */
	uint32_t size = RAM_SIZE;
	uint32_t v0 = 0, v1 = 0x12345678;
	MEM4(addr) = v0;
	MEM4(addr + size) = v1;
	if (MEM4(addr) == v0 && MEM4(addr + size) == v1)
		size <<= 1;
	return size;
}

#if TWO_STAGE
struct entry2 {
	uint32_t code[3];
	uint32_t image_start;
	uint32_t image_size, bss_size;
	uint32_t entry_main;
};

int entry_main(char *image_addr, uint32_t image_size, uint32_t bss_size, uint8_t *entry2_rel, struct entry2 *entry2) {
#else
void entry_main(char *image_addr, uint32_t image_size, uint32_t bss_size) {
#endif
	static const uint8_t fdl_ack[8] = {
		0x7e, 0, 0x80, 0, 0, 0xff, 0x7f, 0x7e };
	int argc; char **argv = NULL;
	uint32_t fw_addr = (uint32_t)image_addr & 0xf0000000;
	uint32_t ram_addr = fw_addr + 0x04000000, ram_size;
#if TWO_STAGE
	int argc1;
	struct sys_data *sys_data_copy;
#endif

	// SC6531E is initialized by FDL1
#if !CHIP || CHIP == 2
#if !CHIP
	if (fw_addr == 0x30000000) {
#endif
		init_sc6531da();
#if !CHIP
#if !TWO_STAGE
		chip_fn[0] = chip_fn[1];
#endif
	}
#endif
#endif

#if TWO_STAGE
	{
		// must be done before zeroing BSS
		uint32_t diff = (uint32_t)entry2 - entry2->image_start;
		if (diff) apply_reloc((uint32_t*)entry2, entry2_rel, diff);
	}
#endif

	memset(image_addr + image_size, 0, bss_size);

#if !CHIP
	_chip = fw_addr == 0x30000000 ? 2 : 1;
#endif

	ram_size = get_ram_size(ram_addr);

	usb_init();
	usb_write(fdl_ack, sizeof(fdl_ack));

	for (;;) {
		char buf[4];
		usb_read(buf, 1, USB_WAIT);
		if (buf[0] == HOST_CONNECT) break;
	}
#if TWO_STAGE
	_debug_msg("doom_init");
#else
	_debug_msg("doom");
#endif
#if INIT_MMU
	{
		volatile uint32_t *tab = (volatile uint32_t*)(ram_addr + ram_size - 0x4000);
		unsigned i;
		for (i = 0; i < 0x1000; i++)
			tab[i] = i << 20 | 3 << 10 | 0x12;			// section descriptor
		// cached=1, buffered=1
		tab[CHIPRAM_ADDR >> 20] |= 3 << 2;
		for (i = 0; i < (ram_size >> 20); i++)
			tab[ram_addr >> 20 | i] |= 3 << 2;
		// for faster search
		for (i = 0; i < (FIRMWARE_SIZE >> 20); i++)
			tab[fw_addr >> 20 | i] |= 3 << 2;
		enable_mmu((uint32_t*)tab, -1);
	}
#endif
#if INIT_MMU
	ram_size -= 0x4000; // MMU table
#endif
	{
		char *addr = image_addr + image_size + bss_size;
		size_t size = ram_addr + ram_size - (uint32_t)addr;
#if TWO_STAGE
		// reserve space to save sys_data
		size -= sizeof(sys_data);
		sys_data_copy = (struct sys_data*)(addr + size);
#endif
		_malloc_init(addr, size);
		_stdio_init();
		argc = _argv_init(&argv, 0);
#if TWO_STAGE
		argc1 = argc;
		addr = (char*)entry2 + entry2->image_size + entry2->bss_size;
		size = ram_addr + ram_size - (uint32_t)addr;
#endif
		printf("malloc heap: %u bytes\n", size);
	}

	sys_data.brightness = 50;
	// sys_data.rotate = 0x00;
	// sys_data.lcd_cs = 0;
	// sys_data.mac = 0;
	// sys_data.spi = 0;
	sys_data.spi_mode = 3;

	while (argc) {
		if (argc >= 2 && !strcmp(argv[0], "--bright")) {
			unsigned a = atoi(argv[1]);
			if (a <= 100) sys_data.brightness = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--rotate")) {
			char *next;
			unsigned scr, key;
			scr = key = strtol(argv[1], &next, 0);
			if (*next == ',') key = atoi(next + 1);
			sys_data.rotate = (key & 3) << 4 | (scr & 3);
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--cs")) {
			unsigned a = atoi(argv[1]);
			if (a < 4) sys_data.lcd_cs = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--spi")) {
			unsigned a = atoi(argv[1]);
			if (a < 2) sys_data.spi = (0x68 + a) << 24;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--spi_mode")) {
			unsigned a = atoi(argv[1]);
			if (a - 1 < 4) sys_data.spi_mode = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--lcd")) {
			sys_data.lcd_id = strtol(argv[1], NULL, 0);
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--mac")) {
			unsigned a = strtol(argv[1], NULL, 0);
			if (a < 0x100) sys_data.mac = a | 0x100;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--keymap")) {
			FILE *f = fopen(argv[1], "rb");
			if (f) {
				printf("keymap loaded from file\n");
				sys_data.keymap_addr = (short*)sys_data.keytrn;
				memset(sys_data.keytrn, -1, 64 * 2);
				fread(sys_data.keytrn, 1, 64 * 2, f);
				fclose(f);
			}
			argc -= 2; argv += 2;
		} else if (!memcmp(argv[0], "--", 2) && argv[0][2]) {
			printf("!!! unknown option \"%s\"\n", argv[0]);
			exit(1);
		} else break;
	}

	scan_firmware(fw_addr);
	CHIP_FN(sys_init)();
#if TWO_STAGE
	*sys_data_copy = sys_data;
	return argc1 - argc;
#else
	exit(main(argc, argv));
#endif
}

