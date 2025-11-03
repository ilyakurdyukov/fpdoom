#include "common.h"
#include "syscode.h"
#include "sdio.h"

#define SDIO_USE_DMA 1

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

#define SDIO0_BASE ((sdio_base_t*)0x20300000)

#define SDIO_BASE_FREQ 26000000

typedef volatile struct {
	uint32_t blk_cnt, blk_size, arg, tr_mode;
	uint32_t resp[4];
	uint32_t dummy1, state, ctrl1, clk_ctrl;
	uint32_t int_st, int_en, int_sig, ctrl2;
	uint32_t cap1, cap2, dummy2[2];
	uint32_t force_evt, adma_err, adma2_addr_l, adma2_addr_h;
} sdio_base_t;

enum {
	SDIO_CMD_DATA = 1 << 21,
	SDIO_CMD_IND_CHK = 1 << 20,
	SDIO_CMD_CRC_CHK = 1 << 19,
	// set for all commands without data
	SDIO_SUB_CMD_FLAG = 1 << 18,

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
	
	SDIO_INT_AXI_RESP_ERR = 1 << 28,
	SDIO_INT_RESP_ERROR = 1 << 27,

	SDIO_INT_ADMA_ERROR = 1 << 25,
	SDIO_INT_AUTO_CMD12 = 1 << 24,
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
	sdio->clk_ctrl |= 1 << 24;
	while (sdio->clk_ctrl & 1 << 24);
}

static void sdio_sdclk_on(sdio_base_t *sdio) {
	sdio->clk_ctrl |= 1; // INT_CLK_EN
	while (!(sdio->clk_ctrl & 2)); // INT_CLK_STABLE
	sdio->clk_ctrl |= 4; // SDCLK_EN
}

static inline void sdio_sdclk_off(sdio_base_t *sdio) {
	sdio->clk_ctrl &= ~4; // SDCLK_EN
	sdio->clk_ctrl &= ~1; // INT_CLK_EN
}

static void sdio_pin_init(void) {
	unsigned i;
	for (i = 0; i < 6 * 4; i += 4)
		MEM4(0x402a0120 + i) = 0;
	for (i = 0; i < 5 * 4; i += 4)
		MEM4(0x402a0520 + i) = 0x100080;
	MEM4(0x402a0520 + i) = 0x180000;
}

static inline
void sdio_sdfreq_val(sdio_base_t *sdio, unsigned val) {
	sdio_sdclk_off(sdio);
	sdio->clk_ctrl = (sdio->clk_ctrl & ~0xffc0) | (val & 0xff) << 8 | (val >> 8) << 6;
	sdio_sdclk_on(sdio);
}

__attribute__((unused))
static void sdio_sdfreq(sdio_base_t *sdio, unsigned freq) {
	unsigned j = 1, base = SDIO_BASE_FREQ;

	// BROM, firmware and docs use different methods
#if 1
	j = (base - 1) / freq;
	if (j > 0x3ff) j = 0x3ff;
	base /= j + 1;
#else
	while (freq < base && j < 256) j <<= 1, base >>= 1;
	j >>= 1;
#endif
	SDIO_LOG("sdio FREQ: %u -> %u (0x%02x)\n", freq, base, j);

	// SDCLK_FRQ_SEL
	sdio_sdfreq_val(sdio, j);
}

static inline void sdio_data_timeout(sdio_base_t *sdio, int cnt) {
	sdio->clk_ctrl = (sdio->clk_ctrl & ~(15 << 16)) | cnt << 16;
}

static inline void sdio_cmd23_en(sdio_base_t *sdio, int val) {
	sdio->ctrl2 = (sdio->ctrl2 & ~(1 << 27)) | val << 27;
}

void sdio_init(void) {
	sdio_base_t *sdio = SDIO0_BASE;

	// enable
	MEM4(0x20e01000) = 0x80;

	// reset
	MEM4(0x20e01004) = 0x800;
	DELAY(0x100)
	MEM4(0x20e02004) = 0x800;

	// LDO: 3V
	//adi_write(0x40608d04, ((3000 - 2000) * 2 + 24) / 25); // sdcore
	//adi_write(0x40608d10, ((3000 - 1613) * 2 + 24) / 25); // sdio
	SDIO_LOG("SD power: core = %dmV, io = %dmV\n",
		(adi_read(0x40608d04) * 25) / 2 + 2000,
		(adi_read(0x40608d10) * 25) / 2 + 1613);

	sdio_pin_init();

	// SD power on
	adi_write(0x40608d00, adi_read(0x40608d00) & ~1); // sdcore
	adi_write(0x40608d0c, adi_read(0x40608d0c) & ~1); // sdio

	// set base freq
	{
		unsigned freq = SDIO_BASE_FREQ;
		int i;
		switch (freq) {
		case 1000000: i = 0; break;
		case 26000000: i = 1; break; // seems to be 28M
		case 307200000: i = 2; break;
		case 384000000: i = 3; break;
		case 390000000: i = 4; break;
		case 409600000: i = 5; break;
		default:
			DBG_LOG("unknown SDIO base freq\n");
			for (;;);
		}
		MEM4(0x2150006c) &= ~7;
		MEM4(0x2150006c) |= i;
	}

	sdio_sw_reset(sdio);

	sdio_sdfreq(sdio, 400000);

	sdio_data_timeout(sdio, 8);
	sdio_cmd23_en(sdio, 0);
}

#define SDIO_MAKE_CMD(cmd, resp) (cmd << 24 | SDIO_CMD_RESP_##resp)
#define SDIO_CMD0_TR SDIO_MAKE_CMD(0, 0)
#define SDIO_CMD0_INT SDIO_INT_CMD_COMPLETE

#define SDIO_INT_COMMON SDIO_INT_CMD_COMPLETE | SDIO_INT_RESP_ERROR | \
	SDIO_INT_CMD_INDEX | \
	SDIO_INT_CMD_END | SDIO_INT_CMD_CRC | SDIO_INT_CMD_TIMEOUT

#define SDIO_CMD2_INT SDIO_INT_CMD_COMPLETE | SDIO_INT_RESP_ERROR | \
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
#define SDIO_CMD24_TR SDIO_MAKE_CMD(24, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK | \
	SDIO_CMD_DMA | SDIO_CMD_DATA

#define SDIO_CMD6_INT SDIO_INT_CMD_COMPLETE | SDIO_INT_RESP_ERROR | \
	SDIO_INT_CMD_END | SDIO_INT_CMD_CRC | SDIO_INT_CMD_TIMEOUT | \
	SDIO_INT_DATA_END | SDIO_INT_DATA_CRC | SDIO_INT_DATA_TIMEOUT | \
	SDIO_INT_TR_COMPLETE | SDIO_INT_DMA
#define SDIO_CMD17_INT SDIO_CMD6_INT
#define SDIO_CMD24_INT (SDIO_CMD6_INT) & ~SDIO_INT_DATA_END

#define SDIO_ACMD6_TR SDIO_MAKE_CMD(6, 48) | SDIO_CMD_IND_CHK | SDIO_CMD_CRC_CHK
#define SDIO_ACMD41_TR SDIO_MAKE_CMD(41, 48)
#define SDIO_ACMD41_INT SDIO_INT_CMD_COMPLETE | SDIO_INT_RESP_ERROR | \
	SDIO_INT_CMD_END | SDIO_INT_CMD_TIMEOUT

static uint32_t sdio_cmd_impl(uint32_t cmd_tr, uint32_t arg,
		uint32_t cmd_int, void *data, int size, uint32_t *resp) {
	sdio_base_t *sdio = SDIO0_BASE;
	uint32_t tmp;
	int cnt = 1; // TODO

	SDIO_LOG("sdio CMD%u: arg = 0x%08x, tr = 0x%06x, int = 0x%08x\n",
			cmd_tr >> 24, arg, cmd_tr & 0xffffff, cmd_int);

	//sdio_data_timeout(sdio, 8);

	// needed for IRQ signals
	//sdio->int_sig = 0;
	sdio->int_en = 0;
	sdio->int_st = ~0; // clear interrupt status

	// cmd_int = 0x1bff01ff;
	sdio->int_en |= cmd_int;
	//sdio->int_sig |= cmd_int;

	if (data) {
		sdio->blk_size = size;
		sdio->blk_cnt = cnt;
		sdio->adma2_addr_l = (uint32_t)data;
		sdio->adma2_addr_h = 0;
		clean_invalidate_dcache_range(data, (char*)data + size * cnt);
	}
	sdio->arg = arg;
	sdio->tr_mode = cmd_tr;

	tmp = data ? SDIO_INT_TR_COMPLETE | SDIO_INT_ERR :
			SDIO_INT_CMD_COMPLETE | SDIO_INT_ERR;
	do cmd_int = sdio->int_st;
	while (!(cmd_int & tmp));
	SDIO_LOG("sdio INT: 0x%08x\n", cmd_int);

#if 0 // unnecessary
	//sdio->int_sig = 0;
	sdio->int_en = 0;
	sdio->int_st = ~0; // clear interrupt status
#endif

	if (!(cmd_int & SDIO_INT_ERR) && resp) {
		resp[0] = sdio->resp[0];
		resp[1] = sdio->resp[1];
		resp[2] = sdio->resp[2];
		resp[3] = sdio->resp[3];
		SDIO_LOG("sdio RESP: %08x %08x %08x %08x\n",
			resp[0], resp[1], resp[2], resp[3]);
	}
	return cmd_int;
}

static inline uint32_t sdio_cmd(uint32_t cmd_tr, uint32_t arg,
		uint32_t cmd_int, void *data, int size, uint32_t *resp) {
	if (!(cmd_tr & SDIO_CMD_DATA)) cmd_tr |= SDIO_SUB_CMD_FLAG;
	return sdio_cmd_impl(cmd_tr, arg, cmd_int, data, size, resp);
}

#ifdef SDIO_SHL
#define sdio_shl (*(volatile uint32_t*)SDIO_SHL)
#else
unsigned sdio_shl;
#endif

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
			sys_wait_ms(20);
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
		{
			unsigned delay = 500;
			// Timeout usually means that the SD card is not inserted.
			if (!(sd_int & SDIO_INT_CMD_TIMEOUT)) {
				if (!(sd_int & SDIO_INT_CMD_COMPLETE)) break;
				if ((int32_t)resp[0] < 0) goto acmd51_done;
				// Some junk cards take more than half a second to be ready.
				delay = 5000;
			}
			if (sys_timer_ms() - time0 > delay) break;
			sys_wait_ms(20);
		}
	}
	DBG_LOG("sdio: ACMD%u failed\n", 41);
	return 1;

acmd51_done:
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
		//sdio->ctrl1 |= 4; // HI_SPD_EN
		for (i = 0; i < 2; i++) {
			// memset(data, 0, 64);
			j = 0;
			do *(uint32_t*)(data + j) = 0;
			while ((j += 4) < 64);
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

int sdio_write_block(uint32_t idx, uint8_t *buf) {
	uint32_t sd_int;
	idx <<= sdio_shl;
	sd_int = sdio_cmd(SDIO_CMD24_TR, idx, SDIO_CMD24_INT, buf, 512, NULL);
	if (!(sd_int & SDIO_INT_CMD_COMPLETE)) {
		DBG_LOG("sdio: CMD%u failed\n", 24);
		return -1;
	}
	return 0;
}
