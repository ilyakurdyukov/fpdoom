
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
	uint32_t cs = sys_data.lcd_cs;
	uint32_t addr0, addr1;
	addr0 = cs << 26 | 0x60000000;
#if 0
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
#else
	col = 0xe30010b0 | col << 16; // 0xe3 = NOP
	while (LCM_CR(0) & 2);
	LCM_CR(0x10 + (cs << 4)) = 0x18; // 8x4 LE
	for (y = 0; y < 8; y++, s += 128 * 7) {
		while (LCM_CR(0) & 2);
		MEM4(addr0) = col + y;
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
			//while (LCM_CR(0) & 2);
			MEM4(addr1) = a;
		}
	}
#endif
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
			SEND(1, a >> 24)
			SEND(1, a >> 16)
			SEND(1, a >> 8)
			SEND(1, a)
		}
#undef X
	}
#undef SEND
}

#define LCD_TIMER 2
#define LCD_TIMER_ADDR (0x81000000 | (LCD_TIMER / 3) << 12 | (LCD_TIMER % 3) << 5)

static void lcd_setup_timer(int i, uint32_t load) {
	uint32_t a, j = i, timer = 0x81000000;
	// 0,1,3,4 freq: 32768
	// 2,5 freq: 26M

	{
		uint32_t t = 0x8b0010a8, add = 0x1000;
		if (_chip != 1) t = 0x8b0000a0, add = 4;
		// enable
		MEM4(t) = i < 3 ? 0x410000 : 0x40000002;
		a = i < 3 ? 0x2000 : 0x1000000;
		t -= 0x40;
		// reset
		MEM4(t) = a; t += add;
		DELAY(1000)
		MEM4(t) = a;
		DELAY(1000)
	}

	if (j >= 3) timer += 0x1000, j -= 3;
	timer += j << 5;
	j += 4;
	if (_chip == 3 && j == 6) j = 23;
	// INT_IRQ_ENABLE
	MEM4(0x80000000 + 8) = 1 << j;

	MEM4(timer + 8) = 0; // ctl
	MEM4(timer + 0) = load; // load
	MEM4(timer + 0xc) = 1; // int enable
}

static void irq_handler(void) {
	uint32_t timer = LCD_TIMER_ADDR;
	if (MEM4(timer + 0xc) & 4) {
		unsigned id;
		MEM4(timer + 0xc) = 9;
		id = sys_data.lcd_id;
		if (id == 0x7567 || id == 0x7565)
			lcd_refresh_st7567();
		else if (id == 0x1230)
			lcd_refresh_hx1230();
	}
}

extern uint8_t int_vectors[], int_vectors_end[];
void set_mode_sp(int mode, uint32_t sp);
void invalidate_tlb_mva(uint32_t);

static void lcd_setup_irq(void) {
	uint8_t *p = (uint8_t*)CHIPRAM_ADDR;

	memcpy(p, int_vectors, int_vectors_end - int_vectors);
	set_mode_sp(0xd2, (intptr_t)(p + 0x200));
	MEM4(p + 0x20) = (intptr_t)&irq_handler;
	{
		uint32_t ram_addr = (uint32_t)&int_vectors & 0xfc000000;
		uint32_t ram_size = *(volatile uint32_t*)ram_addr;
		MEM4(ram_addr + ram_size - 0x4000) =
				CHIPRAM_ADDR | 1 << 5 | 0x12; // domain = 1
		clean_invalidate_dcache();
		clean_icache();
		invalidate_tlb_mva(0);
	}
}

static void lcd_start_timer(void) {
	uint32_t timer = LCD_TIMER_ADDR;
	MEM4(timer + 8) = 0xc0;
	set_cpsr_c(0x5f);
}
