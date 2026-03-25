
#if UMS9117

#define EFUSE_MAX 64
#define EFUSE_BASE 0x40240000
#define EFUSE_CR(x) MEM4(EFUSE_BASE + (x))

static void efuse_init(void) {
	MEM4(0x402e0000) |= 0x2000;
	// reset
	MEM4(0x402e0008) |= 0x4000;
	DELAY(100)
	MEM4(0x402e0008) &= ~0x4000;
}

static void efuse_off(void) {
	MEM4(0x402e0000) &= ~0x2000;
}

static uint32_t efuse_read(unsigned id, int dbl_bit) {
	uint32_t ret;
	EFUSE_CR(0x48) = 0xffff;
	EFUSE_CR(0x50) |= 2;
	EFUSE_CR(0x40) = 1 | dbl_bit << 2;
	EFUSE_CR(0x54) |= 4;
	ret = EFUSE_CR(0x1000 + id * 4);
	EFUSE_CR(0x54) &= ~4;
	return ret;
}
#else

#define EFUSE_MAX (_chip == 1 ? 16 : 8)
#define EFUSE_BASE 0x82002000
#define EFUSE_CR(x) MEM4(EFUSE_BASE + (x))

static void efuse_init(void) {
	uint32_t addr = 0x8b0010a8;
	uint32_t rst = 0x8b001068, add = 0x1000;
	if (_chip != 1) addr = 0x8b0000a0, rst = 0x8b000060, add = 4;

	// enable
	MEM4(addr) = 1 << 27;
	// reset
	MEM4(rst) = 0x400000;
	DELAY(100)
	MEM4(rst + add) = 0x400000;

	EFUSE_CR(0x10) |= 2 << 28;
	EFUSE_CR(0x10) |= 1 << 28;
	EFUSE_CR(0x10) |= 8 << 28;
	sys_wait_ms(2);
}

static void efuse_off(void) {
	EFUSE_CR(0x10) &= ~(1 << 28);
	EFUSE_CR(0x10) &= ~(8 << 28);
	EFUSE_CR(0x10) &= ~(2 << 28);
	{
		uint32_t addr = 0x8b0020a8;
		if (_chip != 1) addr = 0x8b0000a4;
		MEM4(addr) = 1 << 27;
	}
}

static int efuse_read(unsigned id, uint32_t *ret) {
	uint32_t t0;
	EFUSE_CR(8) = id | id << 16;
	EFUSE_CR(0xc) |= 2;
	t0 = sys_timer_ms();
	while (EFUSE_CR(0x14) & 2)
		if (sys_timer_ms() - t0 > 2) return 1;
	*ret = EFUSE_CR(0);
	return 0;
}
#endif

