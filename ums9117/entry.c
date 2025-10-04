#include "common.h"
#include "cmd_def.h"
#include "usbio.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void apply_reloc(uint32_t *image, const uint8_t *rel, uint32_t diff);

void _debug_msg(const char *msg);
void _malloc_init(void *addr, size_t size);
void _stdio_init(void);
int _argv_init(char ***argvp, int skip);
int _argv_copy(char ***argvp, int argc, char *src);

int main(int argc, char **argv);

/* dcache must be disabled for this function */
static uint32_t get_ram_size(uint32_t addr) {
	uint32_t v1 = 0x12345678, i = 3;
	do MEM4(addr + (i << 24)) = i + v1; while (--i);
	do i++; while (i < 4 && MEM4(addr + (i << 24)) == i + v1);
	return i << 24; // 16M, 32M, 48M, 64M
}

#define ERR_EXIT(...) do { \
	fprintf(stderr, "!!! " __VA_ARGS__); \
	exit(1); \
} while (0)

#if TWO_STAGE
struct entry2 {
	uint32_t dhtb[0x200 / 4];
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
	int argc1, argc; char **argv = NULL;
	uint32_t ram_addr = 0x80000000;
	uint32_t ram_size;
#if TWO_STAGE
	char *data_copy;
#endif

#if TWO_STAGE
	{
		// must be done before zeroing BSS
		uint32_t diff = (uint32_t)entry2 - entry2->image_start;
		if (diff) apply_reloc((uint32_t*)entry2, entry2_rel, diff);
	}
#endif

	memset(image_addr + image_size, 0, bss_size);

	{
		static const uint8_t fdl_ack[8] = {
			0x7e, 0, 0x80, 0, 0, 0xff, 0x7f, 0x7e };
		char buf[4]; uint32_t t0, t1;
		usb_init();
		usb_write(fdl_ack, sizeof(fdl_ack));
		for (;;) {
			usb_read(buf, 1, USB_WAIT);
			if (buf[0] == HOST_CONNECT) break;
		}
		t0 = sys_timer_ms();
		do {
			t1 = sys_timer_ms();
			if (usb_read(buf, 4, 0)) t0 = t1;
		} while (t1 - t0 < 10);
	}
#if TWO_STAGE
	_debug_msg("entry1");
#else
	_debug_msg("entry");
#endif

	ram_size = get_ram_size(ram_addr);
#if TWO_STAGE
	MEM4(entry2) = ram_size;
#else
	MEM4(image_addr) = ram_size;
#endif
#if INIT_MMU
	{
		volatile uint32_t *tab = (uint32_t*)ram_addr;
		uint32_t cb = 3 << 2; // cached=1, buffered=1
		unsigned i;
		for (i = 0; i < 0x200; i++) tab[i] = 0; // no access
		for (; i < 0x800 + (ram_size >> 20); i++)
			tab[i] = i << 20 | 3 << 10 | 2; // read/write
		for (; i < 0x1000; i++) tab[i] = 0; // no access
#if 0
		i = 0xffff0000 >> 20; // ROM
		tab[i] = i << 20 | 7 << 10 | cb | 2; // read only
#endif
		for (i = 0; i < ram_size >> 20; i++)
			tab[ram_addr >> 20 | i] |= cb;
		// domains: 0 - manager, 1..3 - client, 4..15 - no access
		enable_mmu((uint32_t*)tab, 0x57);
	}
#endif
	{
		char *addr = image_addr + image_size + bss_size;
		size_t size = ram_addr + ram_size - (uint32_t)addr;
#if TWO_STAGE
		// reserve space to save sys_data
		size -= sizeof(sys_data);
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
	argc1 = _argv_init(&argv, 0);
	argc = argc1;

	sys_data.brightness = 50;
	// sys_data.scaler = 0;
	// sys_data.rotate = 0x00;
	// sys_data.mac = 0;
	sys_data.keyrows = 8;
	sys_data.keycols = 8;

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
		} else if (argc >= 2 && !strcmp(argv[0], "--lcd")) {
			unsigned id = strtol(argv[1], NULL, 0);
			sys_data.lcd_id = id;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--mac")) {
			unsigned a = strtol(argv[1], NULL, 0);
			if (a < 0x100) sys_data.mac = a | 0x100;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--keyflags")) {
			sys_data.keyflags = ~atoi(argv[1]);
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--keyrows")) {
			unsigned a = atoi(argv[1]);
			if (a - 2 <= 6) sys_data.keyrows = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--keycols")) {
			unsigned a = atoi(argv[1]);
			if (a - 2 <= 6) sys_data.keycols = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--keymap")) {
			FILE *f = fopen(argv[1], "rb");
			if (f) {
				uint8_t *p = (uint8_t*)sys_data.keytrn;
				printf("keymap loaded from file\n");
				sys_data.keymap_addr = (short*)p;
				memset(p, -1, 64 * 2);
				fread(p, 1, 64 * 2, f);
				fclose(f);
			}
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--dir")) {
			printf("dir option ignored\n");
			argc -= 2; argv += 2;
		} else if (!strcmp(argv[0], "--")) {
			argc -= 1; argv += 1;
			break;
		} else if (argv[0][0] == '-') {
			ERR_EXIT("unknown option \"%s\"\n", argv[0]);
		} else break;
	}

	scan_firmware(0);
	sys_init();
#if TWO_STAGE
	memcpy(data_copy, &sys_data, sizeof(sys_data));
	return argc1 - argc;
#else
#ifdef CXX_SUPPORT
	cxx_init(argc, argv);
#endif
	exit(main(argc, argv));
#endif
}

