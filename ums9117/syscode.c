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

uint32_t adi_read(uint32_t addr) {
	int32_t a;
	uint32_t base = 0x40600000;
	MEM4(base + 0x28) = addr;
	while ((a = MEM4(base + 0x2c)) < 0);
	return (uint16_t)a;
}

void adi_write(uint32_t addr, uint32_t val) {
	uint32_t base = 0x40600000;
	uint32_t t0 = sys_timer_ms();
	while (MEM4(base + 0x30) & 0x800)
		if (sys_timer_ms() - t0 > 3)
#if NO_ADI_LOG
			for (;;);
#else
			FATAL();
#endif
	MEM4(addr) = val;
}

#define AHB_CR(x) MEM4(0x20e00000 + (x))
#define AHB_PWR_ON(x) AHB_CR(0x1000) = (x)
#define AHB_PWR_OFF(x) AHB_CR(0x2000) = (x)
#define APB_CR(x) MEM4(0x402e0000 + (x))

static void init_chip_id(void) {
	uint32_t t0;
	t0 = MEM4(0x402e00fc);
	sys_data.chip_id.num = t0;
	DBG_LOG("chip_id: %s = 0x%x\n", "num", t0);
}

#define GPIO_BASE 0x40280000

#define gpio_get(id, off) gpio_set(id, off, -1)
static int gpio_set(unsigned id, unsigned off, int state) {
	uint32_t addr, tmp, shl = id & 0xf;
	if (id >= 0x80) FATAL();
	addr = GPIO_BASE + ((id >> 4) << 7);
	addr += off;
	tmp = MEM4(addr);
	if (state < 0) return tmp >> shl & 1;
	tmp &= ~(1u << shl);
	tmp |= state << shl;
	MEM4(addr) = tmp;
	return 0;
}

extern uint16_t gpio_data[];

static void gpio_init(void *ptr) {
	struct {
		int16_t id, val; uint32_t dir, mode;
	} *tab = ptr;
	int i;

	APB_CR(0x1000) = 8;

	for (i = 0; tab[i].id != -1; i++) {
		int a = tab[i].id, val;
		gpio_set(a, 4, 1);	// GPIO_DMSK
		if (!tab[i].dir) {
			gpio_set(a, 8, 1); // GPIO_DIR
			gpio_set(a, 0x28, 0); // GPIO_INEN
			val = tab[i].val;
			val = val != -1 && (val & 0xff);
			gpio_set(a, 0, val); // GPIO_DATA
		} else {
			gpio_set(a, 8, 0); // GPIO_DIR
			gpio_set(a, 0x28, 1); // GPIO_INEN
		}
	}

	{
		uint16_t *p = gpio_data;
		unsigned a;
		while ((a = *p++) != 0xffff) {
			gpio_set(a & 0x7fff, 0, a >> 15);
		}
	}
}

extern uint32_t *pinmap_addr, *gpiomap_addr;

static void pin_init(void) {
	const volatile uint32_t *pinmap = pinmap_addr;

	APB_CR(0x1000) = 0x100000;
	APB_CR(0x10b0) = 0x1000;
	for (;;) {
		uint32_t addr = pinmap[0], val = pinmap[1];
		pinmap += 2;
		if (addr - 0x40608000 < 0x1000) {
			adi_write(addr, val & 0xffff);
		} else if (addr - 0x402a0000 < 0x1000) {
			MEM4(addr) = val;
		} else break;
	}
	MEM4(0x402a0018) &= 0xe0000000;

	if (sys_data.gpio_init)
		gpio_init((void*)gpiomap_addr);
}

static void backlight_init(void) {
	uint32_t tmp;
	adi_write(0x40608c08, adi_read(0x40608c08) | 0x200);
	adi_write(0x40608c10, adi_read(0x40608c10) | 0x80 | 0x1000);
	adi_write(0x40608dec, adi_read(0x40608dec) & ~5);
	adi_write(0x406081c8, adi_read(0x406081c8) & ~0xff);
	tmp = sys_data.bl_extra[0];
	adi_write(0x40608df4, (adi_read(0x40608df4) & ~0xf) | tmp);
	tmp = sys_data.bl_extra[1];
	adi_write(0x406081b8, (adi_read(0x406081b8) & ~0x3f) | tmp);
	adi_write(0x406081bc, (adi_read(0x406081bc) & ~0x3f) | tmp);
	adi_write(0x406081c0, (adi_read(0x406081c0) & ~0x3f) | tmp);
	tmp = sys_data.bl_extra[2];
	adi_write(0x406081c4, (adi_read(0x406081c4) & ~0x3f) | tmp);
}

void sys_brightness(unsigned val) {
	uint32_t tmp, mask = 0;
	if (val > 100) val = 100;

	tmp = adi_read(0x40608180);
	if (sys_data.bl_extra[1]) mask |= 0x111;
	if (sys_data.bl_extra[2]) mask |= 0x1000;
	if (val) tmp |= mask << 1 | mask; else tmp &= ~mask;
	adi_write(0x40608180, tmp);

	tmp = 100;
	if (sys_data.bl_extra[1]) {
		adi_write(0x40608188, val << 8 | tmp);
		adi_write(0x40608198, val << 8 | tmp);
		adi_write(0x406081a8, val << 8 | tmp);
	}
	if (sys_data.bl_extra[2])
		adi_write(0x406081cc, val << 8 | tmp);
}

#define LCM_REG_BASE 0x20a00000
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
//static inline void lcm_send_data(unsigned val) { lcm_send(1, val); }

static void lcm_reset(unsigned delay, unsigned method) {
	uint32_t addr = 0x402e160c; int i;
	method &= 1;
	// 0x1000 = set, 0x2000 = clear
	for (i = 0; i < 3; i++) {
		MEM4(addr + (method << 12)) = 1; method ^= 1;
		sys_wait_ms(delay);
	}
}

static uint32_t get_ahb_freq(void) {
	return 128000000;
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
	const lcd_config_t *lcd_config = lcd_config_t117;
	int i, n = sizeof(lcd_config_t117) / sizeof(*lcd_config_t117);
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

static int is_whtled_on(void) {
	return adi_read(0x40608000 + 0x180) >> 12 & 1;
}

static const lcd_config_t* lcm_init(void) {
	uint32_t id, clk_rate, cs = sys_data.lcd_cs;
	const lcd_config_t *lcd;

	// auto detect SPI1 display
	if (!sys_data.spi) {
		uint32_t x = 0x402a00b8;
		DBG_LOG("LCD: pins = 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
				MEM4(x), MEM4(x + 4), MEM4(x + 8), MEM4(x + 12), MEM4(x + 16));

		// REG: 0xb8, 0xbc, 0xc0, 0xc4, 0xc8
		// CHAKEYAKE T190: 0x10, 0x0, 0x30, 0x30, 0x10
		// common SPI: (0x30 or 0), 0, 0, 0, 0
		// common LCM: (0x30 or 0x10), 0x10, 0x10, 0x10, 0x10

		// SPI0_CD/LCM_CD/---/GPIO44
		if (MEM4(0x402a00c8) == 0) {
			sys_data.spi = 0x70b << 20;
		}
	}
	if (!~sys_data.spi) sys_data.spi = 0;

	AHB_PWR_ON(0x1000);	// LCM enable

	LCM_CR(0) = 0;
	LCM_CR(0x10) = 1;
	LCM_CR(0x14) = 0xa50100;

	if (!is_whtled_on()) lcm_reset(10, 0);

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
	DBG_LOG("LCD: id = 0x%06x\n", id);
	lcd = lcd_find_conf(id, 0);

	lcm_set_freq(cs, clk_rate, lcd);
	lcd_setup.send = lcm_send;
	return lcd;
}

#define REFRESH_CLEAR_RANGE 1

void sys_start_refresh(void) {
	int mask = 1;

	// Workaround for a buggy LCD controller
	// that doesn't reset the page counter.
	if (!sys_data.spi)
		lcm_send_cmd(0x2c); // Memory Write

#if REFRESH_CLEAR_RANGE
	{
		struct sys_display *disp = &sys_data.display;
		unsigned n = disp->w2 * disp->h2 * 2;
		uint8_t *p = sys_data.framebuf;
		if (p) clean_dcache_range(p, p + n);
	}
#else
	clean_dcache();
#endif
	if (sys_data.spi)
		spi_refresh_next(sys_data.spi);
	LCDC_BASE->irq.en |= mask;
	LCDC_BASE->ctrl |= 8;	// start refresh
}

void sys_wait_refresh(void) {
	int mask = 1;

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
	if (mac_arg & 0x20) {
		unsigned t = w; w = h; h = t;
	}
	x2 = 0; w2 = w - 1;
	y2 = 0; h2 = h - 1;
	sys_data.mac = mac_arg;
	sys_data.display.w1 = w;
	sys_data.display.h1 = h;

	lcm_exec(lcd->cmd_init);
	{
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

	AHB_PWR_ON(0x800);	// LCDC enable
	// AHB_SOFT_RST?
	AHB_CR(0x1004) = 2;
	sys_wait_ms(10);
	AHB_CR(0x2004) = 2;

	lcdc->ctrl |= 1; // lcdc_enable = 1

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
#if REFRESH_CLEAR_RANGE
	sys_data.framebuf = NULL;
#endif
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

	if (w2 < w) offset = 0;

#if REFRESH_CLEAR_RANGE
	sys_data.framebuf = base;
#endif
#if TWO_STAGE
	lcm_config_addr(sys_data.lcd_cs);
#endif

	LCDC_BASE->img.y_base_addr = ((uint32_t)base + offset * 2) >> 2;
	LCDC_BASE->img.ctrl |= 1;
}

int keypad_read_pb(void) {
	uint32_t val, addr = 0x40608280, ch = 1;
	// EIC_DBNC_DMSK
	adi_write(addr + 4, adi_read(addr + 4) | 1 << ch);
	// EIC_DBNC_DATA
	val = adi_read(addr) >> ch & 1;
	return val ^ 1;
}

static void eic_enable(void) {
	adi_write(0x406080c8, adi_read(0x406080c8) | 0x10);
	adi_write(0x40608298, 0);
	adi_write(0x40608c08, adi_read(0x40608c08) | 8);
	adi_write(0x40608e40, adi_read(0x40608e40) | 1);
	adi_write(0x40608c10, adi_read(0x40608c10) | 8);
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

	row &= 0xfc; col &= 0xfc;

	//DBG_LOG("keypad: row_mask = 0x%02x, col_mask = 0x%02x\n", row, col);

	APB_CR(0x1000) = 0x100; // APB_PWR_ON
	APB_CR(0x1010) = 2;

	// reset
	APB_CR(0x1008) = 0x100;
	DELAY(1000)
	APB_CR(0x2008) = 0x100;
	DELAY(1000)

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
	ctrl &= ~0xfcfc00;
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

void sys_set_handlers(void) {
	// TODO
}

void sys_init(void) {
	init_chip_id();
	pin_init();

	lcd_init(lcm_init());
	lcd_appinit();
	lcdc_init();
	backlight_init();

	if (sys_data.keymap_addr) {
		keypad_init();
		keytrn_init();
	}
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
		AHB_PWR_OFF(0x800 | 0x1000); // LCDC, LCM
		set_cpsr_c(0xdf); // mask interrupts
	}
	sys_wdg_reset(SYS_RESET_DELAY); // 0.5 sec
}

void sys_wdg_reset(unsigned val) {
	uint32_t wdg;
	// watchdog enable
	adi_write(0x40608c08, adi_read(0x40608c08) | 4);
	adi_write(0x40608c10, adi_read(0x40608c10) | 4);
	wdg = 0x40608040;
	// set reset timer
	adi_write(wdg + 0x20, 0xe551); // LOCK
	DELAY(10000)
	adi_write(wdg + 8, adi_read(wdg + 8) | 9); // CTRL
	adi_write(wdg + 8, adi_read(wdg + 8) | 4);
	// 1 / 32768
	adi_write(wdg + 0, val & 0xffff); // LOAD_LOW
	adi_write(wdg + 4, val >> 16); // LOAD_HIGH
	adi_write(wdg + 8, adi_read(wdg + 8) | 2);
	adi_write(wdg + 0x20, ~0xe551);
	for (;;);
}

