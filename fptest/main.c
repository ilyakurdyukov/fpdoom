#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "syscode.h"
#include "cmd_def.h"
#include "usbio.h"
#include "sdio.h"
#if !UMS9117
#include "sfc.h"
#endif

#define FAT_READ_SYS \
	if (sdio_read_block(sector, buf)) break;
#include "microfat.h"
fatdata_t fatdata_glob;
#include "fatfile.h"

void lcd_appinit(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w1, h = disp->h1;
	disp->w2 = w;
	disp->h2 = h;
}

static void *framebuf_mem = NULL;
static uint16_t *framebuf = NULL;

static void framebuf_alloc(void) {
	struct sys_display *disp = &sys_data.display;
	int w = disp->w2, h = disp->h2;
	size_t size = w * h;
	uint8_t *p;
	framebuf_mem = p = malloc(size * 2 + 31);
	p += -(intptr_t)p & 31;
	framebuf = (void*)p;
}

static void gen_image(uint16_t *p, unsigned w, unsigned h, unsigned st) {
	unsigned x, y;

	for (y = 0; y < h; y++)
	for (x = 0; x < w; x++) {
		unsigned a = x * 32 / w;
		if (y < h >> 2) a <<= 11;
		else if (y < h >> 1) a <<= 5 + 1;
		else if (y >= 3 * h >> 2) a *= 1 << 11 | 1 << (5 + 1) | 1;
		p[y * st + x] = a;
	}
	p[0] = -1; p[w - 1] = -1;
	p += (h - 1) * st;
	p[0] = -1; p[w - 1] = -1;
}

static void test_refresh(void) {
	uint32_t t0 = sys_timer_ms(), t1, n = 0, t2, t3;
	do {
		sys_start_refresh();
		sys_wait_refresh();
		t1 = sys_timer_ms();
		n++;
	} while (t1 - t0 < 1000);
	t1 -= t0;
	printf("%d frames in %d ms\n", n, t1);
	t2 = t1 / n;
	t3 = (t1 - t2 * n) * 1000 / n;
	printf("1 frame in %d.%03d ms\n", t2, t3);
	t3 = n * 1000;
	t2 = t3 / t1;
	t3 = (t3 - t2 * t1) * 1000 / t1;
	printf("%d.%03d frames per second\n", t2, t3);
}

void test_display(void) {
	struct sys_display *disp = &sys_data.display;
	unsigned w = disp->w2, h = disp->h2;
	framebuf_alloc();
	sys_framebuffer(framebuf);
	sys_start();
	gen_image(framebuf, w, h, w);
	test_refresh();
	free(framebuf_mem);
}

static void usbio_bench(int arg, unsigned len, unsigned n) {
	void *mem = malloc(len + 32);
	uint8_t *buf = (uint8_t*)(((intptr_t)mem + 31) & ~31);
	uint32_t t0, t1; unsigned i;

	{
		union { uint16_t u16[5]; uint32_t u32[2]; } buf;
		unsigned tmp = CMD_USBBENCH | arg << 8;
		buf.u16[0] = tmp; tmp += CHECKSUM_INIT;
		buf.u16[1] = len; tmp += len;
		buf.u32[1] = n; tmp += (uint16_t)n + (n >> 16);
		buf.u16[4] = tmp + (tmp >> 16);
		usb_write(&buf, 10); // sizeof(buf) == 12
	}

	memset(buf, 0, len);
	t0 = sys_timer_ms();
	if (arg == 0)
	for (i = 0; i < n; i += len)
		usb_read(buf, len, USB_WAIT);
	else
	for (i = 0; i < n; i += len)
		usb_write(buf, len);
	t1 = sys_timer_ms();
	free(mem);
	t1 -= t0;
	printf("usbio raw %s: %u bytes in %u ms\n",
			arg ? "write" : "read", i, t1);
}

static void sdio_bench(uint32_t pos, uint32_t size) {
	void *mem = malloc(512 + 32);
	uint8_t *buf = (uint8_t*)(((intptr_t)mem + 31) & ~31);
	unsigned i, n = size >> 9;
	uint32_t t0, t1;

	pos >>= 9;
	t0 = sys_timer_ms();
	for (i = 0; i < n; i++)
		if (sdio_read_block(pos + i, buf)) break;
	t1 = sys_timer_ms();
	free(mem);
	t1 -= t0;
	printf("sdio raw read: %u bytes in %u ms\n", i << 9, t1);
}

static void sdio_write_test(unsigned i) {
	void *mem = malloc(512 + 32);
	uint8_t *buf = (uint8_t*)(((intptr_t)mem + 31) & ~31);
	int ret = 0;
	do {
		uint32_t val;
		if (sdio_read_block(i, buf)) break;
		val = *(uint32_t*)buf;
		*(uint32_t*)buf = ~val;
		if (sdio_write_block(i, buf)) break;
		*(uint32_t*)buf = val ^ 0x55555555;
		if (sdio_read_block(i, buf)) break;
		if (*(uint32_t*)buf != ~val) break;
		ret = 1;
	} while (0);
	printf("sdio write %s\n", ret ? "ok" : "failed");
	// check: dd if=/dev/sdx bs=512 skip=1 count=1 | hd
	free(mem);
}

static void test_keypad(void) {
	int type, key;

	for (;;) {
		type = sys_event(&key);
		switch (type) {
		case EVENT_KEYUP:
		case EVENT_KEYDOWN: {
			const char *s = "???";
			int power = key >> 15;
			key &= 0x7fff;
			switch (key) {
#define X(val, name) case val: s = #name; break;
			KEYPAD_ENUM(X)
#undef X
			}
			printf("%s0x%02x (\"%s\") %s\n", power ? "POWER + " : "",
					key, s, type == EVENT_KEYUP ? "release" : "press");
	    break;
		}
		case EVENT_END: sys_wait_ms(10); break;
		case EVENT_QUIT: goto end;
		}
	}
end:;
}

#if UMS9117
static int boot_cable_check(void) { return 0; }
static void test_sfc(void) {}
static void test_lzma(int flags) { (void)flags; }
#else
static int boot_cable_check(void) {
	return !(MEM4(0x205000e0) & 2);
}

static void test_sfc(void) {
	unsigned cs = 0; // SFC_BASE->cs & 1;
	uint32_t id; const char *name;
	printf("sfc: cs1 start = 0x%02x\n", MEM4(0x20a00200) & 0xff);
	sfc_init();
	id = sfc_readid(cs);
	printf("sfc: id = 0x%06x\n", id);
	name = sfc_getvendor(id);
	if (name) printf("flash vendor: %s\n", name);
	name = sfc_getname(id);
	if (name) printf("flash name: %s\n", name);
	{
		int sr2 = -1, sr3 = -1;
		switch (id >> 16) {
		case 0xef: /* Winbond */
			sr3 = 0x15; /* fallthrough */
		case 0xc8: /* GigaDevice */
		case 0xf8: /* Fidelix/Dosilicon */
			sr2 = 0x35; break;
		}
		printf("sfc: sr1 = 0x%02x\n", sfc_read_status(cs));
		if (sr2 >= 0)
			printf("sfc: sr2 = 0x%02x\n", sfc_cmd_read(cs, sr2, 1) >> 24);
		if (sr3 >= 0)
			printf("sfc: sr3 = 0x%02x\n", sfc_cmd_read(cs, sr3, 1) >> 24);
	}
	{
		union { uint8_t u8[256]; uint32_t u32[64]; } buf;
		sfc_read_sfdp(cs, 0, buf.u8, 256);
		if (buf.u32[0] == 0x50444653) {
			int i;
			printf("sfc: SFDP data\n");
			for (i = 0; i < 256; i++)
				printf("%02x%s", buf.u8[i], (i + 1) & 15 ? " " : "\n");
		} else printf("sfc: no SFDP support\n");
	}
	sfc_spiread(cs);
	if (1) {
		uint32_t addr = 0x000000; uint8_t *p;
		unsigned i, n = 0x20000 - 1;
		uint8_t *buf = malloc(n);
		sfc_read(cs, addr, buf, n);
		printf("sfc_compare: ret = %u\n", sfc_compare(cs, addr, buf, n, SFC_CMP_POST));
		sfc_spiread(cs);
		p = (uint8_t*)(_chip == 1 ? 0x10000000 : 0x30000000);
		p += (addr & ~0xff);
		for (i = 0; i < n; i++)
			if (buf[i] != p[i]) break;
		printf("sfc_read check: %u / %u\n", i, n);
		free(buf);
	}
#if 0 // test write
	{
		uint32_t addr = 0x390004; uint8_t *p;
		uint8_t buf[] = { 0xef,0xff,0xff,0x7f, 0xff,0xff,0xfe };
		sfc_write(cs, addr, buf, sizeof(buf));
		sfc_spiread(cs);
		p = (uint8_t*)(_chip == 1 ? 0x10000000 : 0x30000000);
		p += (addr & ~0xff);
		clean_invalidate_dcache_range(p, p + 256);
		print_mem(stdout, p, 256);
	}
#endif
#if 0 // test erase
	{
		uint32_t addr = 0x390000; uint8_t *p;
		sfc_erase(cs, addr, 0x20, 3); // Sector Erase (4KB)
		sfc_spiread(cs);
		p = (uint8_t*)(_chip == 1 ? 0x10000000 : 0x30000000);
		p += (addr & ~0xff);
		clean_invalidate_dcache_range(p, p + 256);
		print_mem(stdout, p, 256);
	}
#endif
}

static void test_lzma(int flags) {
	uint32_t *drps, *colb; uint8_t *dst;
	unsigned colb_offs, src_offs, src_size, dst_size;
	unsigned t0, t1, ret;
	uint32_t *trapgami = NULL;
	intptr_t fw_addr = (_chip == 1 ? 1 : 3) << 28;
	uint32_t *p, *end = (uint32_t*)(fw_addr + 0x11000);

	for (p = (uint32_t*)fw_addr; p < end; p++) {
		// "TRAPGAMI": offsets to "DRPS" sections
		do {
			if (p[0] != 0x50415254) break;
			if (p[1] != 0x494d4147) break;
			// p[2] == ~0 if there's no image
			if (p[2] & 0xff000003) break;
			trapgami = p;
			goto found;
		} while (0);
	}
	return;

found:
	printf("trapgami = 0x%x, drps = 0x%x\n",
			(intptr_t)trapgami - fw_addr, trapgami[2]);
	drps = (uint32_t*)(fw_addr + trapgami[2]);
	do {
		if (drps[0] != 0x53505244) break;
		if (drps[3] != 1) break; // count
		colb_offs = drps[2];
		colb = (uint32_t*)((uint8_t*)drps + colb_offs);
		if (colb[0] != 0x424c4f43) break;
		if (colb[1] != 0x494d4147) break;
		src_offs = colb[2];
		src_size = colb[3];
		dst_size = colb[4];

		dst = malloc(dst_size);
		t0 = sys_timer_ms();
		ret = sys_lzma_decode((uint8_t*)drps + src_offs, src_size, dst, dst_size);
		t1 = sys_timer_ms();
		printf("lzma_decode: %ums, ret = 0x%x\n", t1 - t0, ret);
		if ((flags & 1) && ret == 1) {
			FILE *f = fopen("kern.bin", "wb");
			if (f) {
				fwrite(dst, 1, dst_size, f);
				fclose(f);
			}
		}
		free(dst);
	} while (0);
}
#endif

static void test_timer(void) {
	unsigned t0, t1, delay = 100000;
	t0 = sys_timer_ms();
	sys_wait_us(delay);
	t1 = sys_timer_ms();
	printf("sys_wait_us(%u): %ums\n", delay, t1 - t0);
}

static void test_cpuid(void) {
	uint32_t id0, id1;
	__asm__ __volatile__(
#ifdef __thumb__
		".p2align 2\nbx pc\n.p2align 2\n.code 32\n"
#endif
		"mrc p15, #0, %0, c0, c0, #0\n" // MIDR
		"mrc p15, #0, %1, c0, c0, #1\n" // CTR
#ifdef __thumb__
		"blx 1f\n.code 16\n1:\n"
#endif
		: "=r"(id0), "=r"(id1) :: "lr");
	printf("ARM: id = 0x%08x, cache = 0x%08x\n", id0, id1);
	// id = 0x41069265
	// SC6530C/SC6531: cache = 0x1d152152 (16+16KB)
	// SC6531E: cache = 0x1d112112 (8+8KB)

	// UMS9117:
	// id = 0x410fc075 (Cortex-A7 MPCore, r0p5)
	// cache = 0x84448003
#if UMS9117
	{
		uint32_t i, n, x, clidr;

		__asm__ __volatile__(
			"mrc p15, #1, %0, c0, c0, #1\n"
			: "=r"(clidr));
		printf("ARM: CLIDR = 0x%08x\n", clidr);

		n = clidr >> 24 & 7; // LoC
		for (i = 0; i < n; i++) {
			__asm__ __volatile__(
				"mcr p15, #2, %1, c0, c0, #0\n"
				"isb\n"
				"mrc p15, #1, %0, c0, c0, #0\n"
				: "=r"(x) : "r"(i * 2));
			printf("ARM: CCSIDR(%u) = 0x%08x\n", i, x);
		}
		// ARM: CLIDR = 0x0a200023 (LoUU = 1, LoC = 2, Ctype2 = 4, Ctype1 = 3)
		// ARM: CCSIDR(0) = 0x700fe01a (NumSets = 0x7f, Associativity = 3, LineSize = 2)
		// ARM: CCSIDR(1) = 0x701fe03a (NumSets = 0xff, Associativity = 7, LineSize = 2)
		if (n > 1) {
			__asm__ __volatile__(
				"mcr p15, #2, %0, c0, c0, #0\n"
				"isb\n" :: "r"(0));
		}
	}
#endif
}

static int sdtest_init(int target) {
	static int init_done = 0;

	do {
		if (!(init_done & 1)) {
			sdio_init();
			init_done |= 1;
		}
		if (!(init_done & 2)) {
			if (sdcard_init()) {
				printf("!!! sdcard_init failed\n");
				break;
			}
			SDIO_VERBOSITY(0);
			init_done |= 2;
		}
		if (!(init_done & 4)) {
			fatdata_glob.buf = (uint8_t*)CHIPRAM_ADDR + 0x9000;
			if (fat_init(&fatdata_glob, 0)) {
				printf("!!! fat_init failed\n");
				break;
			}
			init_done |= 4;
		}
	} while (0);
	return ~init_done & target;
}

int main(int argc, char **argv) {
	int i;

	if (boot_cable_check())
		printf("boot cable detected\n");

	if (1) {
		printf("argc = %i\n", argc);
		for (i = 0; i < argc; i++)
			printf("[%i] = %s\n", i, argv[i]);
	}

	if (argc < 2) {
		static const char * const tab[] = {
			"fptest", "cpuid", "display", "timer",
			"sfc", "lzma", "usb", "sdio",
			"keypad", NULL };
		argv = (char**)tab;
		argc = sizeof(tab) / sizeof(*tab) - 1;
	}

	while (argc > 1) {
		if (!strcmp(argv[1], "display")) {
			test_display();
			argc -= 1; argv += 1;
		} else if (!strcmp(argv[1], "keypad")) {
			test_keypad();
			argc -= 1; argv += 1;
		} else if (!strcmp(argv[1], "cpuid")) {
			test_cpuid();
			argc -= 1; argv += 1;
		} else if (!strcmp(argv[1], "timer")) {
			test_timer();
			argc -= 1; argv += 1;
		} else if (!strcmp(argv[1], "usb")) {
			usbio_bench(0, 1024, 1 << 20);
			usbio_bench(1, 1024, 1 << 20);
			argc -= 1; argv += 1;
		} else if (!strcmp(argv[1], "sfc")) {
			test_sfc();
			argc -= 1; argv += 1;
		} else if (!strcmp(argv[1], "lzma")) {
			test_lzma(0);
			argc -= 1; argv += 1;
		}	else if (!strcmp(argv[1], "sdio")) {
			if (!sdtest_init(3)) {
				sdio_bench(1 << 20, 1 << 20);
				if (0) sdio_write_test(1);
			}
			argc -= 1; argv += 1;
		}	else if (argc >= 3 && !strcmp(argv[1], "sdread")) {
			if (!sdtest_init(7)) {
				unsigned clust, size;
				fat_entry_t *p = fat_find_path(&fatdata_glob, argv[2]);
				if (!p || (p->entry.attr & FAT_ATTR_DIR)) {
					printf("file not found\n");
				} else {
					const char *out_fn;
					uint8_t *mem, *buf;
					unsigned n;

					clust = fat_entry_clust(p);
					size = p->entry.size;
					printf("start = 0x%x, size = 0x%x\n", clust, size);

					buf = mem = malloc(((size + 0x1ff) & ~0x1ff) + 31);
					buf += -(intptr_t)buf & 31;
					n = fat_read_simple(&fatdata_glob, clust, buf, size);
					printf("nread = 0x%x\n", n);

					out_fn = argv[3];
					if (*out_fn && strcmp(out_fn, "-")) {
						FILE *fo;
						fo = fopen(out_fn, "wb");
						if (fo) {
							fwrite(buf, 1, size, fo);
							fclose(fo);
						}
					}
					free(mem);
				}
			}
			argc -= 3; argv += 3;
		} else {
			printf("unknown command (\"%s\")\n", argv[1]);
			return 1;
		}
	}
	return 0;
}

void keytrn_init(void) {
	uint8_t keymap[64];
	int i, flags = sys_getkeymap(keymap);
	(void)flags;

#define FILL_KEYTRN(j) \
	for (i = 0; i < 64; i++) { \
		sys_data.keytrn[j][i] = keymap[i] | j << 15; \
	}

	FILL_KEYTRN(0)
	FILL_KEYTRN(1)
#undef FILL_KEYTRN
}

