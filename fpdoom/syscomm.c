#include "common.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)
#define ERR_EXIT(...) \
	do { fprintf(stderr, "!!! " __VA_ARGS__); exit(1); } while (0)

uint32_t *pinmap_addr;

static int check_keymap(const void *buf) {
	const uint16_t *s = (const uint16_t*)buf;
	uint32_t a, tmp, t0 = 0, t1 = ~0;
	unsigned i, n, nrow, ncol;
	n = _chip == 1 ? 48 : 64;
	for (i = 0; i < n; i++) {
		a = s[i];
		if (a == 0xffff) { t0++; continue; }
		if (a - 0x69 < 5) continue;
		if (a - 0x70 < 3) continue;
		if (a - 1 >= 0x39) break;
	}
	for (n = 7; n < i; n += 8)
		t1 &= *(int32_t*)&s[n - 1];
	if (_chip == 1) {
		ncol = 8; nrow = 5;
		if (i < 40) {
			if (i < 35) return 0;
			ncol = 7;
		} else if (!~t1) ncol = i >> 3, nrow = 8;
	} else {
		if (i < 40 || ~t1) return 0;
		ncol = i >> 3;
		nrow = 8;
	}
	n = nrow * ncol;
	if (t0 < n - 32) return 0;
	t0 = 0; t1 = 0;
	for (i = 0; i < n; i++) {
		a = s[i];
		if (a >= 0x40) continue;
		tmp = 1u << (a & 31);
		if (a & 32) t1 |= tmp; else t0 |= tmp;
	}
	do {
		// DBG_LOG("t0 = 0x%x, t1 = 0x%x\n", t0, t1);
		// exception: Children's Camera
		if (t0 == 0x20f2 && !t1) break;
		// exception: Prestigio Wize J1
		if (t0 == 0x63f2 && t1 == 0x3fe0400) break;
		t0 |= ~(1 << 8); // LSOFT
		tmp = 0x03ff0000 | 1 << ('*' & 31) | 1 << ('#' & 31);
		t1 |= ~tmp;
		if (~(t1 & t0)) return 0;
	} while (0);
	// DBG_LOG("keyrows = %u, keycols = %u\n", nrow, ncol);
	if (!sys_data.keyrows) sys_data.keyrows = nrow;
	if (!sys_data.keycols) sys_data.keycols = ncol;
	return n;
}

#define LZMA_BASE ((lzma_base_t*)0x20e00000)

typedef volatile struct {
	uint32_t ctrl, ctrl_sts, int_mask, src_addr;
	uint32_t dst_addr, src_len, dst_len, buf_addr;
	uint32_t buf_len, buf_start, proc_pos, trans_len;
	uint32_t unzip_size;
} lzma_base_t;

int sys_lzma_decode(const uint8_t *src, unsigned src_size,
		uint8_t *dst, unsigned dst_size) {
	lzma_base_t *lzma = LZMA_BASE;
	uint32_t err;

	if (_chip == 1) {
		// LZMA enable
		MEM4(0x20501080) = 0x10000;

		// LZMA reset
		MEM4(0x20501040) = 0x2000;
		DELAY(100)
		MEM4(0x20502040) = 0x2000;

		// 26M_TUNED, 26M, 104M, 208M
		MEM4(0x8d200088) |= 3; // clk 208M
	} else {
		// LZMA enable
		MEM4(0x20500060) = 0x10000;

		// LZMA reset
		MEM4(0x20500020) = 0x2000;
		DELAY(100)
		MEM4(0x20500030) = 0x2000;

		{
			uint32_t m = 1;
			if (_chip != 2) m <<= 30;
			// 26M, 208M, 104M, unknown
			MEM4(0x8b000040) =
					(MEM4(0x8b000040) & ~(m << 1)) | m;
		}
	}

	//memset(dst, 0, dst_size);
	lzma->src_addr = (uint32_t)src;
	lzma->src_len = src_size;
	lzma->dst_addr = (uint32_t)dst;
	lzma->dst_len = dst_size;

	// int clear
	lzma->int_mask |= 0x1f << 8;
	//clean_invalidate_dcache_range(dst, dst + dst_size);
	clean_invalidate_dcache();
	lzma->ctrl = lzma->ctrl_sts | 1; // start
	do err = lzma->int_mask;
	while (!(err & 1 << 24));

	err = err >> 24 & 0x1f;
	if (dst_size != lzma->unzip_size) err |= 0x20;

	// LZMA disable
	{
		uint32_t addr = 0x20502080;
		if (_chip != 1) addr = 0x20500070;
		MEM4(addr) = 0x10000;
	}
	return err;
}

static short* scan_drps(uint32_t *drps) {
	uint32_t *colb; uint8_t *dst = NULL;
	unsigned colb_offs, src_offs, src_size, dst_size;
	short *keymap = NULL;
	DBG_LOG("scan_drps(%p)\n", (void*)drps);
	do {
		uint32_t *p, *end; int ret;
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
		ret = sys_lzma_decode((uint8_t*)drps + src_offs, src_size, dst, dst_size);
		if (ret != 1) break;
		end = (uint32_t*)(dst + dst_size - 128 - 12);
		for (p = (uint32_t*)dst; p < end; p++) {
			// keymap filter
			if (p[0] == 0x4d && p[1] == 0x400 && p[2] == 0xa) {
				ret = check_keymap(p + 3);
				if (ret) {
					if (keymap) {
						DBG_LOG("!!! found two keymaps (kern + 0x%x, kern + 0x%x)\n",
								 (uint8_t*)keymap - dst, (uint8_t*)p - dst);
						keymap = NULL; break;
					}
					keymap = (short*)(p + 3);
					ret = (ret + 1) & ~1; // keep 32-bit alignment
					p = (uint32_t*)(keymap + ret) - 1;
					continue;
				}
			}
		}
		if (keymap) {
			short *buf = (short*)sys_data.keytrn;
			int n = sys_data.keyrows * sys_data.keycols;
			memcpy(buf, keymap, n * 2);
			DBG_LOG("keymap = kern + 0x%x\n", (uint8_t*)keymap - dst);
			keymap = buf;
		}
	} while (0);
	free(dst);
	return keymap;
}

#define DRPS_LIM 0x11000

void scan_firmware(intptr_t fw_addr) {
	short *keymap = NULL; int flags = 1;
	uint32_t *pinmap = NULL;
	uint32_t time0 = sys_timer_ms();
	uint32_t *trapgami = (uint32_t*)(fw_addr + DRPS_LIM);
	uint32_t *p, *end = (uint32_t*)(fw_addr + 0x200000);

	for (p = (uint32_t*)fw_addr; p < end; p++) {
		// "DRPS", "CAPN" : start of compressed data
		if (p[0] == 0x53505244 && p[4] == 0x4e504143) break;

		// "TRAPGAMI": offsets to "DRPS" sections
		if (p < trapgami) do {
			if (p[0] != 0x50415254) break;
			if (p[1] != 0x494d4147) break;
			// p[2] == ~0 if there's no image
			if (p[2] & 0xff000003) break;
			trapgami = p;
		} while (0);

		// keymap filter
		if (p[0] == 0x4d && p[1] == 0x400 && p[2] == 0xa) {
			int ret = check_keymap(p + 3);
			if (ret) {
				if (keymap) {
					DBG_LOG("!!! found two keymaps (%p, %p)\n", (void*)keymap, (void*)p);
					flags &= ~1; keymap = NULL;
				} else if (flags & 1) {
					keymap = (short*)(p + 3);
					p = (uint32_t*)(keymap + ret) - 1;
					continue;
				}
			}
		}

		do {
			int j;
			if (*p >> 12 != 0x8c000000 >> 12) break;
			for (j = 2; ; j += 2) {
				uint32_t a = p[j];
				if (a == -1u && p[j + 1] == -1u) {
					if (j < 20) break;
					if (pinmap)
						ERR_EXIT("found two pinmaps (%p, %p)\n", (void*)pinmap, (void*)p);
					pinmap = (uint32_t*)p;
					p += j;
					break;
				}
				if (a >> 12 == 0x8c000000 >> 12) continue;
				if (a >> 12 == 0x82001000 >> 12) continue;
				break;
			}
		} while (0);
	}
	// TODO: enable QPI mode for SPI flash for faster scanning
	if (flags & 1) {
		if (keymap) DBG_LOG("keymap = %p\n", (void*)keymap);
		else if (!sys_data.keymap_addr &&
				(intptr_t)trapgami - fw_addr < DRPS_LIM)
			keymap = scan_drps((uint32_t*)(fw_addr + trapgami[2]));
		if (!keymap) DBG_LOG("!!! keymap not found\n");
	}
	time0 = sys_timer_ms() - time0;
	if (pinmap) DBG_LOG("pinmap = %p\n", (void*)pinmap);
	else ERR_EXIT("pinmap not found\n");
	DBG_LOG("scan_firmware: %dms\n", time0);
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
	static const unsigned char num_turn[][4 + 9] = {
		{ "\004\005\006\007123456789" },
		{ "\007\006\004\005369258147" },
		{ "\005\004\007\006987654321" },
		{ "\006\007\005\004741852963" },
#if LIBC_SDIO == 0
		{ "\004\005\006\007123745698" },
#endif
	};
	int nrow = sys_data.keyrows;
	int ncol = sys_data.keycols;
	int flags = 0;

#if LIBC_SDIO == 0
	/* special case for LONG-CZ J9 */
	if (sys_data.lcd_id == 0x1306) rotate = 4;
#endif

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

