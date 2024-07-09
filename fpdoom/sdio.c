#include "common.h"
#include "syscode.h"
#include "sdio.h"

#ifndef SDIO_USE_DMA
#define SDIO_USE_DMA 1
#endif

#ifndef SDIO_NO_PINMAP
#define SDIO_NO_PINMAP 0
#endif

#ifndef SDIO_HIGHSPEED
#define SDIO_HIGHSPEED 1
#endif

#if SDIO_VERBOSE > 0
#include <stdio.h>
#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DBG_LOG(...) (void)0
#endif

#if SDIO_VERBOSE > 1
int sdio_verbose = 1;
#define SDIO_LOG(...) do \
	if (sdio_verbose) DBG_LOG(__VA_ARGS__); while (0)
#else
#define SDIO_LOG(...) (void)0
#endif

#define IS_SC6530 (_chip == 3)

void clean_invalidate_dcache_range(void *start, void *end);

static uint32_t adi_read(uint32_t addr) {
	uint32_t a, rd_cmd = 0x82000008; // REG_ADI_RD_CMD
	if (_chip != 1) rd_cmd += 0x10;
	MEM4(rd_cmd) = addr & 0xfff;
	// REG_ADI_RD_DATA, BIT_RD_CMD_BUSY
	while ((a = MEM4(rd_cmd + 4)) >> 31);
	return a & 0xffff;
}

static void adi_write(uint32_t addr, uint32_t val) {
	uint32_t fifo_sts = 0x82000004; // REG_ADI_FIFO_STS
	uint32_t bit = 1 << 22; // BIT_FIFO_FULL
	if (_chip != 1) fifo_sts += 0x1c, bit >>= 13;
	while (MEM4(fifo_sts) & bit);
	MEM4(addr) = val;
	while (!(MEM4(fifo_sts) & bit >> 1)); // BIT_FIFO_EMPTY
}

#define SDIO0_BASE ((sdio_base_t*)0x20700000)
#define SDIO1_BASE ((sdio_base_t*)0x20700100)

typedef volatile struct {
	uint32_t dma_addr, blk_size, arg, tr_mode;
	uint32_t resp[4];
	uint32_t buf_port, state, ctrl1, ctrl2;
	uint32_t int_st, int_en, int_sig;
} sdio_base_t;

enum {
	SDIO_CMD_DATA = 1 << 21,
	SDIO_CMD_IND_CHK = 1 << 20,
	SDIO_CMD_CRC_CHK = 1 << 19,

	SDIO_CMD_RESP_0 = 0 << 16,
	SDIO_CMD_RESP_136 = 1 << 16,
	SDIO_CMD_RESP_48 = 2 << 16,
	SDIO_CMD_RESP_48_BUSY = 3 << 16,

	SDIO_CMD_COMP_ATA = 1 << 6,
	SDIO_CMD_MULT_BLK = 1 << 5,
	SDIO_CMD_DATA_READ = 1 << 4,

	SDIO_CMD_AUTO_CMD12 = 1 << 2,
	SDIO_CMD_BLK_CNT = 1 << 1,
	SDIO_CMD_DMA = 1 << 0,
	
	SDIO_INT_TARGET_RESP = 1 << 28,

	SDIO_INT_AUTO_CMD12 = 1 << 24,
	SDIO_INT_CUR_LIMIT = 1 << 23,
	SDIO_INT_DATA_END = 1 << 22,
	SDIO_INT_DATA_CRC = 1 << 21,
	SDIO_INT_DATA_TIMEOUT = 1 << 20,
	SDIO_INT_CMD_INDEX = 1 << 19,
	SDIO_INT_CMD_END = 1 << 18,
	SDIO_INT_CMD_CRC = 1 << 17,
	SDIO_INT_CMD_TIMEOUT = 1 << 16,
	SDIO_INT_ERR = 1 << 15,

	SDIO_INT_CARD = 1 << 8,
	SDIO_INT_DMA = 1 << 3,
	SDIO_INT_BLK_GAP_EVENT = 1 << 2,
	SDIO_INT_TR_COMPLETE = 1 << 1,
	SDIO_INT_CMD_COMPLETE = 1 << 0,
};

static void sdio_sw_reset(sdio_base_t *sdio) {
	sdio->ctrl2 |= 1 << 24; // SW_RST_ALL
	while (sdio->ctrl2 & 1 << 24);
}

static void sdio_intclk_on(sdio_base_t *sdio) {
	sdio->ctrl2 |= 1; // INT_CLK_EN
	while (!(sdio->ctrl2 & 2)); // INT_CLK_STABLE
}

static void sdio_sdclk_on(sdio_base_t *sdio) {
	sdio->ctrl2 |= 4; // SDCLK_EN
	while (!(sdio->ctrl2 & 2)); // INT_CLK_STABLE
}

static inline void sdio_sdclk_off(sdio_base_t *sdio) {
	sdio->ctrl2 &= ~4; // SDCLK_EN
}

static inline void sdio_data_timeout(sdio_base_t *sdio, int cnt) {
	uint32_t tmp = sdio->int_en;
	sdio->int_en = tmp & ~(1 << 20);
	// DATA_TIMEOUT_CNT
	sdio->ctrl2 = (sdio->ctrl2 & ~(15 << 16)) | cnt << 16;
	sdio->int_en = tmp;
}

static inline void sdio_blksize(sdio_base_t *sdio, int size, int count, int dma_size) {
	// 4K << dma_size
	uint32_t a = count << 16 | (size & 0x1000) << 3 | dma_size << 12 | (size & 0xfff);
	sdio->blk_size = a;
}

static void sdio_pin_init(int state) {
	// name: clk0, clk1, cmd, d0, d1, d2, d3
	// 6531E: 0xb0, NONE, 0xac, 0x9c, 0xa0, 0xa4, 0xa8
	// 6531: 0x278, 0x174, 0x27c, 0x280, 0x284, 0x288, 0x28c
	// 6530: 0x250, 0x254, 0x258, 0x25c, 0x260, 0x264, 0x268
#if SDIO_NO_PINMAP
	static const uint16_t init[6 * 3] = {
		0x3084, 0x3084, 0x3084, 0x3084, 0x3084, 0x2001, /* 6531e */
		0x8800, 0x8600, 0x8a00, 0x8a00, 0x8a00, 0x8a00, /* 6531 */
		0x0244, 0x0244, 0x0244, 0x0244, 0x0244, 0x0244, /* 6530 */
	};
	const uint16_t *p = init;
#endif
	uint32_t val = state ? 0x40 : 0x80, mask = 0xc0;
	uint32_t n = 6, addr = 0x8c00009c - 4, skip = 0;
	if (_chip != 1) {
		addr += 0x250 - 0x9c;
#if SDIO_NO_PINMAP
		p += 12;
#endif
		if (!IS_SC6530)
#if SDIO_NO_PINMAP
			p -= 6,
#endif
			addr += 0x278 - 0x250, val <<= 2, mask <<= 2;
	}
	for (;;) {
		addr += 4;
		if (addr == skip) continue;
#if SDIO_NO_PINMAP
		MEM4(addr) = (*p++ & ~mask) | val;
#else
		MEM4(addr) = (MEM4(addr) & ~mask) | val;
#endif
		if (!--n) break;
	}
	DELAY(100)
}

static void sdio_sdfreq(sdio_base_t *sdio, unsigned freq) {
	unsigned j = 1, base = 48000000;

	while (freq < base && j < 256) j <<= 1, base >>= 1;
	j >>= 1;
	SDIO_LOG("sdio FREQ: %u\n", base);

	sdio_sdclk_off(sdio);
	// SDCLK_FRQ_SEL
	sdio->ctrl2 = (sdio->ctrl2 & ~(0xff << 8)) | j << 8;
}

static void sdio_reset(void) {
	uint32_t addr1 = 0x20501040, addr2 = 0x20502040;
	if (_chip != 1) addr1 = 0x20500020, addr2 = addr1 + 0x10;

	// SDIO0 reset
	MEM4(addr1) = 0x100;
	DELAY(1000)
	MEM4(addr2) = 0x100;
	DELAY(1000)
}

static inline void sdio_slot(unsigned slot) {
	uint32_t mask = 3;
	if (IS_SC6530) mask <<= 2, slot <<= 2;
	MEM4(0x205000b0) = (MEM4(0x205000b0) & ~mask) | slot;
}

void sdio_init(void) {
	sdio_base_t *sdio;

	// SDIO0 enable
	if (_chip == 1)
		MEM4(0x20501080) = 0x200;
	else 
		MEM4(0x20500060) = IS_SC6530 ? 0x400 : 0x200;

	sdio_reset();
	sdio_slot(0);

	// SDIO0 freq 48000000
	if (_chip == 1)
		MEM4(0x8d200044) = (MEM4(0x8d200044) & ~7) | 2;
	else if (!IS_SC6530)
		MEM4(0x8b000044) = (MEM4(0x8b000044) & ~(3 << 23)) | 0 << 23;
	else
		MEM4(0x8b000040) = (MEM4(0x8b000040) & ~(3 << 28)) | 1 << 28;

	sdio = SDIO0_BASE;
	sdio_sw_reset(sdio);
	sdio_intclk_on(sdio);
	sdio_sw_reset(sdio);

	sdio_reset();

	sdio->ctrl1 = (sdio->ctrl1 & ~0x22) | 0; // 1-bit mode
	sdio->ctrl1 &= ~4; // HI_SPD_EN

	if (_chip == 1) {
		// Why is the default set to 3V when it should be 3.3V?
		// LDO_SD_REG1
		// adi_write(0x8200145c, 330 - 120);
		SDIO_LOG("SD power = %dmV\n", (adi_read(0x8200145c) + 120) * 10);
	}

	sdio_pin_init(0);

	// SD power on
	if (_chip == 1) {
		adi_write(0x82001460, adi_read(0x82001460) & ~1); // LDO_SD_REG2
	} else {
		adi_write(0x82001184, adi_read(0x82001184) & ~1);
		adi_write(0x820011a4, adi_read(0x820011a4) | 1);
	}

	sdio_sdfreq(sdio, 400000);
	sdio_intclk_on(sdio);
	sdio_sdclk_on(sdio);
	sys_wait_ms(350);

	sdio_data_timeout(sdio, 14);
}

#define SDIO_MAKE_CMD(cmd, resp) (cmd << 24 | SDIO_CMD_RESP_##resp)
#define SDIO_CMD0_TR SDIO_MAKE_CMD(0, 0)
#define SDIO_CMD0_INT SDIO_INT_CMD_COMPLETE

#define SDIO_INT_COMMON SDIO_INT_CMD_COMPLETE | SDIO_INT_TARGET_RESP | \
	SDIO_INT_CMD_INDEX | \
	SDIO_INT_CMD_END | SDIO_INT_CMD_CRC | SDIO_INT_CMD_TIMEOUT

#define SDIO_CMD2_INT SDIO_INT_CMD_COMPLETE | SDIO_INT_TARGET_RESP | \
	SDIO_INT_CMD_END | SDIO_INT_CMD_CRC | SDIO_INT_CMD_TIMEOUT
#define SDIO_CMD9_INT SDIO_CMD2_INT

#define SDIO_CMD2_TR SDIO_MAKE_CMD(2, 136) | SDIO_CMD_CRC_CHK
#define SDIO_CMD3_TR SDIO_MAKE_CMD(3, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK
#define SDIO_CMD7_TR SDIO_MAKE_CMD(7, 48_BUSY) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK
#define SDIO_CMD8_TR SDIO_MAKE_CMD(8, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK
#define SDIO_CMD9_TR SDIO_MAKE_CMD(9, 136) | SDIO_CMD_CRC_CHK
#define SDIO_CMD16_TR SDIO_MAKE_CMD(16, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK
#define SDIO_CMD55_TR SDIO_MAKE_CMD(55, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK

#if !SDIO_USE_DMA
#define SDIO_INT_DMA 0
#define SDIO_CMD_DMA 0
#endif

#define SDIO_CMD6_TR SDIO_MAKE_CMD(6, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK | \
	SDIO_CMD_DMA | SDIO_CMD_DATA_READ | SDIO_CMD_DATA
#define SDIO_CMD17_TR SDIO_MAKE_CMD(17, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK | \
	SDIO_CMD_DMA | SDIO_CMD_DATA_READ | SDIO_CMD_DATA

#define SDIO_CMD6_INT SDIO_INT_CMD_COMPLETE | SDIO_INT_TARGET_RESP | \
	SDIO_INT_CMD_END | SDIO_INT_CMD_CRC | SDIO_INT_CMD_TIMEOUT | \
	SDIO_INT_DATA_END | SDIO_INT_DATA_CRC | SDIO_INT_DATA_TIMEOUT | \
	SDIO_INT_TR_COMPLETE | SDIO_INT_DMA
#define SDIO_CMD17_INT SDIO_CMD6_INT

#define SDIO_ACMD6_TR SDIO_MAKE_CMD(6, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK
#define SDIO_ACMD41_TR SDIO_MAKE_CMD(41, 48)
#define SDIO_ACMD41_INT SDIO_INT_CMD_COMPLETE | SDIO_INT_TARGET_RESP | \
	SDIO_INT_CMD_END | SDIO_INT_CMD_TIMEOUT

static uint32_t sdio_cmd(uint32_t cmd_tr, uint32_t arg,
		uint32_t cmd_int, void *data, int size, uint32_t *resp) {
	sdio_base_t *sdio = SDIO0_BASE;
	uint32_t tmp;
	int cnt = 1; // TODO

	SDIO_LOG("sdio CMD%u: arg = 0x%08x, tr = 0x%06x, int = 0x%08x\n",
			cmd_tr >> 24, arg, cmd_tr & 0xffffff, cmd_int);

	//sdio_intclk_on(sdio);
	sdio_sdclk_on(sdio);
	//sdio_data_timeout(sdio, 14);

	tmp = 0x11ff01ff;
	// needed for IRQ signals
	//sdio->int_sig &= ~tmp;
	sdio->int_en &= ~tmp;
	sdio->int_st = tmp; // clear interrupt status

	// cmd_int = 0x11ff01ff;
	sdio->int_en |= cmd_int;
	//sdio->int_sig |= cmd_int;

	if (data) {
#if SDIO_USE_DMA
		clean_invalidate_dcache_range(data, (char*)data + size * cnt);
		sdio->dma_addr = (uint32_t)data;
#endif
		sdio_blksize(sdio, size, cnt, 7);
	}
	sdio->arg = arg;
	sdio->tr_mode = (sdio->tr_mode & 0xc004ff80) | cmd_tr;

#if SDIO_USE_DMA
	tmp = data ? SDIO_INT_TR_COMPLETE | SDIO_INT_ERR :
			SDIO_INT_CMD_COMPLETE | SDIO_INT_ERR;
	// IRQ: MEM4(0x80001004) & 1 << 8
	do cmd_int = sdio->int_st;
	while (!(cmd_int & tmp));
#else
	if (data) {
		int i; uint32_t *p = data;
		for (i = 0; i < size; i += 4) {
			cmd_int = sdio->int_st;
			if (cmd_int & SDIO_INT_ERR) break;
			do tmp = sdio->state;
			while (!(tmp & 1 << 11));
			*p++ = sdio->buf_port;
		}
	} else {
		tmp = SDIO_INT_CMD_COMPLETE | SDIO_INT_ERR;
		do cmd_int = sdio->int_st;
		while (!(cmd_int & tmp));
	}
#endif
	SDIO_LOG("sdio INT: 0x%08x\n", cmd_int);

#if 0 // unnecessary
	tmp = 0x11ff01ff;
	//sdio->int_sig &= ~tmp;
	sdio->int_en &= ~tmp;
	sdio->int_st = tmp; // clear interrupt status
#endif

	sdio->ctrl2 |= 3 << 25; // SW_RST_DAT | SW_RST_CMD
	while (sdio->ctrl2 & 3 << 25);

	if (!(cmd_int & SDIO_INT_ERR) && resp) {
		resp[0] = sdio->resp[0];
		resp[1] = sdio->resp[1];
		resp[2] = sdio->resp[2];
		resp[3] = sdio->resp[3];
		SDIO_LOG("sdio RESP: %08x %08x %08x %08x\n",
			resp[0], resp[1], resp[2], resp[3]);
	}

	sdio_sdclk_off(sdio);
	return cmd_int;
}

static unsigned sdio_shl;

int sdcard_init(void) {
	uint32_t resp[4], sd_int;
	uint32_t time0, sd_ver = 0, rca;
	sdio_base_t *sdio = SDIO0_BASE;

#if 0 // doesn't work
	if (!(sdio->int_st & SDIO_INT_CARD)) {
		DBG_LOG("sdcard not detected\n");
		return 1;
	}
#endif

	sdio_cmd(SDIO_CMD0_TR, 0, SDIO_CMD0_INT, NULL, 0, NULL);

	time0 = sys_timer_ms();
	for (;;) {
		sd_int = sdio_cmd(SDIO_CMD8_TR, 0x1aa, SDIO_INT_COMMON, NULL, 0, resp);
		if (sd_int & SDIO_INT_CMD_TIMEOUT) {
			if (sys_timer_ms() - time0 > 500) break;
			sys_wait_ms(5);
		} else if ((sd_int & SDIO_INT_CMD_COMPLETE) && resp[0] == 0x1aa) {
			sd_ver = 1;
			break;
		} else {
			DBG_LOG("sdio: CMD%u failed\n", 8);
			return 1;
		}
	}

	time0 = sys_timer_ms();
	for (;;) {
		sd_int = sdio_cmd(SDIO_CMD55_TR, 0, SDIO_INT_COMMON, NULL, 0, NULL);
		if (sd_int & SDIO_INT_CMD_COMPLETE)
			sd_int = sdio_cmd(SDIO_ACMD41_TR, 0xff8000 | sd_ver << 30, SDIO_ACMD41_INT, NULL, 0, resp);
		if ((sd_int & SDIO_INT_CMD_TIMEOUT) || ((sd_int & SDIO_INT_CMD_COMPLETE) && !(resp[0] >> 31))) {
			if (sys_timer_ms() - time0 > 500) break;
			sys_wait_ms(5);
		} else break;
	}
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: ACMD%u failed\n", 41);
		return 1;
	}

	sdio_shl = sd_ver == 1 ? resp[0] >> 30 & 0 : 9;

	// read CID
	sd_int = sdio_cmd(SDIO_CMD2_TR, 0, SDIO_CMD2_INT, NULL, 0, resp);
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: CMD%u failed\n", 2);
		return 1;
	}

	// read RCA
	sd_int = sdio_cmd(SDIO_CMD3_TR, 0, SDIO_INT_COMMON, NULL, 0, resp);
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: CMD%u failed\n", 3);
		return 1;
	}
	rca = resp[0] >> 16;
	SDIO_LOG("sdio RCA: 0x%04x\n", rca);
	rca <<= 16;

	// read CSD
	sd_int = sdio_cmd(SDIO_CMD9_TR, rca, SDIO_CMD9_INT, NULL, 0, resp);
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: CMD%u failed\n", 9);
		return 1;
	}

	sdio_sdfreq(sdio, 25000000);

	// select/deselect card
	sd_int = sdio_cmd(SDIO_CMD7_TR, rca, SDIO_INT_COMMON, NULL, 0, resp);
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: CMD%u failed\n", 7);
		return 1;
	}

#if 1
	// set bus width (4-bit)
	sd_int = sdio_cmd(SDIO_CMD55_TR, rca, SDIO_INT_COMMON, NULL, 0, NULL);
	if (sd_int & SDIO_INT_CMD_COMPLETE)
		sd_int = sdio_cmd(SDIO_ACMD6_TR, 2, SDIO_INT_COMMON, NULL, 0, resp);
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: ACMD%u failed\n", 6);
		return 1;
	}
	sdio->ctrl1 = (sdio->ctrl1 & ~0x22) | 2; // 4-bit mode
#endif

	// set block length
	sd_int = sdio_cmd(SDIO_CMD16_TR, 512, SDIO_INT_COMMON, NULL, 0, resp);
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: CMD%u failed\n", 16);
		return 1;
	}

#if SDIO_HIGHSPEED
	{
		uint8_t data[64] __attribute__((aligned(32)));
		int i, j;
		sdio->ctrl1 |= 4; // HI_SPD_EN
		for (i = 0; i < 2; i++) {
			memset(data, 0, 64);
			sd_int = sdio_cmd(SDIO_CMD6_TR, 0xfffff1 | i << 31, SDIO_CMD6_INT, data, 64, resp);
			if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
				DBG_LOG("sdio: CMD%u failed\n", 6);
				break;
			}
			if (0) for (j = 0; j < 64; j++)
				DBG_LOG("%02x%s", data[j], ~j & 15 ? " " : "\n");

			if ((data[0x10] & 15) != 1) break;
			// READ16_BE(data + 0xc) & 2
			if (!(data[0xd] & 2)) break;
		}
		if (i == 2) {
			sdio_sdfreq(sdio, 50000000);
			DBG_LOG("sdio: high speed enabled\n");
		}
	}
#endif
	return 0;
}

int sdio_read_block(uint32_t idx, uint8_t *buf) {
	uint32_t sd_int;
	idx <<= sdio_shl;
	sd_int = sdio_cmd(SDIO_CMD17_TR, idx, SDIO_CMD17_INT, buf, 512, NULL);
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: CMD%u failed\n", 17);
		return -1;
	}
	return 0;
}

