#include "common.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)
#define ERR_EXIT(...) \
	do { fprintf(stderr, "!!! " __VA_ARGS__); exit(1); } while (0)

#define FATAL() ERR_EXIT("error at %s:%d\n", __FILE__, __LINE__)

uint32_t *pinmap_addr;

void scan_firmware(intptr_t fw_addr) {
	int i, j; short *keymap = NULL;
	uint32_t *pinmap = NULL;
	uint32_t time0 = sys_timer_ms();
	int flags = 3;

	for (i = 0; i < FIRMWARE_SIZE - 0x1000; i += 4) {
		volatile short *s = (short*)(fw_addr + i);
		// "DRPS", "CAPN" : start of compressed data
		if (MEM4(s) == 0x53505244 && MEM4(s + 8) == 0x4e504143) break;

		// keymap filter
		if (flags & 1)
		if (MEM4(s + 38) == 0xffffffff) {
			uint32_t mask0 = 0, mask1 = 0, empty = 0, tmp;

#define CHECK_KEY \
	empty -= a >> 31; \
	if (a != -1) { \
		if ((unsigned)(a - 1) > (unsigned)0x39 - 1) break; \
		tmp = 1u << (a & 31); \
		if (a & 32) { \
			if (mask1 & tmp) break; \
			mask1 |= tmp; \
		} else { \
			if (mask0 & tmp) break; \
			mask0 |= tmp; \
		} \
	}

			if (_chip != 1) {
				for (j = 0; j < 64; j += 8) {
					int k, empty_ = empty, m0 = mask0, m1 = mask1;
					if (*(int32_t*)(s + j + 6) != -1) break;
					for (k = 0; k < 6; k++) {
						int a = s[j + k];
						CHECK_KEY
					}
					if (k < 6) {
						empty = empty_; mask0 = m0; mask1 = m1;
						break;
					}
				}
			} else
				for (j = 0; j < 40; j++) {
					int a = s[j];
					CHECK_KEY
				}
#undef CHECK_KEY
			if (j >= 40) do {
				if (empty < 10) break;
				tmp = 1 << 1 | 1 << 8 | 1 << 9; // DIAL, LSOFT, RSOFT
				if ((mask0 & tmp) != tmp) break;
				tmp = 0x03ff0000 | 1 << ('*' & 31) | 1 << ('#' & 31);
				if ((mask1 & tmp) != tmp) break;
				if (keymap) {
					DBG_LOG("!!! found two keymaps (%p, %p)\n", (void*)keymap, (void*)s);
					flags &= ~1; keymap = NULL;
				} else keymap = (short*)s;
			} while (0);
		}

		do {
			volatile uint32_t *p = (uint32_t*)s;
			if (*p >> 12 != 0x8c000000 >> 12) break;
			for (j = 2; ; j += 2) {
				uint32_t a = p[j];
				if (a == -1u && p[j + 1] == -1u) {
					if (j < 20) break;
					if (pinmap)
						ERR_EXIT("found two pinmaps (%p, %p)\n", (void*)pinmap, (void*)p);
					pinmap = (uint32_t*)p;
					i = (uintptr_t)(p + j) - fw_addr;
					break;
				}
				if (a >> 12 == 0x8c000000 >> 12) continue;
				if (a >> 12 == 0x82001000 >> 12) continue;
				break;
			}
		} while (0);
	}
	DBG_LOG("scan_firmware: %dms\n", sys_timer_ms() - time0);
	if (flags & 1) {
		if (keymap) DBG_LOG("keymap = %p\n", (void*)keymap);
		else DBG_LOG("!!! keymap not found\n");
	}
	if (pinmap) DBG_LOG("pinmap = %p\n", (void*)pinmap);
	else ERR_EXIT("pinmap not found\n");
	if (!sys_data.keymap_addr)
		sys_data.keymap_addr = keymap;
	pinmap_addr = pinmap;
}

// system timer, 1ms step
uint32_t sys_timer_ms(void) {
	uint32_t a, b = MEM4(0x8100300c);
	do a = b, b = MEM4(0x8100300c); while (a != b);
	return a;
}

void sys_wait_ms(uint32_t delay) {
	uint32_t start = sys_timer_ms();
	while (sys_timer_ms() - start < delay);
}

void sys_wait_us(uint32_t delay) {
	// delay *= 208;
	delay = (delay + delay * 4 + delay * 8) << 4;
	// SC6531: 312MHz
	if (_chip == 2) delay += delay >> 1;
	sys_wait_clk(delay);
}

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

int sys_getkeymap(uint8_t *dest) {
	short *keymap = sys_data.keymap_addr;
	int i, j, rotate = sys_data.rotate >> 4 & 3;
	static const unsigned char num_turn[4][4 + 9] = {
		{ "\004\005\006\007123456789" },
		{ "\007\006\004\005369258147" },
		{ "\005\004\007\006987654321" },
		{ "\006\007\005\004741852963" } };
	int nrow = _chip != 1 ? 8 : 5;
	int ncol = _chip != 1 ? sys_data.keycols : 8;
	int flags = 0;

	memset(dest, 0, 64);

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
	return flags;
}

struct sys_data sys_data;

#if !CHIP
int _chip;

struct chip_fn2 chip_fn[2] = {
#define X(pref) { \
	pref##sys_start, pref##sys_event, \
	pref##sys_framebuffer, pref##sys_brightness, \
	pref##sys_start_refresh, pref##sys_wait_refresh }
	X(sc6531e_), X(sc6531da_)
#undef X
};
#endif

