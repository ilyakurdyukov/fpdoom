#include "common.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* from assembly code */
uint32_t get_cpsr(void);
void set_cpsr_c(uint32_t a);

#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)
#define ERR_EXIT(...) do { \
	fprintf(stderr, "!!! " __VA_ARGS__); \
	exit(1); \
} while (0)

#define FATAL() ERR_EXIT("error at syscode.c:%d\n", __LINE__)

#if defined(CHIP_AUTO) && CHIP == 2
#define IS_SC6530 ((int32_t)(MEM4(0x205003fc) << 15) >= 0)
#else
#define IS_SC6530 (CHIP == 3)
#endif

#if SYSCODE_AUTO
uint32_t adi_read(uint32_t addr) {
	uint32_t a, rd_cmd = 0x82000008; // REG_ADI_RD_CMD
	if (_chip != 1) rd_cmd += 0x10;
	MEM4(rd_cmd) = addr & 0xfff;
	// REG_ADI_RD_DATA, BIT_RD_CMD_BUSY
	while ((a = MEM4(rd_cmd + 4)) >> 31);
	return a & 0xffff;
}

void adi_write(uint32_t addr, uint32_t val) {
	uint32_t fifo_sts = 0x82000004; // REG_ADI_FIFO_STS
	uint32_t bit = 1 << 22; // BIT_FIFO_FULL
	if (_chip != 1) fifo_sts += 0x1c, bit >>= 13;
	while (MEM4(fifo_sts) & bit);
	MEM4(addr) = val;
	while (!(MEM4(fifo_sts) & bit >> 1)); // BIT_FIFO_EMPTY
}
#endif

#if 0
#if CHIP == 1 // SC6531E
#define REG_ADI_FIFO_STS 0x82000004
#define BIT_FIFO_EMPTY (1u << 21)
#define BIT_FIFO_FULL (1u << 22)

#define REG_ADI_RD_CMD 0x82000008
#define REG_ADI_RD_DATA 0x8200000c

#elif CHIP == 2 || CHIP == 3 // SC6530, SC6531DA
#define REG_ADI_FIFO_STS 0x82000020
#define BIT_FIFO_EMPTY (1u << 8)
#define BIT_FIFO_FULL (1u << 9)

#define REG_ADI_RD_CMD 0x82000018
#define REG_ADI_RD_DATA 0x8200001c
#endif
#define BIT_RD_CMD_BUSY (1u << 31)

#define adi_read adi_read_static
#define adi_write adi_write_static
#if 0 // simple code
static uint32_t adi_read(uint32_t addr) {
	uint32_t a;
	MEM4(REG_ADI_RD_CMD) = addr & 0xfff;
	while ((a = MEM4(REG_ADI_RD_DATA)) & BIT_RD_CMD_BUSY);
	return a & 0xffff;
}

static void adi_write(uint32_t addr, uint32_t val) {
	while (MEM4(REG_ADI_FIFO_STS) & BIT_FIFO_FULL);
	MEM4(addr) = val;
	while (!(MEM4(REG_ADI_FIFO_STS) & BIT_FIFO_EMPTY));
}
#else
static uint32_t adi_read(uint32_t addr) {
	uint32_t a, t0, t1;
	// if ((addr ^ 0x82001000) >> 12) FATAL();
	t0 = t1 = sys_timer_ms();
	while (!(MEM4(REG_ADI_FIFO_STS) & BIT_FIFO_EMPTY)) {
		t1 = sys_timer_ms();
		if (t1 - t0 > 60) FATAL();
	}
	MEM4(REG_ADI_RD_CMD) = addr &= 0xfff;
	t0 = t1;
	while ((a = MEM4(REG_ADI_RD_DATA)) & BIT_RD_CMD_BUSY) {
		t1 = sys_timer_ms();
		if (t1 - t0 > 60) FATAL();
	}
#if CHIP == 1
	if (a >> 16 != addr) FATAL();
#else
	if ((a >> 16 & 0x1fff) != addr) FATAL();
#endif
	return a & 0xffff;
}

static void adi_write(uint32_t addr, uint32_t val) {
	uint32_t t0, t1;
	t0 = t1 = sys_timer_ms();
	while (MEM4(REG_ADI_FIFO_STS) & BIT_FIFO_FULL) {
		t1 = sys_timer_ms();
		if (t1 - t0 > 60) FATAL();
	}
	MEM4(addr) = val;
	t0 = t1;
	while (!(MEM4(REG_ADI_FIFO_STS) & BIT_FIFO_EMPTY)) {
		t1 = sys_timer_ms();
		if (t1 - t0 > 60) FATAL();
	}
}
#endif
#endif

#if CHIP == 1 // SC6531E
#define ANA_REG_BASE 0x82001400
#define ANA_CHIP_ID_LOW (ANA_REG_BASE + 0x000)
#define ANA_CHIP_ID_HIGH (ANA_REG_BASE + 0x004)
#define ANA_WHTLED_CTRL (ANA_REG_BASE + 0x13c)
#define ANA_CHGR_CTRL0 (ANA_REG_BASE + 0x150)
#elif CHIP == 2 || CHIP == 3 // SC6530, SC6531DA
#define ANA_REG_BASE 0x82001000
#define ANA_CHIP_ID_HIGH (ANA_REG_BASE + 0x000)
#define ANA_CHIP_ID_LOW (ANA_REG_BASE + 0x004)
#define ANA_CHGR_CTRL0 (ANA_REG_BASE + 0x200)
#define ANA_WHTLED_CTRL (ANA_REG_BASE + 0x220)
#define ANA_LED_CTRL (ANA_REG_BASE + 0x320)
#endif

#define AHB_CR(x) MEM4(0x20500000 + (x))
#define AHB_PWR_ON(x) AHB_CR(_chip != 1 ? 0x60 : 0x1080) = (x)
#define AHB_PWR_OFF(x) AHB_CR(_chip != 1 ? 0x70 : 0x2080) = (x)
#define APB_PWR_ON(x) MEM4(0x8b000000 + (_chip != 1 ? 0xa0 : 0x10a8)) = (x)
#define APB_PWR_OFF(x) MEM4(0x8b000000 + (_chip != 1 ? 0xa4 : 0x20a8)) = (x)

#if !SYSCODE_AUTO
static void init_chip_id(void) {
	uint32_t t0, t1;
#if CHIP == 1
	t1 = MEM4(0x8b00035c);	// 0x36320000
	t0 = MEM4(0x8b000360);	// 0x53433635
	t0 = (t0 >> 8) << 28 | (t0 << 28) >> 4;
	t0 |= (t1 & 0xf000000) >> 4 | (t1 & 0xf0000);
	t1 = MEM4(0x8b000364);	// 0x00000001
	t0 |= t1;
#elif CHIP == 2 || CHIP == 3
	t0 = MEM4(0x205003fc);
#endif
	sys_data.chip_id.num = t0;
	DBG_LOG("chip_id: %s = 0x%x\n", "num", t0);

	t1 = 0x7fffffff;
	switch (t0) {
#if CHIP == 2 || CHIP == 3
	case 0x65300000: t1 = 0x90001; break; // SC6530
	case 0x6530c000: t1 = 0x90001; break; // SC6530C
	case 0x65310000: t1 = 0x90003; break; // SC6531
	case 0x65310001: t1 = 0x90003; break; // SC6531BA
#elif CHIP == 1
	case 0x65620000: t1 = 0xa0001; break; // SC6531EFM
	case 0x65620001: t1 = 0xa0002; break; // SC6531EFM_AB
#endif
	default: FATAL();
	}

	sys_data.chip_id.ver = t1;
	DBG_LOG("chip_id: %s = 0x%x\n", "ver", t1);

#if CHIP == 1
	t0 = MEM4(0x20500168);
	DBG_LOG("chip_id: %s = 0x%x\n", "ahb", t0);
#endif

	t0 = adi_read(ANA_CHIP_ID_HIGH) << 16;
	t0 |= adi_read(ANA_CHIP_ID_LOW);
	sys_data.chip_id.adi = t0;
	DBG_LOG("chip_id: %s = 0x%x\n", "adi", t0);
#if CHIP == 2 || CHIP == 3
	// 0x11300000: SR1130
	// 0x1130c000: SR1130C
	// 0x11310000: SR1131
	// 0x11310001: SR1131BA
#elif CHIP == 1
	// 0x1161a000: SC1161
#endif
}

#define gpio_get(id, off) gpio_set(id, off, -1)
static int gpio_set(unsigned id, unsigned off, int state) {
	uint32_t addr, tmp, shl = id & 0xf;
#if CHIP == 1
	if (id >= 0x4c) FATAL();
	addr = 0x8a000000 + ((id >> 4) << 7);
#elif CHIP == 2 || CHIP == 3
	if (id >= (IS_SC6530 ? 0xa0 : 0x90)) FATAL();
	addr = id >> 7 ?
		(0x82001780 - (8 << 6)) + ((id >> 4) << 6) :
		0x8a000000 + ((id >> 4) << 7);
#endif
	addr += off;
#if CHIP == 2 || CHIP == 3
	if (id >> 7) tmp = adi_read(addr);
	else
#endif
	tmp = MEM4(addr);
	if (state < 0) return tmp >> shl & 1;
	tmp &= ~(1u << shl);
	tmp |= state << shl;
#if CHIP == 2 || CHIP == 3
	if (id >> 7) adi_write(addr, tmp);
	else
#endif
	MEM4(addr) = tmp;
	return 0;
}

static void gpio_init(void *ptr) {
	struct {
		int16_t id, val; uint32_t x04, x08;
	} *tab = ptr;
	int i;

	APB_PWR_ON(0x800080);	// GPIO_D

	for (i = 0; tab[i].id != -1; i++) {
		int a = tab[i].id;
		gpio_set(a, 4, 1);	// GPIODMSK
		if (!tab[i].x04) {
			gpio_set(a, 8, 1); // GPIODIR
			gpio_set(a, 0x18, 0);	// GPIOIE
			if (tab[i].val != -1)
				gpio_set(a, 0, (tab[i].val & 0xff) != 0); // GPIODATA
		} else {
			gpio_set(a, 8, 0); // GPIODIR
		}
	}
}

static void pin_init(void) {
	const volatile uint32_t *pinmap = pinmap_addr;
#if CHIP == 1
	// ANA_LDO_SF_REG0
	uint32_t ldo_sf0 = adi_read(0x8200148c);
#endif
	int sc6530_fix = IS_SC6530 && !sys_data.spi;

	for (;;) {
		uint32_t addr = pinmap[0], val = pinmap[1];
		pinmap += 2;
		// workaround for Samsung GT-E1272
		// hangs shortly after this pin is enabled
		if (sc6530_fix && addr == 0x8c000290) continue;
		if (addr - 0x82001000 < 0x1000) {
			adi_write(addr, val & 0xffff);
		} else if (addr >> 12 == 0x8c000) {
#if CHIP == 1
			if (addr - 0x8c000114 <= 0x18) {
				if (ldo_sf0 & 1 << 9)
					val |= 0x200000;
				else if (sys_data.chip_id.ver == 0xa0001)
					val |= 0x280000;
			}
#endif
			MEM4(addr) = val;
		} else break;
	}
	if (sys_data.gpio_init)
		gpio_init((void*)pinmap);
}

void CHIP_FN(sys_brightness)(unsigned val) {
	unsigned tmp, val2;
	if (val > 100) val = 100;

#if CHIP == 1
	val2 = val = (val * 16 + 50) / 100;
	tmp = adi_read(ANA_WHTLED_CTRL) & ~0x1f;
	if (!val) tmp |= 1; else val--;
	adi_write(ANA_WHTLED_CTRL, tmp | val << 1);
#elif CHIP == 2
	val2 = val = (val * 32 + 50) / 100;
	tmp = adi_read(ANA_WHTLED_CTRL) & ~0x5f;
	if (val) val += 0x40 - 1;
	tmp |= IS_SC6530 ? 0x1100 : 0x1200; /* turn off flashlight */
	adi_write(ANA_WHTLED_CTRL, tmp | val);
#endif
	if (sys_data.bl_gpio != 0xff) {
		int id = sys_data.bl_gpio;
		// check GPIODMSK and GPIODIR
		if (!gpio_get(id, 4) || !gpio_get(id, 8)) FATAL();
		gpio_set(id, 0, val2 != 0); // GPIODATA
	}
}
#endif

#define LCM_REG_BASE 0x20800000
#define LCM_CR(x) MEM4(LCM_REG_BASE + (x))
#define LCDC_CR(x) MEM4(0x20d00000 + (x))

enum {
	LCDC_CTRL = 0x00,
	LCDC_DISP_SIZE = 0x04,
	LCDC_LCM_START = 0x08,
	LCDC_LCM_SIZE = 0x0c,
	LCDC_BG_COLOR = 0x10,
	LCDC_FIFO_STATUS = 0x14,

	LCDC_OSD0_CTRL = 0x20,
	LCDC_OSD0_BASE_ADDR = 0x24,
	LCDC_OSD0_ALPHA_BASE_ADDR = 0x28,
	LCDC_OSD0_SIZE_XY = 0x2c,
	LCDC_OSD0_PITCH = 0x30,
	LCDC_OSD0_DISP_XY = 0x34,

	LCDC_OSD1_CTRL = 0x50,
	LCDC_OSD1_BASE_ADDR = 0x54,
	LCDC_OSD1_SIZE_XY = 0x5c,
	LCDC_OSD1_PITCH = 0x60,
	LCDC_OSD1_DISP_XY = 0x64,
	LCDC_OSD1_ALPHA = 0x68,
	LCDC_OSD1_GREY_RGB = 0x6c,
	LCDC_OSD1_CK = 0x70,

	LCDC_OSD4_CTRL = 0xe0,
	LCDC_OSD4_LCM_ADDR = 0xe4,
	LCDC_OSD4_DISP_XY = 0xe8,
	LCDC_OSD4_SIZE_XY = 0xec,
	LCDC_OSD4_PITCH = 0xf0,

	LCDC_IRQ_EN = 0x110,
	LCDC_IRQ_CLR = 0x114,
	LCDC_IRQ_STATUS = 0x118,
	LCDC_IRQ_RAW = 0x11c,
};

#if !SYSCODE_AUTO
typedef struct {
	uint32_t id, id_mask;
	uint16_t cs, extra02; uint32_t extra04;
	// dimensions
	uint16_t width, height, dim08, dim0c, dim10;
	struct { uint16_t /* ns */
		rcss, /* read CS setup */
		rlpw, /* read low pulse width */
		rhpw, /* read high pulse width */
		wcss, /* write CS setup */
		wlpw, /* write low pulse width */
		whpw; /* write high pulse width */
	} mcu_timing;
	struct { uint32_t freq; } spi;
	uint16_t mac_arg;
	const uint8_t *cmd_init;
} lcd_config_t;

static struct {
	const lcd_config_t *lcd;
	uint32_t cs, addr[2];
} lcd_setup;

static void lcm_config_addr(unsigned cs) {
	uint32_t tmp = cs << 26 | 0x60000000;
	lcd_setup.addr[0] = tmp;
	lcd_setup.addr[1] = tmp | 0x20000;
}

static void lcm_set_mode(unsigned a0) {
	uint32_t cs = lcd_setup.cs;
	while (LCM_CR(0) & 2);
	MEM4(LCM_REG_BASE + 0x10 + (cs << 4)) = a0 ? 1 : 0x28;
}

static void lcm_send(unsigned idx, unsigned val) {
	//lcm_set_mode(1);
	while (LCM_CR(0) & 2);
	MEM2(lcd_setup.addr[idx]) = val;
}

static uint16_t lcm_recv(unsigned idx) {
	//lcm_set_mode(1);
	while (LCM_CR(0) & 2);
	return MEM2(lcd_setup.addr[idx]);
}

static inline void lcm_send_cmd(unsigned val) { lcm_send(0, val); }
static inline void lcm_send_data(unsigned val) { lcm_send(1, val); }

static void lcm_reset(unsigned delay, unsigned method) {
	uint32_t addr = 0x8b000224, t;
	int i;
	method &= 1;
	for (i = 0; i < 3; i++) {
		t = MEM4(addr) & ~method;
		t |= method ^= 1;
		MEM4(addr) = t;
		sys_wait_ms(delay);
	}
}

static uint32_t get_cpu_freq(void) {
	static const unsigned tab[4] = {
#if CHIP == 1 // SC6531E
		26000000, 104000000, 208000000, 208000000
#elif CHIP == 2 || CHIP == 3 // SC6531DA
		26000000, 208000000, 249600000, 312000000
#endif
	};

	if (IS_SC6530) {
		static const unsigned tab[4] = {
			26000000, 208000000, 104000000, 156000000
		};
		return tab[MEM4(0x8b000040) & 3];
	}
	return tab[MEM4(0x8b00004c) & 3];
}

static uint32_t get_ahb_freq(void) {
	if (IS_SC6530) {
		uint32_t freq = get_cpu_freq();
		if (MEM4(0x8b000040) & 1 << 2) freq >>= 1;
		return freq;
	}
	if (MEM4(0x8b00004c) & 1 << 2) return 208000000 >> 1;
	return get_cpu_freq() >> 1;
}

// worst timings
static void lcm_set_safe_freq(unsigned cs) {
	uint32_t addr = LCM_REG_BASE + 0x10 + (cs << 4);
	uint32_t sum = 30 << 16 | 6 | 0x80;
	sum |= 30 << 21 | 14 << 8 | 6 << 4;
	MEM4(addr) = 1;
	MEM4(addr + 4) = sum;
}

static void lcm_set_freq(unsigned cs, uint32_t clk_rate, const lcd_config_t *lcd) {
	unsigned sum; int t1, t2, t3;

	if (clk_rate > 1000000) clk_rate /= 1000000;
	LCM_CR(0) = 0x11110000;

#define DBI_CYCLES(r, name, max) \
	r = (clk_rate * lcd->mcu_timing.name - 1) / 1000 + 1; \
	if (r > max) r = max;

	DBI_CYCLES(t1, rcss, 6) sum = t1;
	DBI_CYCLES(t2, rlpw, 14) sum += t2;
	DBI_CYCLES(t3, rhpw, 14) sum += t3;
	if (sum > 30) sum = 30;
	sum = sum << 16 | t1 | 0x80;

	DBI_CYCLES(t1, wcss, 6) sum |= t1 << 4;
	DBI_CYCLES(t2, wlpw, 14) sum |= t2 << 8;
	DBI_CYCLES(t3, whpw, 14)
	t2 += t3 - 1 > t1 ? t3 - 1 : t1 + 1;
	// can't happen: 14 + (max(14 - 1, 6 + 1)) < 30
	//if (t2 > 30) t2 = 30;
	sum |= t2 << 21;
#undef DBI_CYCLES

	while (LCM_CR(0) & 2);

	lcd_setup.lcd = lcd;
	lcd_setup.cs = cs;

	MEM4(LCM_REG_BASE + 0x14 + (cs << 4)) = sum;
}

// ID0: LCD module's manufacturer ID
// ID1: LCD module/driver version ID
// ID2: LCD module/driver ID

static uint32_t lcd_cmdret(int cmd, unsigned n) {
	uint32_t ret = 0;
	lcm_send_cmd(cmd);
	do ret = ret << 8 | (lcm_recv(1) & 0xff); while (--n);
	return ret;
}

static unsigned lcd_getid(void) {
#if 1
	return lcd_cmdret(0x04, 4) & 0xffffff;
#else
	unsigned id = 0, i;
	for (i = 0; i < 3; i++) {
		lcm_send_cmd(0xda + i);
		lcm_recv(1); // dummy
		id = id << 8 | (lcm_recv(1) & 0xff);
	}
	return id;
#endif
}

#define LCM_CMD(cmd, len) 0x80 | len, cmd
#define LCM_DATA(len) len
#define LCM_DELAY(delay) 0x40 | (delay >> 8 & 0x1f), (delay & 0xff)
#define LCM_END 0

#include "lcd_config.h"
#include "lcd_spi.h"

static void lcm_exec(const uint8_t *cmd) {
	void (*send_fn)(unsigned, unsigned) = lcm_send;

	if (sys_data.spi) send_fn = spi_send;

	for (;;) {
		uint32_t a, len;
		a = *cmd++;
		if (!a) break;
		len = a & 0x1f;
		a >>= 5;
		if (a == 4) {
			send_fn(0, *cmd++);
			a = 0;
		}
		if (a == 0) {
			while (len--) send_fn(1, *cmd++);
		} else if (a == 2) {
			sys_wait_ms(len << 8 | *cmd++);
		} else FATAL();
	}
}

static unsigned lcd_getid2(void) {
	unsigned id;

	// Manufacturer Command Access Protect
	lcm_send_cmd(0xb0);
	lcm_send_data(0x00);
	// Device Code Read
	// 0x0122xxxx: RenesasSP
	// 0x0204xxxx: Ilitek
	id = lcd_cmdret(0xbf, 5);
	return id;
}

#if 0
void lcm_send_rgb16(uint16_t *ptr, uint32_t size) {
	uint32_t addr, tmp;
	//lcm_set_mode(1);
	addr = lcd_setup.addr[1];
	while (size--) {
		tmp = *ptr++;
		while (LCM_CR(0) & 2);
		MEM2(addr) = tmp >> 8;
		while (LCM_CR(0) & 2);
		MEM2(addr) = tmp & 0xff;
	}
}
#endif

static int is_whtled_on(void) {
#if CHIP == 1
	return (adi_read(ANA_WHTLED_CTRL) & 1) == 0;
#elif CHIP == 2 || CHIP == 3
	return (adi_read(ANA_LED_CTRL) & 4) == 0;
#endif
}

static void lcm_init(void) {
	uint32_t id, clk_rate, cs = sys_data.lcd_cs;
	int i, n = sizeof(lcd_config) / sizeof(*lcd_config);

	AHB_PWR_ON(0x40);	// LCM enable

	LCM_CR(0) = 0;
	LCM_CR(0x10) = 1;
	LCM_CR(0x14) = 0xa50100;

	if (IS_SC6530 || !is_whtled_on()) lcm_reset(32, 0);

	clk_rate = get_ahb_freq();
	DBG_LOG("LCD: clk_rate = %u\n", clk_rate);

	if (sys_data.spi) {
		lcd_spi_init(sys_data.spi, clk_rate);
		return;
	}

	lcm_set_safe_freq(cs);
	lcm_config_addr(cs);
	id = sys_data.lcd_id;
	if (!id) id = lcd_getid();
	if (!id) id = lcd_getid2();
	// workaround for NT35310
	else if (id == 0x8000) id = lcd_cmdret(0xd4, 4);
	DBG_LOG("LCD: id = 0x%06x\n", id);

	for (i = 0; i < n; i++)
		if ((id & lcd_config[i].id_mask) == lcd_config[i].id &&
				lcd_config[i].mcu_timing.rcss) break;
	if (i == n) ERR_EXIT("unknown LCD\n");

	lcm_set_freq(cs, clk_rate, lcd_config + i);
	lcm_config_addr(cs);
	lcm_set_mode(1);
}

void CHIP_FN(sys_start_refresh)(void) {
	int mask = 1;	// osd == 3 ? 2 : /* osd0 */ 1
	clean_dcache();
	if (sys_data.spi)
		spi_refresh_next(sys_data.spi);
	LCDC_CR(LCDC_IRQ_EN) |= mask;
	LCDC_CR(LCDC_CTRL) |= 8;	// start refresh
}

void CHIP_FN(sys_wait_refresh)(void) {
	int mask = 1;	// osd == 3 ? 2 : /* osd0 */ 1

	if (!(LCDC_CR(LCDC_IRQ_EN) & mask)) return;

	while ((LCDC_CR(LCDC_IRQ_RAW) & mask) == 0);
	LCDC_CR(LCDC_IRQ_CLR) |= mask;
}

static void lcd_init(void) {
	const lcd_config_t *lcd = lcd_setup.lcd;
	unsigned w = lcd->width, h = lcd->height, x2, y2, w2, h2;
	unsigned rotate, mac_arg;
	const uint8_t rtab[4] = { 0, 0xa0, 0xc0, 0x60 };
	static uint8_t cmd_init2[] = {
		// MY, MX, MV, ML, RB, MH, 0, 0
		LCM_CMD(0x36, 1), 0, // Memory Access Control
		LCM_CMD(0x2a, 4), 0,0,0,0, // Column Address Set
		LCM_CMD(0x2b, 4), 0,0,0,0, // Page Address Set
		LCM_CMD(0x2c, 0), // Memory Write
		LCM_END
	};

	rotate = sys_data.rotate & 3;
	mac_arg = lcd->mac_arg;
	if (sys_data.mac) mac_arg = sys_data.mac;
	mac_arg ^= rtab[rotate];
	if (mac_arg & 0x20) {
		unsigned t = w; w = h; h = t;
	}
	x2 = 0; w2 = w - 1;
	y2 = 0; h2 = h - 1;
	// fix for ST7735
	if (lcd->id == 0x7c89f0) {
		w -= x2 = w & 3;
		h -= y2 = h & 3;
	}
	sys_data.display.w1 = w;
	sys_data.display.h1 = h;

	lcm_exec(lcd->cmd_init);
	if (lcd->id == 0x289328) {
		/* Why can't people use standardized commands? */
		static uint8_t cmd_init2_ili9328[] = {
#define X(a, b) LCM_CMD(0x00, 0), LCM_CMD(a, b)
			// ORG, 0, I/D1, I/D0, AM, 0, 0, 0
			X(0x03, 2), 0x10,0, // Entry Mode
			X(0x50, 2), 0,0, // Horizontal Address Start Position
			X(0x51, 2), 0,0, // Horizontal Address End Position
			X(0x52, 2), 0,0, // Vertical Address Start Position
			X(0x53, 2), 0,0, // Vertical Address End Position
			X(0x20, 2), 0,0, // Horizontal GRAM Address Set
			X(0x21, 2), 0,0, // Vertical GRAM Address Set
			X(0x22, 0), // Write Data to GRAM
#undef X
			LCM_END
		};
		uint8_t *p = cmd_init2_ili9328;
		p[5] = mac_arg >> 2; p += 6;
		if (mac_arg & 0x20) {
			unsigned t;
			t = w2; w2 = h2; h2 = t;
			t = x2; x2 = y2; x2 = t;
		}
		p[4] = x2 >> 8; p[5] = x2; p += 6;
		p[4] = w2 >> 8; p[5] = w2; p += 6;
		p[4] = y2 >> 8; p[5] = y2; p += 6;
		p[4] = h2 >> 8; p[5] = h2; p += 6;
		if (!(mac_arg & 0x40)) p[4] = w2 >> 8, p[5] = w2;
		p += 6;
		if (!(mac_arg & 0x80)) p[4] = h2 >> 8, p[5] = h2;
		lcm_exec(cmd_init2_ili9328);
	} else {
		uint8_t *p = cmd_init2;
		p[2] = mac_arg; p += 3;
		p[2] = x2 >> 8; p[3] = x2;
		p[4] = w2 >> 8; p[5] = w2; p += 6;
		p[2] = y2 >> 8; p[3] = y2;
		p[4] = h2 >> 8; p[5] = h2;
		lcm_exec(cmd_init2);
	}
}

static void lcdc_init(void) {
	struct sys_display *disp = &sys_data.display;
	unsigned w = disp->w1, h = disp->h1;
	unsigned w2 = disp->w2, h2 = disp->h2;

	AHB_PWR_ON(0x1000);	// LCDC enable
	// AHB_SOFT_RST?
	AHB_CR(_chip != 1 ? 0x20 : 0x1040) = 0x200;
	sys_wait_ms(10);
	AHB_CR(_chip != 1 ? 0x30 : 0x2040) = 0x200;

	LCDC_CR(LCDC_CTRL) |= 1; // lcdc_enable = 1

	// INT_IRQ_ENABLE
	// MEM4(0x80000000 + 8) |= 0x400000;

	LCDC_CR(LCDC_CTRL) |= 2; // fmark_mode = 1
	LCDC_CR(LCDC_CTRL) &= ~4; // fmark_pol = 0
	LCDC_CR(LCDC_BG_COLOR) = 0x000000;

	LCDC_CR(LCDC_DISP_SIZE) = w | h << 16;
	LCDC_CR(LCDC_LCM_START) = 0;
	LCDC_CR(LCDC_LCM_SIZE) = w | h << 16;

	{
		uint32_t addr, mode;
		if (!sys_data.spi) {
			lcm_set_mode(0);
			mode = 2; addr = lcd_setup.addr[1];
		} else {
			spi_refresh_init(sys_data.spi);
			mode = 0; addr = sys_data.spi + SPI_TXD;
		}
		LCDC_CR(LCDC_OSD4_CTRL) |= 0x20;
		LCDC_CR(LCDC_OSD4_CTRL) |= (LCDC_CR(LCDC_OSD4_CTRL) & ~(3 << 6)) | mode << 6;
		LCDC_CR(LCDC_CTRL) &= ~(7 << 5);
		LCDC_CR(LCDC_OSD4_LCM_ADDR) = addr >> 2;
	}

	CHIP_FN(sys_start_refresh)();
	CHIP_FN(sys_wait_refresh)();
	LCDC_CR(LCDC_IRQ_EN) &= ~1;

	{
		uint32_t a = LCDC_CR(LCDC_OSD0_CTRL);
		int fmt = 5;
		//a |= 1;
		a &= ~2; // disable color key
		a |= 4; // block alpha
		a = (a & ~(15 << 4)) | fmt << 4; // format
		a = (a & ~(3 << 8)) | 2 << 8; // little endian
		LCDC_CR(LCDC_OSD0_CTRL) = a;
	}

	LCDC_CR(LCDC_OSD0_PITCH) = w2;

	w2 = w2 > w ? w : w2;
	h2 = h2 > h ? h : h2;
	w2 &= ~1;	// must be aligned

	//LCDC_CR(LCDC_OSD0_BASE_ADDR) = (uint32_t)base >> 2;
	LCDC_CR(LCDC_OSD0_SIZE_XY) = w2 | h2 << 16;
	LCDC_CR(LCDC_OSD0_DISP_XY) = ((w - w2) >> 1) | ((h - h2) >> 1) << 16;
}

void CHIP_FN(sys_framebuffer)(void *base) {
	struct sys_display *disp = &sys_data.display;
	unsigned w = disp->w1, w2 = disp->w2;
	unsigned offset = (w2 - w) >> 1;

	if (w2 < w) offset = 0;

	LCDC_CR(LCDC_OSD0_BASE_ADDR) = ((uint32_t)base + offset * 2) >> 2;
	LCDC_CR(LCDC_OSD0_CTRL) |= 1;
}
#endif

#if SYSCODE_AUTO
static int keypad_read_pb(void) {
	uint32_t val, addr = 0x82001200, ch = 1;
	if (_chip != 1) addr = 0x82001900, ch = 3;
	// EIC_DBNC_DMSK
	adi_write(addr + 4, adi_read(addr + 4) | 1 << ch);
	// EIC_DBNC_DATA
	val = adi_read(addr) >> ch & 1;
	// inverted on SC6531DA
	if (_chip == 2) val ^= 1;
	return val;
}

static void eic_enable(void) {
	// EIC_A
	if (_chip == 1) {
		adi_write(0x82001410, adi_read(0x82001410) | 8);
		adi_write(0x82001408, adi_read(0x82001408) | 8);
	} else {
		adi_write(0x820010e4, 0x20);
		adi_write(0x820010e0, 0x80);
	}
	// EIC_D, not really needed
	//APB_PWR_ON(0x4000000);
	//APB_PWR_ON(0x2000000);
}

void keypad_init(void) {
	keypad_base_t *kpd = KEYPAD_BASE;
	short *keymap = sys_data.keymap_addr;
	int i, j, row = 0, col = 0, ctrl;
	int nrow = sys_data.keyrows;
	int ncol = sys_data.keycols;

	for (i = 0; i < ncol; i++)
	for (j = 0; j < nrow; j++)
		if (keymap[i * nrow + j] != -1)
			col |= 1 << i, row |= 1 << j;

	if (_chip != 1)
		row &= 0xfc, col &= 0xfc;	// why?

	//DBG_LOG("keypad: row_mask = 0x%02x, col_mask = 0x%02x\n", row, col);

	APB_PWR_ON(0x80040);

	kpd->int_clr = 0xfff;
	kpd->clk_divide = 1;
	kpd->debounce = 16;
	kpd->int_en = 0xfff;
	kpd->polarity = 0xffff;

	ctrl = kpd->ctrl;
	//DBG_LOG("keypad: ctrl = %08x\n", ctrl);
	ctrl |= 1; // enable
	ctrl &= ~2; // sleep
	ctrl |= 4; // long
	ctrl |=	row << 16 | col << 8;
	kpd->ctrl = ctrl;

	// Required to check the status of the power button.
	eic_enable();
}

int sys_event(int *rkey) {
	static int static_i = 0;
	static uint32_t static_ev, static_st;
	static uint8_t pb_saved[8] = { 0 };
	uint32_t event, status;
	int i = static_i;
	uint16_t *keytrn;

	if (!sys_data.keymap_addr) return EVENT_END;
	event = static_ev;
	status = static_st;
	if (!i) {
		keypad_base_t *kpd = KEYPAD_BASE;
		event = kpd->int_raw;
		status = kpd->key_status;
		event &= 0xff;
		if (!event) return EVENT_END;
		kpd->int_clr = 0xfff;
		if (status & 8) return EVENT_END;
		event = (event & 0xfff) | keypad_read_pb() << 12;
		static_ev = event;
		static_st = status;
	}

	for (; i < 8; i++) {
		if (event >> i & 1) {
			uint32_t k = status >> ((i & 3) * 8);
			keytrn = sys_data.keytrn[0];
			k = (k & 0x70) >> 1 | (k & 7);
			if (i < 4) {
				if (event >> 12)
					pb_saved[k >> 3] |= 1 << (k & 7), keytrn += 64;
			} else {
				uint8_t *p = &pb_saved[k >> 3];
				unsigned a = *p, b = 1 << (k & 7);
				if (a & b) *p = a ^ b, keytrn += 64;
			}
			k = keytrn[k];
			if (k) {
				*rkey = k;
				static_i = i + 1;
				return i < 4 ? EVENT_KEYDOWN : EVENT_KEYUP;
			}
		}
	}
	static_i = 0;
	return EVENT_END;
}
#endif

#if !SYSCODE_AUTO
void keypad_init(void);

void CHIP_FN(sys_init)(void) {

#if CHIP == 2 || CHIP == 3
	// will allow to remove the battery after boot
	if (!IS_SC6530 && sys_data.charge >= 2) {
		// CHGR_CTRL0:CHGR_CC_EN = 1
		adi_write(ANA_CHGR_CTRL0, adi_read(ANA_CHGR_CTRL0) | 1);
		// CHGR_CTRL0:CHGR_CC_I = 300mA
		adi_write(ANA_CHGR_CTRL0, (adi_read(ANA_CHGR_CTRL0) & ~(0xf << 4)) | 0 << 4);
		// CHGR_CTRL0:CHGR_PD = 1
		adi_write(ANA_CHGR_CTRL0, adi_read(ANA_CHGR_CTRL0) | 1 << 8);
		sys_wait_ms(10);
		// CHGR_CTRL0:CHGR_PD = 0
		adi_write(ANA_CHGR_CTRL0, adi_read(ANA_CHGR_CTRL0) & ~(1 << 8));
		// CHGR_CTRL0:RECHG = 1
		adi_write(ANA_CHGR_CTRL0, adi_read(ANA_CHGR_CTRL0) | 1 << 12);
	} else
#endif
	if (sys_data.charge >= 0) {
		unsigned charger_pd = sys_data.charge < 1;
		unsigned sh = _chip == 1 ? 0 : 8;	// CHGR_PD
		uint32_t val = adi_read(ANA_CHGR_CTRL0) & ~(1 << sh);
		adi_write(ANA_CHGR_CTRL0, val | charger_pd << sh);
	}

	init_chip_id();
	pin_init();
	lcm_init();
	lcd_init();
	lcd_appinit();
	lcdc_init();
	if (sys_data.keymap_addr) {
		keypad_init();
		keytrn_init();
	}
	sys_data.init_done = 1;
}
#endif // CHIP

#if SYSCODE_AUTO
void sys_start(void) {
	sys_brightness(sys_data.brightness);
	if (sys_data.keymap_addr)
		KEYPAD_BASE->int_clr = 0xfff;
}

void sys_exit(void) {
	if (sys_data.init_done) {
		sys_brightness(0);
		LCDC_CR(LCDC_CTRL) &= ~1;
		AHB_PWR_OFF(0x1000 | 0x40); // LCDC, LCM
	}
	sys_wdg_reset(SYS_RESET_DELAY); // 0.5 sec
}

void sys_wdg_reset(unsigned val) {
	uint32_t wdg;
	// watchdog enable
	if (_chip == 1) {
		adi_write(0x82001408, adi_read(0x82001408) | 4);
		adi_write(0x82001410, adi_read(0x82001410) | 4);
		wdg = 0x82001040;
	} else {
		adi_write(0x820010e0, 4);
		adi_write(0x820010e4, 2);
		wdg = 0x82001480;
	}
	// set reset timer
	adi_write(wdg + 0x20, 0xe551); // LOCK
	adi_write(wdg + 8, adi_read(wdg + 8) | 9); // CTRL
	// 1 / 32768
	adi_write(wdg + 0, val & 0xffff); // LOAD_LOW
	adi_write(wdg + 4, val >> 16); // LOAD_HIGH
	adi_write(wdg + 8, adi_read(wdg + 8) | 2);
	adi_write(wdg + 0x20, ~0xe551);
	for (;;);
}
#endif

