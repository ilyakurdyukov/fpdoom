#include "common.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)
#define ERR_EXIT(...) \
	do { fprintf(stderr, "!!! " __VA_ARGS__); exit(1); } while (0)

uint32_t *pinmap_addr, *gpiomap_addr;

static uint8_t* loadfile(const char *fn, size_t *num) {
	size_t n, j = 0; uint8_t *buf = 0;
	FILE *fi = fopen(fn, "rb");
	if (fi) {
		fseek(fi, 0, SEEK_END);
		n = ftell(fi);
		if (n) {
			fseek(fi, 0, SEEK_SET);
			buf = (uint8_t*)malloc(n);
			if (buf) j = fread(buf, 1, n, fi);
		}
		fclose(fi);
	}
	if (num) *num = j;
	return buf;
}


static uint32_t* gpiomap_check(uint32_t *p, unsigned size) {
	uint32_t *p0;
	if (size < 8 + 12) return NULL;
	// skip empty GPIO related table
	if (~p[0] || p[1] != 0xffff) return NULL;
	p0 = p += 2; size -= 8;
	for (; size >= 12; p += 3, size -= 12) {
		uint32_t a = p[0];
		if (a & 0xfffeff80) {
			if (a != 0xffff || p[1] != 2 || p[2] != 5) break;
			return p0;
		}
		if (p[1] >= 2 || p[2] >= 6) break;
	}
	return NULL;
}

static int pinmap_check(uint32_t *p, uint32_t size) {
	if ((size < 8) || (size & 3)) return -1;
	for (; size > 8; size -= 8, p += 2) {
		uint32_t addr = p[0];
		if (!(addr - 0x40608000 < 0x1000) &&
				!(addr - 0x402a0000 < 0x1000))
			break;
	}
	if (~(p[0] & p[1])) return -1;
	gpiomap_addr = gpiomap_check(p + 2, size - 8);
	return 0;
}

void scan_firmware(intptr_t fw_addr) {
#if LIBC_SDIO
	const char *pinmap_fn = "/" FPBIN_DIR "pinmap.bin";
	const char *keymap_fn = "/" FPBIN_DIR "keymap.bin";
#else
	const char *pinmap_fn = "pinmap.bin";
	const char *keymap_fn = "keymap.bin";
#endif
	size_t size;
	(void)fw_addr;

	pinmap_addr = (uint32_t*)loadfile(pinmap_fn, &size);
	if (!pinmap_addr) ERR_EXIT("pinmap not found\n");
	if (pinmap_check(pinmap_addr, size)) ERR_EXIT("pinmap check failed\n");
	if (sys_data.gpio_init && !gpiomap_addr)
		ERR_EXIT("gpiomap not found\n");

	if (!sys_data.keymap_addr) {
		FILE *f = fopen(keymap_fn, "rb");
		if (f) {
			uint8_t *p = (uint8_t*)sys_data.keytrn;
			printf("keymap loaded from file\n");
			sys_data.keymap_addr = (short*)p;
			memset(p, -1, 64 * 2);
			fread(p, 1, 64 * 2, f);
			fclose(f);
		}
	}
}

// system timer, 1ms step
uint32_t sys_timer_ms(void) {
	return MEM4(0x4023000c);
}

void sys_wait_ms(uint32_t delay) {
	uint32_t start = sys_timer_ms();
	while (sys_timer_ms() - start < delay);
}

void sys_wait_us(uint32_t delay) {
	delay *= 1000;
	sys_wait_clk(delay);
}

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

int sys_getkeymap(uint8_t *dest) {
	short *keymap = sys_data.keymap_addr;
	int i, j, rotate = sys_data.rotate >> 4 & 3;
	static const unsigned char num_turn[][4 + 9] = {
		{ "\004\005\006\007123456789" },
		{ "\007\006\004\005369258147" },
		{ "\005\004\007\006987654321" },
		{ "\006\007\005\004741852963" },
	};
	int nrow = sys_data.keyrows;
	int ncol = sys_data.keycols;
	int flags = 0;

	memset(dest, -1, 64);

	for (i = 0; i < ncol; i++)
	for (j = 0; j < nrow; j++) {
		unsigned a = keymap[i * nrow + j];
		if (a - KEYPAD_UP < 4)
			a = num_turn[rotate][a - KEYPAD_UP];
		else if (a - KEYPAD_1 < 9)
			a = num_turn[rotate][a - KEYPAD_1 + 4];
		if (a > 0xff) a = 0xff;
		dest[j * 8 + i] = a;
		if (a == KEYPAD_CENTER) flags = 1;
	}
	if (sys_data.keyflags)
		flags = sys_data.keyflags;
	return flags;
}

struct sys_data sys_data;

#if !CHIP
int _chip;
#endif

