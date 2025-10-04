
static const uint8_t cmd9106_init[] = {
	//LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	LCM_CMD(0xb3, 1), 0x03,
	LCM_CMD(0xb6, 1), 0x11,
	LCM_CMD(0xa3, 1), 0x11, // Frame Rate
	LCM_CMD(0xac, 1), 0x0b,
	LCM_CMD(0x21, 0), // Display Inversion ON
	//LCM_CMD(0x36, 1), 0xd0, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0xb4, 1), 0x21, // Display Inversion Control
	LCM_CMD(0xaf, 1), 0x80,
	LCM_CMD(0xb0, 1), 0x40,
	LCM_CMD(0xc6, 1), 0x19,
	LCM_CMD(0xf3, 2), 0x01,0x55,
	// Set Gamma 1
	LCM_CMD(0xf0, 14), 0x2e,0x4b,0x24,0x58,
		0x37,0x28,0x2f,0x00, 0x2a,0x0a,0x0a,0x05, 0x05,0x0f,
	// Set Gamma 2
	LCM_CMD(0xf1, 14), 0x0b,0x2c,0x24,0x3e,
		0x28,0x28,0x2f,0x00, 0x08,0x0b,0x0b,0x04, 0x03,0x0f,
	LCM_CMD(0x35, 1), 0x00,
	LCM_CMD(0x44, 1), 0x00,
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

#define LCD_CONFIG(id, w,h, mac, a,b,c,d,e,f, spi, name) \
	{ id, ~0, w,h, mac, { a,b,c,d,e,f }, { spi }, name##_init },
#define X(...) LCD_CONFIG(__VA_ARGS__)
#define NO_TIMINGS 0,0,0,0,0,0

static const lcd_config_t lcd_config_t117[] = {
/* CHAKEYAKE T190 */

	// GlaxyCore GC9106
	X(0x009106, 128,160, 0xd0, 60,120,75,40,50,50, 0, cmd9106)
};
#undef X
#undef NO_TIMINGS
