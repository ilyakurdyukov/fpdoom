
static void fast_gpio_set(unsigned id, unsigned off, int state) {
	uint32_t addr, tmp, shl = id & 0xf;
	addr = 0x8a000000 + ((id >> 4) << 7);
	addr += off;
	tmp = MEM4(addr);
	tmp &= ~(1u << shl);
	tmp |= state << shl;
	MEM4(addr) = tmp;
}

// Is there a variety of settings or just these?
// 3-lines, 9-bit
#define GPIO_SDA 0x28
#define GPIO_CLK 0x29
#define GPIO_CS 0x2a
#define GPIO_RST 0x3e
#if 0 // orig
// 4*40 = 160, 312000000 / 160 = ~2ns
#define GPIO_DELAY_CLK DELAY(40)
#define GPIO_DELAY_END
#else
#define GPIO_DELAY_CLK
#define GPIO_DELAY_END
#endif

static void lcd_gpio_prep(int state) {
	fast_gpio_set(GPIO_SDA, 8, state);
	fast_gpio_set(GPIO_CLK, 8, state);
	fast_gpio_set(GPIO_CS, 8, state);
	fast_gpio_set(GPIO_RST, 8, state);
}

#if 1
#define lcd_gpio_send lcd_gpio_send_asm
void lcd_gpio_send(unsigned idx, unsigned val);
#else
static void lcd_gpio_send(unsigned idx, unsigned val) {
#if !((GPIO_SDA ^ GPIO_CLK) >> 4) && !((GPIO_SDA ^ GPIO_CS) >> 4)
	uint32_t addr, tmp;
	addr = 0x8a000000 + ((GPIO_SDA >> 4) << 7);
	tmp = MEM4(addr);
	val = val << 24 | 1 << 23;
	do {
		tmp &= ~(1u << (GPIO_CLK & 0xf) |
				1u << (GPIO_SDA & 0xf) | 1u << (GPIO_CS & 0xf));
		tmp |= idx << (GPIO_SDA & 0xf);
		MEM4(addr) = tmp;
		DELAY(3)
		tmp |= 1u << (GPIO_CLK & 0xf);
		MEM4(addr) = tmp;
		DELAY(2)
		idx = val >> 31;
	} while ((val <<= 1));
	tmp |= 1u << (GPIO_CS & 0xf);
	MEM4(addr) = tmp;
#else
	//lcd_gpio_prep(1); // don't need
	fast_gpio_set(GPIO_CS, 0, 0);
	val = val << 24 | 1 << 23;
	do {
		fast_gpio_set(GPIO_CLK, 0, 0);
		fast_gpio_set(GPIO_SDA, 0, idx);
		GPIO_DELAY_CLK
		fast_gpio_set(GPIO_CLK, 0, 1);
		GPIO_DELAY_CLK
		idx = val >> 31;
	} while ((val <<= 1));
	fast_gpio_set(GPIO_CS, 0, 1);
	GPIO_DELAY_END
#endif
}
#endif

static void lcd_gpio_reset(void) {
	lcd_gpio_prep(1);
	fast_gpio_set(GPIO_RST, 0, 1);
	sys_wait_ms(10);
	fast_gpio_set(GPIO_RST, 0, 0);
	sys_wait_ms(10);
	fast_gpio_set(GPIO_RST, 0, 1);
}
#undef GPIO_SDA
#undef GPIO_CLK
#undef GPIO_CS
#undef GPIO_RST

/* ST7567, SSD1306, STE2007, HX1230 */
static void lcd_clear_mono(void) {
	struct sys_display *disp = &sys_data.display;
	unsigned x, y, col = 0;
	if (sys_data.lcd_id == 0x7567) col = sys_data.mac >> 4 & 4;
	unsigned w = disp->w1, h = (disp->h1 + 7) >> 3;
	void (*send_fn)(unsigned, unsigned) = lcd_setup.send;
	for (y = 0; y < h; y++) {
		send_fn(0, 0xb0 + y); // Set Page Address
		send_fn(0, 0x10); // Set Column Address (high)
		send_fn(0, col); // Set Column Address (low)
		// lsb = y, msb = y + 7
		for (x = 0; x < w; x++)
#if 0 // for testing
			send_fn(1, x * 2 | ((2 << y) - 1));
			//send_fn(1, (y ^ (x >> 3)) & 1 ? 0xff : 0);
#else
			send_fn(1, 0);
#endif
	}
}

static void lcd_refresh_st7567(void) {
	uint8_t *s = sys_data.framebuf;
	unsigned x, y, col = sys_data.mac >> 4 & 4;
	uint32_t addr0, addr1;
	addr0 = sys_data.lcd_cs << 26 | 0x60000000;
#define SEND(type, a) { while (LCM_CR(0) & 2); \
	*(volatile uint16_t*)addr##type = (a); }
	for (y = 0; y < 8; y++, s += 128 * 7) {
		SEND(0, 0xb0 + y) // Set Page Address
		SEND(0, 0x10) // Set Column Address (high)
		SEND(0, col) // Set Column Address (low)
		addr1 = addr0 | 0x20000;
		for (x = 0; x < 128 / 4; x++, s += 4) {
			uint32_t *s2 = (uint32_t*)s;
			uint32_t a = ~0, b, m = 0x80808080;
#define X(i) \
	b = *s2 + s2[128 * 64 / 4]; \
	s2[128 * 64 / 4] = b & ~m; \
	s2 += 128 / 4; a ^= (b & m) >> i;
			X(7) X(6) X(5) X(4)
			X(3) X(2) X(1) X(0)
#undef X
			SEND(1, a & 255)
			SEND(1, a >> 8 & 255)
			SEND(1, a >> 16 & 255)
			SEND(1, a >> 24 & 255)
		}
	}
#undef SEND
}

static void lcd_refresh_hx1230(void) {
	uint8_t *s = sys_data.framebuf;
	unsigned x, y;
#define SEND(type, a) lcd_gpio_send(type, a);
	s += 96;
	for (y = 0; y < 9; y++, s += 96 * 9) {
		SEND(0, 0xb0 + y) // Set Page Address
		SEND(0, 0x10) // Set Column Address (high)
		SEND(0, 0x00) // Set Column Address (low)
		for (x = 0; x < 96 / 4; x++) {
			uint32_t *s2 = (uint32_t*)(s -= 4);
			uint32_t a = ~0, b, m = 0x80808080;
#define X(i) \
	b = *s2 + s2[96 * 68 / 4]; \
	s2[96 * 68 / 4] = b & ~m; \
	s2 += 96 / 4; a ^= (b & m) >> i;
			X(7) X(6) X(5) X(4)
			if (y < 8) { X(3) X(2) X(1) X(0) }
#undef X
			SEND(1, a >> 24 & 255)
			SEND(1, a >> 16 & 255)
			SEND(1, a >> 8 & 255)
			SEND(1, a & 255)
		}
#undef X
	}
#undef SEND
}

