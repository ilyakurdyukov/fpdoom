enum {
	SPI_TXD = 0
};

typedef volatile struct {
	uint32_t txd, clkd, ctl0, ctl1; // 0x00
	uint32_t ctl2, ctl3, ctl4, ctl5;
	uint32_t int_en, int_clr, int_raw, int_mask; // 0x20
	uint32_t sts1, sts2, dspwait, sts3;
	uint32_t ctl6, sts4, fifo_rst, ctl7; // 0x40
	uint32_t sts5, ctl8, ctl9, ctl10;
	uint32_t ctl11, ctl12, sts6, sts7; // 0x60
	uint32_t sts8, sts9;
} spi_base_t;

static void spi_set_clkd(spi_base_t *spi, int val) {
	spi->clkd = val;
}

static void spi_set_mode(spi_base_t *spi, int val) {
	spi->ctl7 = (spi->ctl7 & ~(7 << 3)) | val << 3;
}

static void spi_set_chnl_len(spi_base_t *spi, int val) {
	val &= 0x1f;
	spi->ctl0 = (spi->ctl0 & ~(0x1f << 2)) | val << 2;
}

static void spi_set_s3w_rd_strt(spi_base_t *spi, int val) {
	val &= 0x1f;
	spi->ctl2 = (spi->ctl2 & ~0x1f) | val;
}

static void spi_set_rtx_mode(spi_base_t *spi, int mode) {
	// 01 - recieve, 10 - transmit
	spi->ctl1 |= mode << 12;
}

static void spi_set_spi_cd_bit(spi_base_t *spi, int val) {
	// 0 - cmd, 1 - data
	spi->ctl8 = (spi->ctl8 & ~(1 << 15)) | val << 15;
}

// cs = 0..3
static void spi_select_cs(spi_base_t *spi, unsigned cs, unsigned state) {
	uint32_t tmp = spi->ctl0 | 1 << (cs += 8);
	spi->ctl0 = tmp & ~(state << cs);
}

// data_len < (1 << 20)
// dummy_len = 0..63
static void spi_set_tx_len(spi_base_t *spi, int data_len, int dummy_len) {
	uint32_t a, b;
	a = spi->ctl8 & ~0x3ff;
	b = spi->ctl9 & ~0xffff;
	a |= dummy_len << 4 | data_len >> 16;
	spi->ctl8 = a;
	spi->ctl9 = b | (data_len & 0xffff);
}

static void spi_set_rx_len(spi_base_t *spi, int data_len, int dummy_len) {
	uint32_t a, b;
	a = spi->ctl10 & ~0x3ff;
	b = spi->ctl11 & ~0xffff;
	a |= dummy_len << 4 | data_len >> 16;
	spi->ctl10 = a;
	spi->ctl11 = b | (data_len & 0xffff);
}

static void spi_set_ctl12_tx_rx(spi_base_t *spi) {
	spi->ctl12 |= 3;	// SW_TX_REQ(2) | SW_RX_REQ(1)
}

static void spi_wait_tx(spi_base_t *spi) {
	while (!(spi->int_raw & 1 << 8));	// TX_END_IRQ
	spi->int_clr |= 1 << 8;
	while (spi->sts2 & 1 << 8);	// BUSY
	while (!(spi->sts2 & 1 << 7));	// TXF_REAL_EMPTY
}

static uint32_t spi_send_recv(spi_base_t *spi, int data, int data_len, int dummy_len) {
	spi_set_tx_len(spi, data_len, dummy_len);
	spi_set_rx_len(spi, data_len, dummy_len);
	spi_set_ctl12_tx_rx(spi);
	spi->txd = data;
	spi_wait_tx(spi);
	while (spi->sts2 & 1 << 5);	// RXF_REAL_EMPTY
	return spi->txd;
}

static void spi_send(unsigned idx, unsigned data) {
	spi_base_t *spi = (spi_base_t*)sys_data.spi;
	if (!idx) spi_set_spi_cd_bit(spi, 0);
	spi_set_tx_len(spi, 1, 0);
	spi->ctl12 |= 2;
	spi->txd = data;
	spi_wait_tx(spi);
	if (!idx) spi_set_spi_cd_bit(spi, 1);
}

static void spi_refresh_init(uint32_t spi_base) {
	spi_base_t *spi = (spi_base_t*)spi_base;
	spi->ctl7 |= 0x80;	// SPI_TX_HLD_EN
	spi_set_spi_cd_bit(spi, 1);
	if (sys_data.spi_mode < 3) {
		spi->ctl7 |= 1 << 14;	// RGB565_EN
		spi_set_chnl_len(spi, 17);
	} else {
		spi_set_chnl_len(spi, 16);
	}
}

static void spi_refresh_next(uint32_t spi_base) {
	spi_base_t *spi = (spi_base_t*)spi_base;
	struct sys_display *disp = &sys_data.display;
	unsigned len = disp->w1 * disp->h1;
	while (!(spi->sts2 & 1 << 7));	// TXF_REAL_EMPTY
	while (spi->sts2 & 1 << 8);	// BUSY
	spi_set_tx_len(spi, len, 0);
	spi->ctl12 |= 2;
}

static void lcm_exec(const uint8_t *cmd);
static void lcdc_init(void);

#define SPI_ID(spi) ((uint32_t)(spi) >> 24 & 1)

static void spi_pin_grp_select(spi_base_t *spi, unsigned pin_gid) {
	uint32_t tmp = MEM4(0x8b0001e0);
	unsigned shl = SPI_ID(spi) ? 4 : 29;
	if (!pin_gid) FATAL();	// TODO
	tmp = (tmp & ~(3 << shl)) | pin_gid << shl;
	MEM4(0x8b0001e0) = tmp;
	DELAY(100)
}

static const lcd_config_t* lcd_spi_init(uint32_t spi_base) {
	spi_base_t *spi = (spi_base_t*)spi_base;
	const lcd_config_t *lcd;
	uint32_t id, tmp;

	if (!SPI_ID(spi)) {
		AHB_PWR_ON(0x4000); // SPI0 enable
		tmp = _chip == 1 ? 0x800 : 0;
		if (_chip == 1) {
			// freq: 3 - 75.6M, 4 - 104M, 5 - 104M (APLL)
			MEM4(0x8d200054) = (MEM4(0x8d200054) & ~0x307) | 5;
		}
	} else {
		AHB_PWR_ON(0x80000); // SPI1 enable
		tmp = IS_SC6530 ? 0x20 : 0x8000;
		if (_chip == 1) {
			// freq: 3 - 48M, 4 - 75.6M, 5 - 104M, 6 - 104M (APLL)
			MEM4(0x8d200058) = (MEM4(0x8d200058) & ~0x307) | 6;
		}
	}
	DELAY(100)

	// SPI reset
	if (tmp) {
		uint32_t rst = 0x20501040, add = 0x1000;
		if (_chip != 1) rst = 0x8b000060, add = 4;
		MEM4(rst) = tmp;
		DELAY(1000)
		MEM4(rst + add) = tmp;
		DELAY(1000)
	}

	if (_chip == 2)
		spi_pin_grp_select(spi, 1);

	{
		uint32_t mode = 0;
		mode = (mode & 1 ? 1 : 2) | (mode & 2 ? 1 << 13 : 0);
		spi->int_en = 0;
		spi->ctl0 = 0xf00 | mode | 8 << 2;
		spi->ctl1 &= ~0x3000;
		spi->ctl4 = 0x8000;
		spi->ctl5 = 0;
	}

	spi_set_clkd(spi, 15);
	spi_set_mode(spi, sys_data.spi_mode < 3 ? 5 : 6);
	spi_set_chnl_len(spi, 16);
	spi_set_s3w_rd_strt(spi, 7);
	spi_set_rtx_mode(spi, 3);
	spi_set_spi_cd_bit(spi, 0);
	spi_select_cs(spi, sys_data.lcd_cs, 1);

	id = sys_data.lcd_id;
	if (!id) {
		id = (spi_send_recv(spi, 0xda << 8, 1, 0) & 0xff) << 16;
		id |= (spi_send_recv(spi, 0xdb << 8, 1, 0) & 0xff) << 8;
		id |= spi_send_recv(spi, 0xdc << 8, 1, 0) & 0xff;
		DBG_LOG("LCD(SPI%u): id = 0x%06x\n", SPI_ID(spi), id);
	}
	lcd = lcd_find_conf(id, 1);

	{
		unsigned spi_freq = 104000000;
		unsigned freq = lcd->spi.freq, clkd;
		if (_chip != 1)
			spi_freq = !SPI_ID(spi) ? 78000000 : 48000000;
		clkd = spi_freq / (freq << 1);
		if (clkd) clkd--;
		spi_set_clkd(spi, clkd);
	}
	spi_set_chnl_len(spi, 8);
	spi_set_mode(spi, sys_data.spi_mode);
	return lcd;
}

