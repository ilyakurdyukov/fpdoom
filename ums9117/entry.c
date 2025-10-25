#include "common.h"
#include "cmd_def.h"
#include "usbio.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if LIBC_SDIO
#include "sdio.h"
#include "microfat.h"
#include "fatfile.h"
#if CHIPRAM_ARGS || LIBC_SDIO >= 3
static int read_args(FILE *fi, char *d, char *e) {
	int argc = 0, j = 1, q = 0;
	for (;;) {
		int a = fgetc(fi);
		if (a == '#') {
			if (argc) break;
			do {
				a = fgetc(fi);
				if (a == EOF) goto end;
			} while (a != '\n');
			continue;
		}
		if (a <= 0x20) {
			if (a == '\r') continue;
			if (a != ' ' && a != '\t' && a != '\n') return -1;
			if (!q) {
				if (a == '\n' && argc) break;
				if (j) continue;
				a = 0;
			}
		} else if (a == '"' || a == '\'') {
			if (!q) {	q = a; argc += j; j = 0; continue; }
			if (q == a) { q = 0; continue; }
		} else if (a == '\\' && q != '\'') {
			do a = fgetc(fi); while (a == '\r');
			if (a == '\n') continue;
			if (a == EOF || a < 0x20) return -1;
		}
		argc += j;
		if (d == e) return -1;
		*d++ = a; j = !a;
	}
end:
	if (q) return -1;
	if (!j) {
		if (d == e) return -1;
		*d = 0;
	}
	return argc;
}
#endif
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
	uint32_t v1 = 0x12345678, i = 3;
	do MEM4(addr + (i << 24)) = i + v1; while (--i);
	do i++; while (i < 4 && MEM4(addr + (i << 24)) == i + v1);
	return i << 24; // 16M, 32M, 48M, 64M
}

#define ERR_EXIT(...) do { \
	fprintf(stderr, "!!! " __VA_ARGS__); \
	exit(1); \
} while (0)

uint16_t gpio_data[16];

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

#if LIBC_SDIO < 3
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
#endif
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
		uint32_t cb = 3 << 2; // Outer and Inner Write-Back, no Write-Allocate
		unsigned i;
		for (i = 0; i < 0x200; i++) tab[i] = 0; // no access
		for (; i < 0x800 + (ram_size >> 20); i++)
			tab[i] = i << 20 | 3 << 10 | 2; // read/write
		for (; i < 0x1000; i++) tab[i] = 0; // no access
#if 0
		i = 0x00000000 >> 20; // IRAM
		tab[i] = i << 20 | 3 << 10 | cb | 2; // read/write
#endif
#if 0
		i = 0xffff0000 >> 20; // ROM
		tab[i] = i << 20 | 0x23 << 10 | cb | 2; // read only
#endif
		for (i = 0; i < ram_size >> 20; i++)
			tab[ram_addr >> 20 | i] |= cb;
// TODO: binaries don't start when ACTLR.SMP is enabled for fpmenu
#if !NO_ACTLR
		// setup ACTLR
		__asm__ __volatile__(
			"mcr p15, #0, %0, c1, c0, #1"
			:: "r"(0x6040));
#endif
		// domains: 0 - manager, 1..3 - client, 4..15 - no access
		enable_mmu((uint32_t*)((uint32_t)tab | 0x49), 0x57);
	}
#endif
	{
		char *addr = image_addr + image_size + bss_size;
		size_t size = ram_addr + ram_size - (uint32_t)addr;
#if LIBC_SDIO
		fatdata_glob.buf = (uint8_t*)ram_addr + 0x8000;
#endif
#if TWO_STAGE
		// reserve space to save sys_data
		size -= sizeof(sys_data);
#if LIBC_SDIO
		size -= sizeof(fatdata_t);
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
#if LIBC_SDIO < 3
	sdio_init();
	if (sdcard_init())
		ERR_EXIT("sdcard_init failed\n");
#else
#if SDIO_SHL != CHIPRAM_ADDR
	sdio_shl = MEM4(CHIPRAM_ADDR);
#endif
#endif
	if (fat_init(&fatdata_glob, 0))
		ERR_EXIT("fat_init failed\n");
#endif
#if CHIPRAM_ARGS || LIBC_SDIO >= 3
	{
		char *p = (char*)CHIPRAM_ADDR + 4;
		argc1 = *(short*)p;
#if LIBC_SDIO
		if (!argc1) {
			FILE *fi = fopen(FPBIN_DIR "config.txt", "rb");
			if (!fi) ERR_EXIT("failed to open " FPBIN_DIR "config.txt");
			argc1 = read_args(fi, p + sizeof(short), (char*)CHIPRAM_ADDR + 0x1000);
			if (argc1 < 0) ERR_EXIT("read_args failed");
			fclose(fi);
		}
#endif
		argc1 = _argv_copy(&argv, argc1, p + sizeof(short));
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
	sys_data.keyrows = 8;
	sys_data.keycols = 8;

	gpio_data[0] = ~0;

	// CHAKEYAKE T190: 9,0,0x2f
	// Inoi 284 Flip: 1,0x16,0x16
	// Nokia 110 4G (TA-1543): 0,0xe,0
	sys_data.bl_extra[0] = 9;
	// sys_data.bl_extra[1] = 0;
	sys_data.bl_extra[2] = 0x2f;

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
		} else if (argc >= 2 && !strcmp(argv[0], "--lcd_cs")) {
			unsigned a = atoi(argv[1]);
			if (a < 4) sys_data.lcd_cs = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--spi")) {
			unsigned a = atoi(argv[1]);
			if (!~a) sys_data.spi = a;
			if (a < 2) sys_data.spi = (0x70a + a) << 20;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--spi_mode")) {
			unsigned a = atoi(argv[1]);
			if (a - 1 < 4) sys_data.spi_mode = a;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--lcd")) {
			unsigned id = strtol(argv[1], NULL, 0);
			sys_data.lcd_id = id;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--mac")) {
			unsigned a = strtol(argv[1], NULL, 0);
			if (a < 0x100) sys_data.mac = a | 0x100;
			argc -= 2; argv += 2;
		} else if (!strcmp(argv[0], "--gpio_init")) {
			sys_data.gpio_init = 1;
			argc -= 1; argv += 1;
		} else if (argc >= 2 && !strcmp(argv[0], "--gpio_data")) {
			char *s = argv[1];
			int i = 0, a, val;
			for (;;) {
				a = strtol(s, &s, 0);
				val = 0;
				if (a < 0) val = 1, a = -a;
				if (a >= 0x80) ERR_EXIT("invalid gpio_data num\n");
				gpio_data[i++] = a | val << 15;
				if ((unsigned)i >= sizeof(gpio_data) / sizeof(*gpio_data))
					ERR_EXIT("too many gpio_data\n");
				a = *s++;
				if (!a) break;
				if (a != ',') ERR_EXIT("invalid separator\n");
			}
			gpio_data[i] = ~0;
			sys_data.gpio_init = 1;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--bl_extra")) {
			char *s = argv[1];
			sys_data.bl_extra[0] = strtol(s, &s, 0) & 0xf;
			if (*s++ != ',') ERR_EXIT("invalid separator\n");
			sys_data.bl_extra[1] = strtol(s, &s, 0) & 0x3f;
			if (*s++ != ',') ERR_EXIT("invalid separator\n");
			sys_data.bl_extra[2] = strtol(s, &s, 0) & 0x3f;
			argc -= 2; argv += 2;
		} else if (argc >= 2 && !strcmp(argv[0], "--charge")) {
			sys_data.charge = atoi(argv[1]);
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
#if LIBC_SDIO
			fatdata_t *fatdata = &fatdata_glob;
			const char *name = argv[1];
			unsigned clust = fat_dir_clust(fatdata, name);
			if (!clust)
				ERR_EXIT("!!! dir \"%s\" not found\n", name);
			fatdata->curdir = clust;
#else
			printf("dir option ignored\n");
#endif
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
	data_copy += sizeof(sys_data);
#if LIBC_SDIO
	MEM4(CHIPRAM_ADDR) = sdio_shl;
	memcpy(data_copy, &fatdata_glob, sizeof(fatdata_t));
	data_copy += sizeof(fatdata_t);
#endif
#if CHIPRAM_ARGS || LIBC_SDIO >= 3
	{
		char *p = (char*)CHIPRAM_ADDR + 4;
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

