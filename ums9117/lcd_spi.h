enum {
	SPI_TXD = 0x00,
	SPI_CLKD = 0x04,
	SPI_CTL0 = 0x08,
	SPI_CTL1 = 0x0c,
	SPI_CTL2 = 0x10,
	SPI_CTL4 = 0x18,
	SPI_CTL5 = 0x1c,
	SPI_INT_EN = 0x20,
	SPI_INT_CLR = 0x24,
	SPI_INT_RAW_STS = 0x28,
	SPI_STS2 = 0x34,
	SPI_CTL7 = 0x4c,
	SPI_CTL8 = 0x54, // tx
	SPI_CTL9 = 0x58, // tx
	SPI_CTL10 = 0x5c, // rx
	SPI_CTL11 = 0x60, // rx
	SPI_CTL12 = 0x64,
};

static void spi_set_clkd(uint32_t spi, int val) {
	MEM4(spi + SPI_CLKD) = val;
}

static void spi_set_mode(uint32_t spi, int val) {
	MEM4(spi + SPI_CTL7) = (MEM4(spi + SPI_CTL7) & ~(7 << 3)) | val << 3;
}

static void spi_set_chnl_len(uint32_t spi, int val) {
	val &= 0x1f;
	MEM4(spi + SPI_CTL0) = (MEM4(spi + SPI_CTL0) & ~(0x1f << 2)) | val << 2;
}

static void spi_set_s3w_rd_strt(uint32_t spi, int val) {
	val &= 0x1f;
	MEM4(spi + SPI_CTL2) = (MEM4(spi + SPI_CTL2) & ~0x1f) | val;
}

static void spi_set_rtx_mode(uint32_t spi, int mode) {
	// 01 - recieve, 10 - transmit
	MEM4(spi + SPI_CTL1) |= mode << 12;
}

static void spi_set_spi_cd_bit(uint32_t spi, int val) {
	// 0 - cmd, 1 - data
	MEM4(spi + SPI_CTL8) = (MEM4(spi + SPI_CTL8) & ~(1 << 15)) | val << 15;
}

// cd = 0..3
static void spi_select_cs(uint32_t spi, unsigned cs, unsigned state) {
	uint32_t tmp = MEM4(spi + SPI_CTL0) | 1 << (cs += 8);
	MEM4(spi + SPI_CTL0) = tmp & ~(state << cs);
}

// data_len < (1 << 20)
// dummy_len = 0..63
static void spi_set_tx_len(uint32_t spi, int data_len, int dummy_len) {
	uint32_t a, b;
	a = MEM4(spi + SPI_CTL8) & ~0x3ff;
	b = MEM4(spi + SPI_CTL9) & ~0xffff;
	a |= dummy_len << 4 | data_len >> 16;
	MEM4(spi + SPI_CTL8) = a;
	MEM4(spi + SPI_CTL9) = b | (data_len & 0xffff);
}

static void spi_set_rx_len(uint32_t spi, int data_len, int dummy_len) {
	uint32_t a, b;
	a = MEM4(spi + SPI_CTL10) & ~0x3ff;
	b = MEM4(spi + SPI_CTL11) & ~0xffff;
	a |= dummy_len << 4 | data_len >> 16;
	MEM4(spi + SPI_CTL10) = a;
	MEM4(spi + SPI_CTL11) = b | (data_len & 0xffff);
}

static void spi_set_ctl12_tx_rx(uint32_t spi) {
	MEM4(spi + SPI_CTL12) |= 3;	// SW_TX_REQ(2) | SW_RX_REQ(1)
}

static void spi_wait_tx(uint32_t spi) {
	while (!(MEM4(spi + SPI_INT_RAW_STS) & 1 << 8));	// TX_END_IRQ
	MEM4(spi + SPI_INT_CLR) |= 1 << 8;
	while (MEM4(spi + SPI_STS2) & 1 << 8);	// BUSY
	while (!(MEM4(spi + SPI_STS2) & 1 << 7));	// TXF_REAL_EMPTY
}

static uint32_t spi_send_recv(uint32_t spi, int data, int data_len, int dummy_len) {
	spi_set_tx_len(spi, data_len, dummy_len);
	spi_set_rx_len(spi, data_len, dummy_len);
	spi_set_ctl12_tx_rx(spi);
	MEM4(spi + SPI_TXD) = data;
	spi_wait_tx(spi);
	while (MEM4(spi + SPI_STS2) & 1 << 5);	// RXF_REAL_EMPTY
	return MEM4(spi + SPI_TXD);
}

static void spi_send(unsigned idx, unsigned data) {
	uint32_t spi = sys_data.spi;
	if (!idx) spi_set_spi_cd_bit(spi, 0);
	spi_set_tx_len(spi, 1, 0);
	MEM4(spi + SPI_CTL12) |= 2;
	MEM4(spi + SPI_TXD) = data;
	spi_wait_tx(spi);
	if (!idx) spi_set_spi_cd_bit(spi, 1);
}

static void spi_refresh_init(uint32_t spi) {
	MEM4(spi + SPI_CTL7) |= 0x80;	// SPI_TX_HLD_EN
	spi_set_spi_cd_bit(spi, 1);
	if (sys_data.spi_mode < 3) {
		MEM4(spi + SPI_CTL7) |= 1 << 14;	// RGB565_EN
		spi_set_chnl_len(spi, 17);
	} else {
		spi_set_chnl_len(spi, 16);
	}
}

static void spi_refresh_next(uint32_t spi) {
	struct sys_display *disp = &sys_data.display;
	unsigned len = disp->w1 * disp->h1;
	while (!(MEM4(spi + SPI_STS2) & 1 << 7));	// TXF_REAL_EMPTY
	while (MEM4(spi + SPI_STS2) & 1 << 8);	// BUSY
	spi_set_tx_len(spi, len, 0);
	MEM4(spi + SPI_CTL12) |= 2;
}

static void lcm_exec(const uint8_t *cmd);
static void lcdc_init(void);

#define SPI_ID(spi) ((spi) >> 20 & 1)

static const lcd_config_t* lcd_spi_init(uint32_t spi, uint32_t clk_rate) {
	const lcd_config_t *lcd;
	uint32_t id, cs = 0;

	(void)clk_rate;

	if (!SPI_ID(spi)) {
		MEM4(0x71301000) = 0x20; // SPI0 enable
		// freq: 0 - 26M, 1 - 128M, 2 - 153.6M, 3 - 192M
		MEM4(0x21500048) = 3;
	} else {
		APB_CR(0x1134) = 0x200; // SPI1 enable
		MEM4(0x21500054) = 3;
	}

	{
		uint32_t mode = 0;
		mode = (mode & 1 ? 1 : 2) | (mode & 2 ? 1 << 13 : 0);
		MEM4(spi + SPI_INT_EN) = 0;
		MEM4(spi + SPI_CTL0) = 0xf00 | mode | 8 << 2;
		MEM4(spi + SPI_CTL1) = MEM4(spi + SPI_CTL1) & ~0x3000;
		MEM4(spi + SPI_CTL4) = 0x8000;
		MEM4(spi + SPI_CTL5) = 0;
	}

	spi_set_clkd(spi, 15);
	spi_set_mode(spi, sys_data.spi_mode < 3 ? 5 : 6);
	spi_set_chnl_len(spi, 16);
	spi_set_s3w_rd_strt(spi, 7);
	spi_set_rtx_mode(spi, 3);
	spi_set_spi_cd_bit(spi, 0);
	spi_select_cs(spi, cs, 1);

	id = sys_data.lcd_id;
	if (!id) {
		id = (spi_send_recv(spi, 0xda << 8, 1, 0) & 0xff) << 16;
		id |= (spi_send_recv(spi, 0xdb << 8, 1, 0) & 0xff) << 8;
		id |= spi_send_recv(spi, 0xdc << 8, 1, 0) & 0xff;
		DBG_LOG("LCD(SPI%u): id = 0x%06x\n", SPI_ID(spi), id);
	}
	lcd = lcd_find_conf(id, 1);

	{
		unsigned spi_freq = 192000000;
		unsigned freq = lcd->spi.freq, clkd;
		clkd = spi_freq / (freq << 1);
		if (clkd) clkd--;
		spi_set_clkd(spi, clkd);
	}
	spi_set_chnl_len(spi, 8);
	spi_set_mode(spi, sys_data.spi_mode);
	return lcd;
}

