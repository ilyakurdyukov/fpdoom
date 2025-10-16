#include "common.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)
#define ERR_EXIT(...) do { \
	fprintf(stderr, "!!! " __VA_ARGS__); \
	exit(1); \
} while (0)

#define FATAL() ERR_EXIT("error at syscode.c:%d\n", __LINE__)

#define IS_SC6530 (_chip == 3)

#ifndef CHECK_ADI
#define CHECK_ADI 0
#endif

#if CHECK_ADI
#define ADI_WAIT_DEF uint32_t t0, t1;
#define ADI_WAIT_INIT t1 = sys_timer_ms();
#define ADI_WAIT(cond) t0 = t1; \
	while (cond) if ((t1 = sys_timer_ms()) - t0 > 60) FATAL();
#else
#define ADI_WAIT_DEF
#define ADI_WAIT_INIT
#define ADI_WAIT(cond) while (cond);
#endif

uint32_t adi_read(uint32_t addr) {
	uint32_t a, rd_cmd = 0x82000008; // REG_ADI_RD_CMD
#if CHECK_ADI
	uint32_t fifo_sts = rd_cmd - 4; // REG_ADI_FIFO_STS
	uint32_t bit = 1 << 21; // BIT_FIFO_EMPTY
	ADI_WAIT_DEF
	if (_chip != 1) rd_cmd += 0x10, fifo_sts += 0x1c, bit >>= 13;
	ADI_WAIT_INIT
	ADI_WAIT(!(MEM4(fifo_sts) & bit))
	// if ((addr ^ 0x82001000) >> 12) FATAL();
	MEM4(rd_cmd) = addr &= 0xfff;
	// REG_ADI_RD_DATA, BIT_RD_CMD_BUSY
	ADI_WAIT((a = MEM4(rd_cmd + 4)) >> 31);
	if (_chip != 1) a &= 0x1fffffff;
	if (a >> 16 != addr) FATAL();
#else
	if (_chip != 1) rd_cmd += 0x10;
	// if ((addr ^ 0x82001000) >> 12) FATAL();
	MEM4(rd_cmd) = addr & 0xfff;
	// REG_ADI_RD_DATA, BIT_RD_CMD_BUSY
	while ((a = MEM4(rd_cmd + 4)) >> 31);
#endif
	return a & 0xffff;
}

void adi_write(uint32_t addr, uint32_t val) {
	uint32_t fifo_sts = 0x82000004; // REG_ADI_FIFO_STS
	uint32_t bit = 1 << 22; // BIT_FIFO_FULL
	ADI_WAIT_DEF
	if (_chip != 1) fifo_sts += 0x1c, bit >>= 13;
	ADI_WAIT_INIT
	ADI_WAIT(MEM4(fifo_sts) & bit)
	MEM4(addr) = val;
	ADI_WAIT(!(MEM4(fifo_sts) & bit >> 1)) // BIT_FIFO_EMPTY
}

// ANA_REG_BASE
// SC6531E: 0x82001400
// SC6530, SC6531DA: 0x82001000

#define AHB_CR(x) MEM4(0x20500000 + (x))
#define AHB_PWR_ON(x) AHB_CR(_chip != 1 ? 0x60 : 0x1080) = (x)
#define AHB_PWR_OFF(x) AHB_CR(_chip != 1 ? 0x70 : 0x2080) = (x)
#define APB_PWR_ON(x) MEM4(0x8b000000 + (_chip != 1 ? 0xa0 : 0x10a8)) = (x)
#define APB_PWR_OFF(x) MEM4(0x8b000000 + (_chip != 1 ? 0xa4 : 0x20a8)) = (x)

static void init_chip_id(void) {
	uint32_t t0, t1, ver = 0, adi;
	if (_chip == 1) {
		t1 = MEM4(0x8b00035c);	// 0x36320000
		t0 = MEM4(0x8b000360);	// 0x53433635
		t0 = (t0 >> 8) << 28 | (t0 << 28) >> 4;
		t0 |= (t1 & 0xf000000) >> 4 | (t1 & 0xf0000);
		t1 = MEM4(0x8b000364);	// 0x00000001
		t0 |= t1;
		switch (t0) {
		case 0x65620000: ver = 0xa0001; break; // SC6531EFM
		case 0x65620001: ver = 0xa0002; break; // SC6531EFM_AB
		default: FATAL();
		}
		adi = 0x82001400 + 4; t1 = adi - 4;
	} else {
		t0 = MEM4(0x205003fc);
		switch (t0) {
		case 0x65300000: ver = 0x90001; break; // SC6530
		case 0x6530c000: ver = 0x90001; break; // SC6530C
		case 0x65310000: ver = 0x90003; break; // SC6531
		case 0x65310001: ver = 0x90003; break; // SC6531BA
		default: FATAL();
		}
		adi = 0x82001000; t1 = adi + 4;
	}
	// ANA_CHIP_ID_{HIGH,LOW}
	adi = adi_read(adi) << 16 | adi_read(t1);
	sys_data.chip_id.num = t0;
	sys_data.chip_id.ver = ver;
	sys_data.chip_id.adi = adi;
	DBG_LOG("chip_id: %s = 0x%x\n", "num", t0);
	DBG_LOG("chip_id: %s = 0x%x\n", "ver", ver);
	if (_chip == 1) {
		t0 = MEM4(0x20500168);
		DBG_LOG("chip_id: %s = 0x%x\n", "ahb", t0);
	}
	DBG_LOG("chip_id: %s = 0x%x\n", "adi", adi);
	// 0x11300000: SR1130
	// 0x1130c000: SR1130C
	// 0x11310000: SR1131
	// 0x11310001: SR1131BA
	// 0x1161a000: SC1161
}

#define gpio_get(id, off) gpio_set(id, off, -1)
static int gpio_set(unsigned id, unsigned off, int state) {
	uint32_t addr, tmp, shl = id & 0xf;
	unsigned n = 0x4c;
	if (_chip >= 2) { n = 0x90; if (IS_SC6530) n = 0xa0; }
	if (id >= n) FATAL();
	if (id >= 128)
		addr = (0x82001780 - (8 << 6)) + ((id >> 4) << 6);
	else addr = 0x8a000000 + ((id >> 4) << 7);
	addr += off;
	if (id >= 128) tmp = adi_read(addr);
	else tmp = MEM4(addr);
	if (state < 0) return tmp >> shl & 1;
	tmp &= ~(1u << shl);
	tmp |= state << shl;
	if (id >= 128) adi_write(addr, tmp);
	else MEM4(addr) = tmp;
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
	uint32_t sc6530_fix = 0;
	if (IS_SC6530) {
		// workaround for Samsung GT-E1272
		// hangs shortly after 0x8c000290 is set to 0x231
		sc6530_fix = 0x8c000290; // SPI0 pin
		// workaround for Samsung B310E
		// hangs shortly after 0x8c0002a4 is set to 0x231
		if (MEM4(0x205003fc) << 16) // SC6530C
			sc6530_fix += 0x2a4 - 0x290; // UART TX pin
	}

	for (;;) {
		uint32_t addr = pinmap[0], val = pinmap[1];
		pinmap += 2;
		if (addr - 0x82001000 < 0x1000) {
			adi_write(addr, val & 0xffff);
		} else if (addr >> 12 == 0x8c000) {
			if (addr == sc6530_fix) continue;
			if (_chip == 1 && addr - 0x8c000114 <= 0x18) {
				// ANA_LDO_SF_REG0
				if (adi_read(0x8200148c) & 1 << 9)
					val |= 0x200000;
				else if (sys_data.chip_id.ver == 0xa0001)
					val |= 0x280000;
			}
			MEM4(addr) = val;
		} else break;
	}
	if (sys_data.gpio_init)
		gpio_init((void*)pinmap);
}

void sys_brightness(unsigned val) {
	unsigned tmp, val2, ctrl;
	if (val > 100) val = 100;

	// ANA_WHTLED_CTRL
	if (_chip == 1) {
		ctrl = 0x82001400 + 0x13c;
		val2 = val = (val * 16 + 50) / 100;
		tmp = adi_read(ctrl) & ~0x1f;
		if (!val) tmp |= 1; else val--;
		adi_write(ctrl, tmp | val << 1);
	} else {
		ctrl = 0x82001000 + 0x220;
		val2 = val = (val * 32 + 50) / 100;
		tmp = adi_read(ctrl) & ~0x5f;
		if (val) val += 0x40 - 1;
		tmp |= IS_SC6530 ? 0x1100 : 0x1200; /* turn off flashlight */
		adi_write(ctrl, tmp | val);
	}
	if (sys_data.bl_gpio != 0xff) {
		int id = sys_data.bl_gpio;
		// check GPIODMSK and GPIODIR
		if (!gpio_get(id, 4) || !gpio_get(id, 8)) FATAL();
		gpio_set(id, 0, val2 != 0); // GPIODATA
	}
}

#define LCM_REG_BASE 0x20800000
#define LCM_CR(x) MEM4(LCM_REG_BASE + (x))

typedef struct {
	uint32_t id, id_mask;
	uint16_t width, height;
	uint16_t mac_arg;
	struct { uint8_t /* ns */
		rcss, /* read CS setup */
		rlpw, /* read low pulse width */
		rhpw, /* read high pulse width */
		wcss, /* write CS setup */
		wlpw, /* write low pulse width */
		whpw; /* write high pulse width */
	} mcu_timing;
	struct { uint32_t freq; } spi;
	const uint8_t *cmd_init;
} lcd_config_t;

static struct {
	void (*send)(unsigned, unsigned);
	uint32_t addr;
} lcd_setup;

static void lcm_config_addr(unsigned cs) {
	lcd_setup.addr = cs << 26 | 0x60000000;
}

static void lcm_set_mode(unsigned val) {
	uint32_t cs = sys_data.lcd_cs;
	while (LCM_CR(0) & 2);
	LCM_CR(0x10 + (cs << 4)) = val;
}

static void lcm_send(unsigned idx, unsigned val) {
	while (LCM_CR(0) & 2);
	MEM2(lcd_setup.addr | idx << 17) = val;
}

static uint16_t lcm_recv(unsigned idx) {
	while (LCM_CR(0) & 2);
	return MEM2(lcd_setup.addr | idx << 17);
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
#define X(a) (a / 26)
#define X2(x, a, b, c, d) uint32_t x = \
	X(a) | X(b) << 8 | X(c) << 16 | X(d) << 24;
	X2(t1, 260, 1040, 2080, 2080)
	X2(t2, 260, 2080, 2496, 3120)
	X2(t3, 260, 2080, 1040, 1560)
#undef X2
#undef X
	uint32_t t = t1, addr = 0x8b00004c;
	if (_chip >= 2) {
		t = t2; if (IS_SC6530) t = t3, addr -= 0xc;
	}
	return (t >> (MEM4(addr) & 3) * 8 & 0xff) * 2600000;
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
	MEM4(addr) = 1; // lcm_set_mode(1)
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

	LCM_CR(0x14 + (cs << 4)) = sum;
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

static const lcd_config_t* lcd_find_conf(uint32_t id, int mode) {
	const lcd_config_t *lcd_config = lcd_config1;
	int i, n = sizeof(lcd_config1) / sizeof(*lcd_config1);
	if (_chip != 1) {
		lcd_config = lcd_config2;
		n = sizeof(lcd_config2) / sizeof(*lcd_config2);
	}
	for (i = 0; i < n; i++)
		if ((id & lcd_config[i].id_mask) == lcd_config[i].id) {
			if (mode == 0) {
				if (lcd_config[i].mcu_timing.rcss) break;
			} else if (mode == 1) {
				if (lcd_config[i].spi.freq) break;
			} else break;
		}
	if (i == n) ERR_EXIT("unknown LCD\n");
	return lcd_config + i;
}

#include "lcd_spi.h"
#include "lcd_mono.h"

static void lcm_exec(const uint8_t *cmd) {
	void (*send_fn)(unsigned, unsigned) = lcd_setup.send;

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

static int is_whtled_on(void) {
	if (_chip == 1) // ANA_WHTLED_CTR
		return (adi_read(0x82001400 + 0x13c) & 1) == 0;
	else // ANA_LED_CTRL
		return (adi_read(0x82001000 + 0x320) & 4) == 0;
}

static const lcd_config_t* lcm_init(void) {
	uint32_t id, clk_rate, cs = sys_data.lcd_cs;
	const lcd_config_t *lcd;

	// auto detect SPI1 display
	if (_chip == 1 && !sys_data.spi) {
		uint32_t x = 0x8c000138;
		DBG_LOG("LCD: pins = 0x%x, 0x%x, 0x%x, 0x%x\n",
				MEM4(x), MEM4(x + 4), MEM4(x + 12), MEM4(x + 16));

		// REG: 0x138, 0x13c, 0x144, 0x148
		// ---
		// LCM: all 0x2014 or 0x2015
		// SPI1_MODE3: all 0x2004 (0x82004 for Nokia TA-1400)
		// SPI1_MODE1: 0x2074, 0x2004, 0x2004, 0x2074
		// GPIO (LONG-CZ J9): all 0x2031

		// SPI1_CLK/LCMCD
		if ((MEM4(0x8c00013c) & 0x3fff) == 0x2004) {
			sys_data.spi = 0x69 << 24;
			// SPI1_DI/LCMWR/---/GPIO_25
			if ((MEM4(0x8c000138) & 0x3fff) == 0x2074)
				sys_data.spi_mode = 1;
		}
	}
	if (!~sys_data.spi) sys_data.spi = 0;

	id = sys_data.lcd_id;
	if (id == 0x1230 || id == 0x1306) {
		DBG_LOG("LCD: id = 0x%06x\n", id);
		lcd = lcd_find_conf(id, 2);
		if (id == 0x1230) {
			lcd_reset_hx1230();
			lcd_setup.send = lcd_send_hx1230;
#if LIBC_SDIO == 0
		} else if (id == 0x1306) {
			lcd_reset_ssd1306();
			lcd_setup.send = lcd_send_ssd1306;
#endif
		}
		return lcd;
	}

	AHB_PWR_ON(0x40);	// LCM enable

	LCM_CR(0) = 0;
	LCM_CR(0x10) = 1;
	LCM_CR(0x14) = 0xa50100;

	if (IS_SC6530 || !is_whtled_on()) lcm_reset(32, 0);

	clk_rate = get_ahb_freq();
	DBG_LOG("LCD: clk_rate = %u\n", clk_rate);

	if (sys_data.spi) {
		lcd = lcd_spi_init(sys_data.spi, clk_rate);
		lcd_setup.send = spi_send;
		return lcd;
	}

	lcm_set_safe_freq(cs);
	lcm_config_addr(cs);
	id = sys_data.lcd_id;
	if (!id) id = lcd_getid();
	// workaround for NT35310
	if (id == 0x8000) id = lcd_cmdret(0xd4, 4);
	// workaround for ILI9225G and SPFD5420A
	else if (!id || id == 0x040404) {
		lcm_send_cmd(0x00);
		id = lcd_cmdret(0x00, 2);
	}
	if (!id) id = lcd_cmdret(0xd3, 4) & 0xffffff; // ILI9341
	if (!id) id = lcd_getid2();
	DBG_LOG("LCD: id = 0x%06x\n", id);
	lcd = lcd_find_conf(id, 0);

	lcm_set_freq(cs, clk_rate, lcd);
	lcd_setup.send = lcm_send;
	return lcd;
}

void sys_start_refresh(void) {
	int mask = 1;	// osd == 3 ? 2 : /* osd0 */ 1
	if (sys_data.mac & 0x100) {
		if (!(sys_data.mac & 1))
			lcd_refresh_mono(sys_data.framebuf);
		return;
	}
	clean_dcache();
	if (sys_data.spi)
		spi_refresh_next(sys_data.spi);
	LCDC_BASE->irq.en |= mask;
	LCDC_BASE->ctrl |= 8;	// start refresh
}

void sys_wait_refresh(void) {
	int mask = 1;	// osd == 3 ? 2 : /* osd0 */ 1

	if (!(LCDC_BASE->irq.en & mask)) return;

	while ((LCDC_BASE->irq.raw & mask) == 0);
	LCDC_BASE->irq.clr |= mask;
}

static void lcd_init(const lcd_config_t *lcd) {
	unsigned w = lcd->width, h = lcd->height, x2, y2, w2, h2;
	unsigned rotate, mac_arg;
	const uint8_t rtab[4] = { 0, 0xa0, 0xc0, 0x60 };

	sys_data.lcd_id = lcd->id;

	rotate = sys_data.rotate & 3;
	mac_arg = lcd->mac_arg;
	if (sys_data.mac)
		mac_arg = (mac_arg & ~0xff) | (sys_data.mac & 0xff);
	mac_arg ^= rtab[rotate];
	if (mac_arg & 0x100) mac_arg &= ~0x20;
	if (mac_arg & 0x20) {
		unsigned t = w; w = h; h = t;
	}
	x2 = 0; w2 = w - 1;
	y2 = 0; h2 = h - 1;
	// fix for ST7735
	if ((lcd->id & 0xffffff) == 0x7c89f0) {
		w -= x2 = w & 3;
		h -= y2 = h & 3;
	}
	sys_data.mac = mac_arg;
	sys_data.display.w1 = w;
	sys_data.display.h1 = h;

	lcm_exec(lcd->cmd_init);
	if (lcd->id == 0x289328 || lcd->id == 0x5420 || lcd->id == 0x9226) {
		/* Why can't people use standardized commands? */
#define X(a, b) LCM_CMD(a >> 8, 0), LCM_CMD(a & 0xff, b)
		static uint8_t cmd_init2_ili9328[] = {
			// ORG, 0, I/D1, I/D0, AM, 0, 0, 0
			X(0x03, 2), 0x10,0, // Entry Mode
			X(0x50, 2), 0,0, // Horizontal Address Start Position
			X(0x51, 2), 0,0, // Horizontal Address End Position
			X(0x52, 2), 0,0, // Vertical Address Start Position
			X(0x53, 2), 0,0, // Vertical Address End Position
			X(0x20, 2), 0,0, // Horizontal GRAM Address Set
			X(0x21, 2), 0,0, // Vertical GRAM Address Set
			X(0x22, 0), // Write Data to GRAM
			LCM_END
		};
		static uint8_t cmd_init2_spfd5420a[] = {
			X(0x003, 2), 0x10,0, // Entry Mode
			X(0x210, 2), 0,0, // Horizontal Address Start Position
			X(0x211, 2), 0,0, // Horizontal Address End Position
			X(0x212, 2), 0,0, // Vertical Address Start Position
			X(0x213, 2), 0,0, // Vertical Address End Position
			X(0x200, 2), 0,0, // Horizontal GRAM Address Set
			X(0x201, 2), 0,0, // Vertical GRAM Address Set
			X(0x202, 0), // Write Data to GRAM
			LCM_END
		};
#undef X
		uint8_t *p = cmd_init2_ili9328, *p0;
		if (lcd->id == 0x5420) p = cmd_init2_spfd5420a;
		if (lcd->id == 0x9226) { // ILI9225G
			p[6 * 1 + 3] = 0x37;
			p[6 * 2 + 3] = 0x36;
			p[6 * 3 + 3] = 0x39;
			p[6 * 4 + 3] = 0x38;
		}
		p0 = p;
		mac_arg &= 0xff;
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
		lcm_exec(p0);
	} else if (sys_data.mac & 0x100) {
		void (*send_fn)(unsigned, unsigned) = lcd_setup.send;
		uint32_t timer_val = 26000000 / 75, id;
		if (sys_data.spi) FATAL();
		send_fn(0, 0xa0 | (mac_arg >> 6 & 1)); // SEG Direction (MX)
		send_fn(0, 0xc0 | (mac_arg >> 4 & 8)); // COM Direction (MY)
		send_fn(0, 0xa6 | (mac_arg >> 3 & 1)); // Inverse Display (INV)
		id = lcd->id;
		if (id == 0x7567 && (sys_data.mac & 0x10)) { // ST7567A
			send_fn(0, 0xff); // Extension Command (ON)
			send_fn(0, 0x72); // Display Setting Mode (ON)
			send_fn(0, 0xfe); // Extension Command (OFF)
			send_fn(0, 0xd0 + 4); // Set Duty (65)
			send_fn(0, 0x90 + 0); // Set Bias (1/9)
			send_fn(0, 0x98 + 6); // Set Frame Rate (300)
			send_fn(0, 0x81); // Set EV (contrast)
			send_fn(0, 0x28); // Set EV (value)
			timer_val = 26000000 / 300;
		} else if (id == 0x1306) {
			timer_val = 26000000 / 150;
		}
		if (sys_data.mac & 1) {
			if (timer_val)
				lcd_setup_timer(LCD_TIMER, timer_val);
			else sys_data.mac &= ~1;
		}
	} else {
		static uint8_t cmd_init2[] = {
			// MY, MX, MV, ML, RB, MH, 0, 0
			LCM_CMD(0x36, 1), 0, // Memory Access Control
			LCM_CMD(0x2a, 4), 0,0,0,0, // Column Address Set
			LCM_CMD(0x2b, 4), 0,0,0,0, // Page Address Set
			LCM_CMD(0x2c, 0), // Memory Write
			LCM_END
		};
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
	lcdc_base_t *lcdc = LCDC_BASE;
	struct sys_display *disp = &sys_data.display;
	unsigned w = disp->w1, h = disp->h1;
	unsigned w2 = disp->w2, h2 = disp->h2;

	if (sys_data.mac & 0x100) {
		lcd_clear_mono();
		return;
	}

	AHB_PWR_ON(0x1000);	// LCDC enable
	// AHB_SOFT_RST?
	AHB_CR(_chip != 1 ? 0x20 : 0x1040) = 0x200;
	sys_wait_ms(10);
	AHB_CR(_chip != 1 ? 0x30 : 0x2040) = 0x200;

	lcdc->ctrl |= 1; // lcdc_enable = 1

	// INT_IRQ_ENABLE
	// MEM4(0x80000000 + 8) |= 0x400000;

	lcdc->ctrl |= 2; // fmark_mode = 1
	lcdc->ctrl &= ~4; // fmark_pol = 0
	lcdc->bg_color = 0x000000;

	lcdc->disp_size = w | h << 16;
	lcdc->lcm_start = 0;
	lcdc->lcm_size = w | h << 16;

	{
		uint32_t addr, mode;
		if (!sys_data.spi) {
			lcm_set_mode(0x28); // 8x2 BE
			mode = 2; addr = lcd_setup.addr | 1 << 17;
		} else {
			spi_refresh_init(sys_data.spi);
			mode = 0; addr = sys_data.spi + SPI_TXD;
		}
		lcdc->cap.ctrl |= 0x20;
		lcdc->cap.ctrl |= (lcdc->cap.ctrl & ~(3 << 6)) | mode << 6;
		lcdc->ctrl &= ~(7 << 5);
		lcdc->cap.base_addr = addr >> 2;
	}

	sys_start_refresh();
	sys_wait_refresh();
	lcdc->irq.en &= ~1;

	{
		uint32_t a = lcdc->img.ctrl;
		int fmt = 5;
		//a |= 1;
		a &= ~2; // disable color key
		//a |= 4; // block alpha
		a = (a & ~(15 << 4)) | fmt << 4; // format
		a = (a & ~(3 << 8)) | 2 << 8; // little endian
		lcdc->img.ctrl = a;
	}

	lcdc->img.pitch = w2;

	w2 = w2 > w ? w : w2;
	h2 = h2 > h ? h : h2;
	w2 &= ~1;	// must be aligned

	//lcdc->img.base_addr = (uint32_t)base >> 2;
	lcdc->img.size_xy = w2 | h2 << 16;
	lcdc->img.disp_xy = ((w - w2) >> 1) | ((h - h2) >> 1) << 16;
}

void sys_framebuffer(void *base) {
	struct sys_display *disp = &sys_data.display;
	unsigned w = disp->w1, w2 = disp->w2;
	unsigned offset = (w2 - w) >> 1;

	if (sys_data.mac & 0x100) {
		uint32_t r, n = w * disp->h1;
		uint8_t *d = (uint8_t*)base;
		sys_data.framebuf = base;
		memset(d, 0x80, n); d += n;
		r = sys_timer_ms();
		do *d++ = (r = (r * 0x08088405) + 1) >> 25;
		while (--n);
		{
			unsigned id = sys_data.lcd_id;
			void (*fn)(uint8_t*);
			do {
				fn = lcd_refresh_st7567;
				if (id == 0x7567 || id == 0x7565) {
					lcd_refresh_st7567_init();
					break;
				}
				fn = lcd_refresh_hx1230;
				if (id == 0x1230) break;
#if LIBC_SDIO == 0
				fn = lcd_refresh_ssd1306;
				if (id == 0x1306) break;
#endif
				/* should not happen */
			} while (0);
			lcd_refresh_mono = fn;
		}
		if (sys_data.mac & 1) {
			lcd_start_timer();
		}
		return;
	}

	if (w2 < w) offset = 0;

	LCDC_BASE->img.y_base_addr = ((uint32_t)base + offset * 2) >> 2;
	LCDC_BASE->img.ctrl |= 1;
}

int keypad_read_pb(void) {
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

static void keypad_init(void) {
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
	ctrl &= _chip == 1 ? ~0xffff00 : ~0xfcfc00;
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

static void init_charger(void) {
	// ANA_CHGR_CTRL0
	unsigned ctrl0 = 0x82001400 + 0x150;
	if (_chip != 1) ctrl0 = 0x82001000 + 0x200;
	// will allow to remove the battery after boot
	if (_chip != 1 && !IS_SC6530 && sys_data.charge >= 2) {
		// CHGR_CTRL0:CHGR_CC_EN = 1
		adi_write(ctrl0, adi_read(ctrl0) | 1);
		// CHGR_CTRL0:CHGR_CC_I = 300mA
		adi_write(ctrl0, (adi_read(ctrl0) & ~(0xf << 4)) | 0 << 4);
		// CHGR_CTRL0:CHGR_PD = 1
		adi_write(ctrl0, adi_read(ctrl0) | 1 << 8);
		sys_wait_ms(10);
		// CHGR_CTRL0:CHGR_PD = 0
		adi_write(ctrl0, adi_read(ctrl0) & ~(1 << 8));
		// CHGR_CTRL0:RECHG = 1
		adi_write(ctrl0, adi_read(ctrl0) | 1 << 12);
	} else if (sys_data.charge >= 0) {
		unsigned charger_pd = sys_data.charge < 1;
		unsigned sh = _chip == 1 ? 0 : 8;	// CHGR_PD
		uint32_t val = adi_read(ctrl0) & ~(1 << sh);
		adi_write(ctrl0, val | charger_pd << sh);
	}
}

static void irq_handler(void) {
	uint32_t timer = LCD_TIMER_ADDR;
	if (MEM4(timer + 0xc) & 4) {
		MEM4(timer + 0xc) = 9;
		lcd_refresh_mono(sys_data.framebuf);
	}
}

extern uint8_t int_vectors[], int_vectors_end[];
void set_mode_sp(int mode, uint32_t sp);
void invalidate_tlb(void);
void invalidate_tlb_mva(uint32_t);

void sys_prep_vectors(uint32_t *ttb) {
	uint8_t *p = (uint8_t*)CHIPRAM_ADDR + 0x19000;
	intptr_t sp = (intptr_t)(p + 0x9000);
	uint32_t cb = 3 << 2; // write-back cachable
	uint32_t domain = 1 << 5;

	memcpy(p, int_vectors, int_vectors_end - int_vectors);
	set_mode_sp(0xd2, sp); // sp_IRQ
	set_mode_sp(0xd3, sp); // sp_SVC
	set_mode_sp(0xd7, sp); // sp_ABT
	set_mode_sp(0xdb, sp); // sp_UND
#if 1 // 4K coarse page
	memset(p + 0x400, 0, 0x400);
#if 0 // vectors at 0
	MEM4(p + 0x400) = (intptr_t)p | cb | 2;
	ttb[0] = ((intptr_t)p + 0x400) | domain | 0x11;
#else // vectors at 0xffff0000
	MEM4(p + 0x7c0) = (intptr_t)p | cb | 2;
	ttb[0xfff] = ((intptr_t)p + 0x400) | domain | 0x11;
#endif
#else // 1M section
	ttb[0] = (intptr_t)p | domain | cb | 0x12;
#endif
	clean_invalidate_dcache();
	invalidate_tlb_mva(0xffff0000);
}

// restrict access to memory that shouldn't be used
static void restrict_mem(void) {
	uint32_t ram_addr = (uint32_t)&int_vectors & 0xfc000000;
	uint32_t ram_size = MEM4(ram_addr);
	uint32_t ttb = ram_addr + ram_size - 0x4000;
	unsigned i;
	for (i = 0; i < 0x40 * 4; i += 4)
		MEM4(ttb + i) = 0;
	clean_invalidate_dcache();
	invalidate_tlb();
}

void _debug_msg(const char *msg);

void def_data_except(uint32_t fsr, uint32_t far, uint32_t pc) {
#if LIBC_SDIO >= 3
	(void)fsr; (void)far; (void)pc;
#else
	char buf[80];
	sprintf(buf, "exception: FSR=0x%03x, FAR=0x%08x, PC=0x%08x",
			fsr & 0x1ff, far, pc);
	_debug_msg(buf);
#endif
	for (;;);
}

#if LIBC_SDIO < 3
static void undef_handler(uint32_t pc) {
	char buf[80];
	sprintf(buf, "undefined instruction: PC=0x%08x", pc);
	_debug_msg(buf);
	for (;;);
}
#endif

void app_data_except(uint32_t fsr, uint32_t far, uint32_t pc);

void sys_set_handlers(void) {
	uint8_t *p = (uint8_t*)CHIPRAM_ADDR + 0x19000;

	MEM4(p + 0x20) = (intptr_t)&irq_handler;
#ifdef APP_DATA_EXCEPT
	MEM4(p + 0x24) = (intptr_t)&app_data_except;
#else
#if LIBC_SDIO < 3
	MEM4(p + 0x24) = (intptr_t)&def_data_except;
#endif
#endif
#if LIBC_SDIO < 3
	MEM4(p + 0x28) = (intptr_t)&undef_handler;
#endif
	clean_invalidate_dcache();
	invalidate_icache();
}

void sys_init(void) {
	// Disable UART to save some power.
	APB_PWR_OFF(0xc00);	// disable UART
	init_charger();
	init_chip_id();
	pin_init();
	lcd_init(lcm_init());
	lcd_appinit();
	lcdc_init();
	if (sys_data.keymap_addr) {
		keypad_init();
		keytrn_init();
	}
	restrict_mem();
	sys_data.init_done = 1;
}

void sys_start(void) {
	sys_brightness(sys_data.brightness);
	if (sys_data.keymap_addr)
		KEYPAD_BASE->int_clr = 0xfff;
}

void sys_exit(void) {
	if (sys_data.init_done) {
		sys_brightness(0);
		LCDC_BASE->ctrl &= ~1;
		AHB_PWR_OFF(0x1000 | 0x40); // LCDC, LCM
		set_cpsr_c(0xdf); // mask interrupts
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

