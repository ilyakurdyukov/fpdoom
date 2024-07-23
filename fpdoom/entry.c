#include "common.h"
#include "cmd_def.h"
#include "usbio.h"
#include "syscode.h"

#define SMC_INIT_BUF (CHIPRAM_ADDR + 0xa000 - 1024)
#if !CHIP || CHIP == 2
#include "init_sc6531da.h"
#endif
#if !CHIP || CHIP == 3
#include "init_sc6530.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if LIBC_SDIO
#include "sdio.h"
#include "microfat.h"
#include "fatfile.h"
#endif

void apply_reloc(uint32_t *image, const uint8_t *rel, uint32_t diff);

void _debug_msg(const char *msg);
void _malloc_init(void *addr, size_t size);
void _stdio_init(void);
int _argv_init(char ***argvp, int skip);
int _argv_copy(char ***argvp, int argc, char *src);

int main(int argc, char **argv);

/* dcache must be disabled for this function */
static uint32_t get_ram_size(uint32_t addr) {
	uint32_t size = 1 << 20; /* start from 2MB */
	uint32_t v1 = 0x12345678;
	do {
		if (size >> 27) break;
		size <<= 1;
		MEM4(addr + size) = v1;
		MEM4(addr) = size;
	} while (MEM4(addr + size) == v1);
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
#ifdef CXX_SUPPORT
#include "cxx_init.h"
#endif
void entry_main(char *image_addr, uint32_t image_size, uint32_t bss_size) {
#endif
	static const uint8_t fdl_ack[8] = {
		0x7e, 0, 0x80, 0, 0, 0xff, 0x7f, 0x7e };
	int argc1, argc; char **argv = NULL;
	uint32_t fw_addr = (uint32_t)image_addr & 0xf0000000;
	uint32_t ram_addr = fw_addr + 0x04000000, ram_size;
#if TWO_STAGE
	char *data_copy;
#endif
#if !CHIP
	uint32_t chip = 1;
#endif

	// SC6531E is initialized by FDL1
#if !CHIP
	if (fw_addr == 0x30000000) {
		uint32_t t0 = MEM4(0x205003fc);
		chip = (int32_t)(t0 << 15) < 0 ? 2 : 3;
		if (chip == 2) init_sc6531da();
		if (chip == 3) init_sc6530();
		chip_fn[0] = chip_fn[1];
	}
#elif CHIP == 2
	init_sc6531da();
#elif CHIP == 3
	init_sc6530();
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
	_chip = chip;
#endif

#if LIBC_SDIO < 3
	usb_init();
	usb_write(fdl_ack, sizeof(fdl_ack));

	for (;;) {
		char buf[4];
		usb_read(buf, 1, USB_WAIT);
		if (buf[0] == HOST_CONNECT) break;
	}
#endif
#if TWO_STAGE
	_debug_msg("entry1");
#else
	_debug_msg("entry");
#endif

	ram_size = get_ram_size(ram_addr);
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
	ram_size -= 0x4000; // MMU table
#endif
	{
		char *addr = image_addr + image_size + bss_size;
		size_t size = ram_addr + ram_size - (uint32_t)addr;
#if LIBC_SDIO
		size -= 1024;
		fatdata_glob.buf = (uint8_t*)addr + size;
#endif
#if TWO_STAGE
		// reserve space to save sys_data
		size -= sizeof(sys_data);
#if LIBC_SDIO
		size -= sizeof(unsigned) + sizeof(fatdata_t);
#endif
		data_copy = addr + size;
#endif
		_malloc_init(addr, size);
		_stdio_init();
#if TWO_STAGE
		addr = (char*)entry2 + entry2->image_size + entry2->bss_size;
		size = ram_addr + ram_size - (uint32_t)addr;
#endif
		printf("malloc heap: %u bytes\n", size);
	}
#if LIBC_SDIO
	sdio_init();
	if (sdcard_init()) {
		printf("!!! sdcard_init failed\n");
		exit(1);
	}
	if (fat_init(&fatdata_glob, 0)) {
		printf("!!! fat_init failed\n");
		exit(1);
	}
#endif
#if CHIPRAM_ARGS || LIBC_SDIO >= 3
	{
		char *p = (char*)CHIPRAM_ADDR;
		argc1 = *(short*)p;
		if (argc1) {
			argc1 = _argv_copy(&argv, argc1, p + sizeof(short));
			p += sizeof(short);
		} else {
			// TODO: read and parse args file
			argv = NULL;
		}
	}
#if LIBC_SDIO == 1 // debug
#define PRINT_ARGS(argc) { int j; printf("argc = %u\n", argc); \
	for (j = 0; j < argc; j++) printf("%u: \"%s\"\n", j, argv[j]); }
	PRINT_ARGS(argc1)
#endif
#else
	argc1 = _argv_init(&argv, 0);
#endif
	argc = argc1;

	sys_data.brightness = 50;
	// sys_data.scaler = 0;
	// sys_data.rotate = 0x00;
	// sys_data.lcd_cs = 0;
	// sys_data.mac = 0;
	// sys_data.spi = 0;
	// sys_data.gpio_init = 0;
	sys_data.charge = -1;
	sys_data.spi_mode = 3;
	sys_data.bl_gpio = 0xff;
	sys_data.keycols = 5;

	while (argc) {
		if (argc >= 2 && !strcmp(argv[0], "--bright")) {
			unsigned a = atoi(argv[1]);
			if (a <= 100) sys_data.brightness = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--scaler")) {
			sys_data.scaler = atoi(argv[1]) + 1;
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
		} else if (argc >= 2 && !strcmp(argv[0], "--gpio_init")) {
			sys_data.gpio_init = 1;
			argc -= 1; argv += 1;
		} else if (argc >= 2 && !strcmp(argv[0], "--bl_gpio")) {
			sys_data.gpio_init = 1;
			sys_data.bl_gpio = atoi(argv[1]);
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--charge")) {
			sys_data.charge = atoi(argv[1]);
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--keycols")) {
			unsigned a = atoi(argv[1]);
			if (a <= 8) sys_data.keycols = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--keymap")) {
			FILE *f = fopen(argv[1], "rb");
			if (f) {
				printf("keymap loaded from file\n");
				sys_data.keymap_addr = (short*)sys_data.keytrn;
				memset(sys_data.keytrn, -1, 64 * 2);
				fread(sys_data.keytrn, 1, 64 * 2, f);
				fclose(f);
				sys_data.keycols = 8;
			}
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--dir")) {
#if LIBC_SDIO
			fatdata_t *fatdata = &fatdata_glob;
			const char *name = argv[1];
			unsigned clust = fat_dir_clust(fatdata, name);
			if (!clust) {
				printf("dir \"%s\" not found\n", name);
				exit(1);
			}
			fatdata->curdir = clust;
#else
			printf("dir option ignored\n");
#endif
			argc -= 2; argv += 2;
		} else if (!strcmp(argv[0], "--")) {
			argc -= 1; argv += 1;
			break;
		} else if (argv[0][0] == '-') {
			printf("!!! unknown option \"%s\"\n", argv[0]);
			exit(1);
		} else break;
	}

	scan_firmware(fw_addr);
	CHIP_FN(sys_init)();
#if TWO_STAGE
	memcpy(data_copy, &sys_data, sizeof(sys_data));
	data_copy += sizeof(sys_data);
#if LIBC_SDIO
	*(unsigned*)data_copy = sdio_shl;
	data_copy += sizeof(unsigned);
	memcpy(data_copy, &fatdata_glob, sizeof(fatdata_t));
	data_copy += sizeof(fatdata_t);
#endif
#if CHIPRAM_ARGS || LIBC_SDIO >= 3
	{
		char *p = (char*)CHIPRAM_ADDR;
		*(short*)p = argc;
		if (argc) {
			size_t size = argv[argc - 1] - argv[0];
			size += strlen(argv[argc - 1]) + 1;
			memcpy(p + sizeof(short), argv[0], size);
		}
	}
#if LIBC_SDIO == 1
	PRINT_ARGS(argc)
#endif
	return 0;
#else
	return argc1 - argc;
#endif
#else
#ifdef CXX_SUPPORT
	cxx_init(argc, argv);
#endif
	exit(main(argc, argv));
#endif
}

