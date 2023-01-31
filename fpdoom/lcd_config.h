#if CHIP == 1
static const uint8_t cmd8585_init[] = {
	LCM_DELAY(120),
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	//LCM_CMD(0x36, 1), 0x00, // Memory Access Control
	LCM_CMD(0xb2, 5), 0x0c,0x0c,0x00,0x33,0x33, // Porch Setting
	LCM_CMD(0xb7, 1), 0x35, // Gate Control
	LCM_CMD(0xbb, 1), 0x2b, // VCOM Setting
	LCM_CMD(0xc0, 1), 0x2c, // LCM Control
	LCM_CMD(0xc2, 1), 0x01,
	LCM_CMD(0xc3, 1), 0x17,
	LCM_CMD(0xc4, 1), 0x20,
	LCM_CMD(0xc6, 1), 0x0b,
	LCM_CMD(0xca, 1), 0x0f,
	LCM_CMD(0xc8, 1), 0x08,
	// Write Content Adaptive Brightness Control and Color Enhancement
	LCM_CMD(0x55, 1), 0x90,
	LCM_CMD(0xd0, 2), 0xa4,0xa1, // Power Control 1
	LCM_CMD(0x3a, 1), 0x55, // Interface Pixel Format
	// Positive Voltage Gamma Control
	LCM_CMD(0xe0, 14), 0xf0,0x00,0x0a,0x10,
		0x12,0x1b,0x39,0x44, 0x47,0x28,0x12,0x10, 0x16,0x1b,
	// Negative Voltage Gamma Control
	LCM_CMD(0xe1, 14), 0xf0,0x00,0x0a,0x10,
		0x11,0x1a,0x3b,0x34, 0x4e,0x3a,0x17,0x16, 0x21,0x22,
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

#if 0
static const uint8_t cmd8585_off[] = {
	LCM_CMD(0x28, 0), // Display OFF
	LCM_DELAY(50),
	LCM_CMD(0x10, 0), // Enter Sleep Mode
	LCM_DELAY(50),
	LCM_END
};
#endif

static const uint8_t cmd9305_init[] = {
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	//LCM_CMD(0x36, 1), 0x48, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0x35, 1), 0x00,
	LCM_CMD(0x44, 2), 0x00,0x30,
	LCM_CMD(0xa4, 2), 0x44,0x44, // Vcore voltage Control
	LCM_CMD(0xa5, 2), 0x42,0x42,
	LCM_CMD(0xaa, 2), 0x88,0x88,
	LCM_CMD(0xe8, 2), 0x12,0x0b, // Frame Rate
	LCM_CMD(0xe3, 2), 0x01,0x10,
	LCM_CMD(0xff, 1), 0x61,
	LCM_CMD(0xac, 1), 0x00,
	LCM_CMD(0xae, 1), 0x2b,
	LCM_CMD(0xad, 1), 0x33,
	LCM_CMD(0xaf, 1), 0x55,
	LCM_CMD(0xa6, 2), 0x2a,0x2a, // Vreg1a voltage Control
	LCM_CMD(0xa7, 2), 0x2b,0x2b, // Vreg1b voltage Control
	LCM_CMD(0xa8, 2), 0x18,0x18, // Vreg2a voltage Control
	LCM_CMD(0xa9, 2), 0x2a,0x2a, // Vreg2b voltage Control
	LCM_CMD(0xf0, 6), 0x02,0x02,0x00,0x08,0x0c,0x10, // Set Gamma 1
	LCM_CMD(0xf1, 6), 0x01,0x00,0x00,0x14,0x1d,0x0e, // Set Gamma 2
	LCM_CMD(0xf2, 6), 0x10,0x09,0x37,0x04,0x04,0x48, // Set Gamma 3
	LCM_CMD(0xf3, 6), 0x10,0x0b,0x3f,0x05,0x05,0x4e, // Set Gamma 4
	LCM_CMD(0xf4, 6), 0x0d,0x19,0x19,0x1d,0x1e,0x0f, // Set Gamma 5
	LCM_CMD(0xf5, 6), 0x06,0x12,0x13,0x1a,0x1b,0x0f, // Set Gamma 6
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

static const uint8_t cmd9306_init[] = {
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	//LCM_CMD(0x36, 1), 0x48, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0xad, 1), 0x33, // ???
	LCM_CMD(0xaf, 1), 0x55, // ???
	LCM_CMD(0xae, 1), 0x2b, // ???
	LCM_CMD(0xa4, 2), 0x44,0x44, // Vcore voltage Control
	LCM_CMD(0xa5, 2), 0x42,0x42, // ???
	LCM_CMD(0xaa, 2), 0x88,0x88, // ???
	LCM_CMD(0xae, 1), 0x2b, // ???
	LCM_CMD(0xe8, 2), 0x11,0x0b, // Frame Rate
	LCM_CMD(0xe3, 2), 0x01,0x10, // ???
	LCM_CMD(0xff, 1), 0x61, // ???
	LCM_CMD(0xac, 1), 0x00, // ???
	LCM_CMD(0xaf, 1), 0x67, // ???
	LCM_CMD(0xa6, 2), 0x2a,0x2a, // Vreg1a voltage Control
	LCM_CMD(0xa7, 2), 0x2b,0x2b, // Vreg1b voltage Control
	LCM_CMD(0xa8, 2), 0x18,0x18, // Vreg2a voltage Control
	LCM_CMD(0xa9, 2), 0x2a,0x2a, // Vreg2b voltage Control
	LCM_CMD(0xf0, 6), 0x02,0x00,0x00,0x1b,0x1f,0x0b, // Set Gamma 1
	LCM_CMD(0xf1, 6), 0x01,0x03,0x00,0x28,0x2b,0x0e, // Set Gamma 2
	LCM_CMD(0xf2, 6), 0x0b,0x08,0x3b,0x04,0x03,0x4c, // Set Gamma 3
	LCM_CMD(0xf3, 6), 0x0e,0x07,0x46,0x04,0x05,0x51, // Set Gamma 4
	LCM_CMD(0xf4, 6), 0x08,0x15,0x15,0x1f,0x22,0x0f, // Set Gamma 5
	LCM_CMD(0xf5, 6), 0x0b,0x13,0x11,0x1f,0x21,0x0f, // Set Gamma 6
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

static const uint8_t cmd9300_init[] = {
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	//LCM_CMD(0x36, 1), 0x48, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0xa4, 2), 0x44,0x44, // Vcore voltage Control
	LCM_CMD(0xa5, 2), 0x42,0x42,
	LCM_CMD(0xaa, 2), 0x88,0x88,
	LCM_CMD(0xe8, 2), 0x12,0x0b, // Frame Rate
	LCM_CMD(0xe3, 2), 0x01,0x10,
	LCM_CMD(0xff, 1), 0x61,
	LCM_CMD(0xac, 1), 0x00,
	LCM_CMD(0xad, 1), 0x33,
	LCM_CMD(0xae, 1), 0x2b,
	LCM_CMD(0xaf, 1), 0x55,
	LCM_CMD(0xa6, 2), 0x2a,0x2a, // Vreg1a voltage Control
	LCM_CMD(0xa7, 2), 0x2b,0x2b, // Vreg1b voltage Control
	LCM_CMD(0xa8, 2), 0x10,0x10, // Vreg2a voltage Control
	LCM_CMD(0xa9, 2), 0x2a,0x2a, // Vreg2b voltage Control
	LCM_CMD(0x35, 1), 0x00,
	LCM_CMD(0x44, 2), 0x00,0x0a,
	LCM_CMD(0xf0, 6), 0x02,0x00,0x00,0x0a,0x0e,0x10, // Set Gamma 1
	LCM_CMD(0xf1, 6), 0x01,0x02,0x00,0x28,0x35,0x0f, // Set Gamma 2
	LCM_CMD(0xf2, 6), 0x10,0x09,0x35,0x03,0x03,0x43, // Set Gamma 3
	LCM_CMD(0xf3, 6), 0x11,0x0b,0x54,0x05,0x03,0x60, // Set Gamma 4
	LCM_CMD(0xf4, 6), 0x0c,0x17,0x16,0x11,0x12,0x0f, // Set Gamma 5
	LCM_CMD(0xf5, 6), 0x08,0x17,0x17,0x2a,0x2a,0x0f, // Set Gamma 6
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

#if 0
static const uint8_t cmd93xx_off[] = {
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	LCM_CMD(0x28, 0), // Display OFF
	LCM_DELAY(120),
	LCM_CMD(0x10, 0), // Enter Sleep Mode
	LCM_DELAY(120),
	LCM_END
};
#endif

static const uint8_t cmd9102_init[] = {
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	//LCM_CMD(0x36, 1), 0xd0, // Memory Access Control
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	LCM_CMD(0xa8, 1), 0x02, // ???
	LCM_CMD(0xa7, 1), 0x02, // ???
	LCM_CMD(0xea, 1), 0x3a, // ???
	LCM_CMD(0xb4, 1), 0x00, // Display Inversion Control
	LCM_CMD(0xff, 1), 0x0b, // Power Control 4
	LCM_CMD(0xfd, 1), 0x04, // Power Control 3
	LCM_CMD(0xe3, 1), 0x07, // ???
	LCM_CMD(0xa4, 1), 0x08,	// Power Control 1
	LCM_CMD(0xa3, 3), 0x07,0x16,0x16, // Frame Rate
	LCM_CMD(0xe7, 2), 0x94,0x88, // ???
	LCM_CMD(0xed, 1), 0x11, // Power Control 2
	LCM_CMD(0xe4, 1), 0xc5, // ???
	LCM_CMD(0xe2, 1), 0x80, // ???
	LCM_CMD(0xe5, 1), 0x10, // ???

	LCM_CMD(0xf0, 1), 0x55,	// Set Gamma 1
	LCM_CMD(0xf1, 1), 0x37,	// Set Gamma 2
	LCM_CMD(0xf2, 1), 0x01,	// Set Gamma 3
	LCM_CMD(0xf3, 1), 0x52,	// Set Gamma 4
	LCM_CMD(0xf4, 1), 0x00,	// Set Gamma 5
	LCM_CMD(0xf5, 1), 0x00,	// Set Gamma 6
	// 0xf6 skipped
	LCM_CMD(0xf7, 1), 0x67,	// Set Gamma 7
	LCM_CMD(0xf8, 1), 0x22, // Set Gamma 8
	LCM_CMD(0xf9, 1), 0x54,	// Set Gamma 9
	LCM_CMD(0xfa, 1), 0x05,	// Set Gamma 10
	LCM_CMD(0xfb, 1), 0x00,	// Set Gamma 11
	LCM_CMD(0xfc, 1), 0x00,	// Set Gamma 12

	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(200),
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

static const uint8_t cmd9106_init[] = {
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	LCM_CMD(0xb3, 1), 0x03, // ???
	LCM_CMD(0xb6, 1), 0x01, // ???
	LCM_CMD(0xa3, 1), 0x11, // Frame Rate
	LCM_CMD(0x21, 0), // Display Inversion ON
	//LCM_CMD(0x36, 1), 0x00, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0xb4, 1), 0x21, // Display Inversion Control
	// Set Gamma 1
	LCM_CMD(0xf0, 14), 0x31,0x4c,0x24,0x58,
		0xa8,0x26,0x28,0x00, 0x2c,0x0c,0x0c,0x15, 0x15,0x0f,
	// Set Gamma 2
	LCM_CMD(0xf1, 14), 0x0e,0x2d,0x24,0x3e,
		0x99,0x12,0x13,0x00, 0x0a,0x0d,0x0d,0x14, 0x13,0x0f,
	//LCM_CMD(0xfe, 0), // Inter Register Enable 1
	//LCM_CMD(0xff, 0), // ???
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x29, 0), // Display ON
	//LCM_DELAY(120),
	LCM_END
};

static const uint8_t cmd9316_init[] = {
	//LCM_DELAY(120),
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	//LCM_CMD(0x36, 1), 0xd8, // Memory Access Control
	LCM_CMD(0x35, 1), 0x00, // Tearing Effect Line ON
	LCM_CMD(0x26, 1), 0x01, // Gamma Set
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

static const uint8_t cmd1529_init[] = {
	//LCM_DELAY(100),
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(200),
	// Manufacturer Command Access Protect
	LCM_CMD(0xb0, 1), 0x04,
	LCM_CMD(0xb3, 5), 0x02,0x00,0x00,0x00,0x00,
	// Display mode and Frame memory write mode
	LCM_CMD(0xb4, 1), 0x00,
	// Panel Driving Setting
	LCM_CMD(0xc0, 8), 0x03,0xdf,0x40,0x10,0x00,0x01,0x00,0x55,
	// Display Timing Setting for Normal Mode
	LCM_CMD(0xc1, 5), 0x07,0x27,0x08,0x08,0x00,
	// Source/Gate Driving Timing Setting
	LCM_CMD(0xc4, 4), 0x57,0x00,0x05,0x03,
	// DPI polarity Control
	LCM_CMD(0xc6, 1), 0x04,
#define GAMMA 0x03,0x12,0x1a,0x24, 0x32,0x4b,0x3b,0x29, 0x1f,0x18,0x12,0x04
	// Gamma Setting A set
	LCM_CMD(0xc8, 24), GAMMA, GAMMA,
	// Gamma Setting B set
	LCM_CMD(0xc9, 24), GAMMA, GAMMA,
	// Gamma Setting C set
	LCM_CMD(0xca, 24), GAMMA, GAMMA,
#undef GAMMA
	LCM_CMD(0xd0, 16),	// Power Setting
		0x99,0x06,0x08,0x20, 0x29,0x04,0x01,0x00,
		0x08,0x01,0x00,0x06, 0x01,0x00,0x00,0x20,
	// VCOM Setting
	LCM_CMD(0xd1, 4), 0x00,0x28,0x28,0x15,
	// NVM Access Control
	//LCM_CMD(0xe0, 3), 0x00,0x00,0x00,
	//LCM_CMD(0xe1, 6), 0x00,0x00,0x00,0x00,0x00,0x00,
	// NVM Load Control
	//LCM_CMD(0xe2, 1), 0x00,
	//LCM_CMD(0x36, 1), 0x00, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x55, // Pixel Format Set
	//LCM_CMD(0x2a, 4), 0x00,0x00,0x01,0x3f,
	//LCM_CMD(0x2b, 4), 0x00,0x00,0x01,0xdf,
	LCM_CMD(0x29, 0), // Display ON
	LCM_DELAY(120),
	//LCM_CMD(0x2c, 0),
	LCM_END
};
#endif

#if CHIP == 2
static const uint8_t cmd9106_init[] = {
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	LCM_CMD(0xb3, 1), 0x03, // ???
	LCM_CMD(0xb6, 1), 0x01, // ???
	LCM_CMD(0xa3, 1), 0x11, // Frame Rate
	LCM_CMD(0x21, 0), // Display Inversion ON
	//LCM_CMD(0x36, 1), 0xd0, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0xb4, 1), 0x21, // Display Inversion Control
	// Set Gamma 1
	LCM_CMD(0xf0, 14), 0x0c,0x46,0x25,0x56,
		0xac,0x24,0x25,0x00, 0x00,0x12,0x15,0x16, 0x17,0x0f,
	// Set Gamma 2
	LCM_CMD(0xf1, 14), 0x00,0x26,0x25,0x3a,
		0xb9,0x0f,0x10,0x00, 0x00,0x07,0x07,0x17, 0x16,0x0f,
	//LCM_CMD(0xfe, 0), // Inter Register Enable 1
	//LCM_CMD(0xff, 0), // ???
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x29, 0), // Display ON
	//LCM_DELAY(120),
	LCM_END
};

static const uint8_t cmd9108_init[] = {
	LCM_DELAY(120),
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	LCM_CMD(0xb3, 1), 0x03,
	LCM_CMD(0xb6, 1), 0x01,
	LCM_CMD(0xa3, 1), 0x11,
	LCM_CMD(0x21, 0),
	//LCM_CMD(0x36, 1), 0xd0, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0xb4, 1), 0x21,
	// Set Gamma 1
	LCM_CMD(0xf0, 14), 0x25,0x58,0x24,0x68,
		0xad,0x36,0x38,0x00, 0x0b,0x15,0x15,0x17, 0x15,0x0f,
	// Set Gamma 2
	LCM_CMD(0xf1, 14), 0x00,0x1e,0x25,0x30,
		0x97,0x03,0x03,0x00, 0x00,0x07,0x07,0x15, 0x14,0x0f,
	LCM_CMD(0x35, 1), 0x00,
	LCM_CMD(0x44, 1), 0x00,
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

static const uint8_t cmd9307_init[] = {
	LCM_CMD(0xfe, 0), // Inter Register Enable 1
	LCM_CMD(0xef, 0), // Inter Register Enable 2
	//LCM_CMD(0x36, 1), 0x48, // Memory Access Control
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0x86, 1), 0x98,
	LCM_CMD(0x89, 1), 0x03,
	LCM_CMD(0x8b, 1), 0x80,
	LCM_CMD(0x8d, 1), 0x22,
	LCM_CMD(0xe8, 2), 0x13,0x00,
	LCM_CMD(0xc3, 1), 0x27,
	LCM_CMD(0xc4, 1), 0x18,
	LCM_CMD(0xc9, 1), 0x1f,
	LCM_CMD(0xc5, 1), 0x0f,
	LCM_CMD(0xc6, 1), 0x10,
	LCM_CMD(0xc7, 1), 0x10,
	LCM_CMD(0xc8, 1), 0x10,
	LCM_CMD(0xff, 1), 0x62,
	LCM_CMD(0x99, 1), 0x3e,
	LCM_CMD(0x9d, 1), 0x4b,
	LCM_CMD(0x8e, 1), 0x0f,
	LCM_CMD(0xf0, 6), 0x8f,0x16,0x06,0x06,0x06,0x3c,
	LCM_CMD(0xf2, 6), 0x8f,0x13,0x06,0x06,0x07,0x3b,
	LCM_CMD(0xf1, 6), 0x52,0xbc,0x8f,0x35,0x38,0x4f,
	LCM_CMD(0xf3, 6), 0x54,0xbc,0x8f,0x33,0x3c,0x4f,
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};

static const uint8_t cmd7735_init[] = {
	//LCM_DELAY(120),
	LCM_CMD(0x11, 0), // Sleep Out Mode
	LCM_DELAY(120),
	LCM_CMD(0xb1, 3), 0x01,0x08,0x05,
	LCM_CMD(0xb2, 3), 0x01,0x08,0x05,
	LCM_CMD(0xb3, 6), 0x01,0x08,0x05,0x05,0x08,0x05,
	LCM_CMD(0xb4, 1), 0x03,
	LCM_CMD(0xc0, 3), 0x28,0x08,0x04,
	LCM_CMD(0xc1, 1), 0xc0,
	LCM_CMD(0xc2, 2), 0x0d,0x00,
	LCM_CMD(0xc3, 2), 0x8d,0x2a,
	LCM_CMD(0xc4, 2), 0x8d,0xee,
	LCM_CMD(0xc5, 1), 0x14,
	//LCM_CMD(0x36, 1), 0xc8, // Memory Access Control
	// Set Gamma 1
	LCM_CMD(0xe0, 16), 0x07,0x18,0x0c,0x15,
		0x2e,0x2a,0x23,0x28, 0x28,0x28,0x2e,0x39, 0x00,0x03,0x02,0x10,
	// Set Gamma 2
	LCM_CMD(0xe1, 16), 0x06,0x23,0x0d,0x17,
		0x35,0x30,0x2a,0x2d, 0x2c,0x29,0x31,0x3b, 0x00,0x02,0x03,0x12,
	LCM_CMD(0x3a, 1), 0x05, // Pixel Format Set
	LCM_CMD(0x29, 0), // Display ON
	LCM_END
};
#endif

static const lcd_config_t lcd_config[] = {
#if CHIP == 1
/* F+ F256 */

	// Sitronix ST7789
	{ 0x858552, 0xffffff, 0, 0, 1,  240, 320, 1, 0, 2,  { 60,  80,  90, 60, 80, 80 }, { 0 },  0x00, cmd8585_init },
	// GlaxyCore GC9305 (untested)
	{ 0x009305, 0xffffff, 0, 0, 1,  240, 320, 1, 0, 2,  { 30, 150, 150, 40, 50, 50 }, { 0 },  0x48, cmd9305_init },
	// GlaxyCore GC9306
	{ 0x009306, 0xffffff, 0, 0, 1,  240, 320, 1, 0, 2,  { 30, 150, 150, 40, 40, 40 }, { 0 },  0x48, cmd9306_init },
	// GlaxyCore GC9300 (untested)
	{ 0x009300, 0xffffff, 0, 0, 1,  240, 320, 1, 0, 2,  { 30, 150, 150, 40, 40, 40 }, { 0 },  0x48, cmd9300_init },

/* F+ Ezzy 4 */

	// GlaxyCore GC9102 (untested)
	{ 0x009102, 0xffffff, 0, 0, 1,  128, 160, 1, 0, 2,  { 60, 80, 90, 60, 80, 80 }, { 0 },  0xd0, cmd9102_init },
	// GlaxyCore GC9106
	{ 0x009106, 0xffffff, 0, 0, 1,  128, 160, 1, 0, 2,  { 60, 80, 90, 60, 80, 80 }, { 39000000 },  0xd0, cmd9106_init },

/* Nokia TA-1174 */

	{ 0x009316, 0xffffff, 1, 2, 5,  128, 160, 0, 0, 2,  { 0 }, { 13000000 }, 0xd8, cmd9316_init },

/* BQ 3586 Tank Max */

	// RenesasSP R61529
	{ 0x01221529, 0xffffffff, 0, 0, 1,  320, 480, 1, 0, 2,  { 170, 170, 250, 8, 15, 15 }, { 0 }, 0x00, cmd1529_init },
#endif

#if CHIP == 2
/* Joy's S21 */

	// GlaxyCore GC9106
	{ 0x009106, 0xffffff, 0, 0, 0,  128, 160, 1, 0, 2,  { 30, 150, 150, 40, 50, 50 }, { 0 },  0xd0, cmd9106_init },
	// GlaxyCore GC9108
	{ 0x009108, 0xffffff, 0, 0, 0,  128, 160, 1, 0, 2,  { 30, 150, 150, 40, 50, 50 }, { 0 },  0xd0, cmd9108_init },

/* Vector M115 */

	// Sitronix ST7735S
	{ 0x7c89f0, 0xffffff, 0, 0, 1,  128 + 2, 128 + 3, 1, 0, 2,  { 15, 120, 75, 15, 35, 35 }, { 0 }, 0xc8, cmd7735_init },

/* DZ09 */

	// GlaxyCore GC9307
	{ 0x009307, 0xffffff, 2, 17, 5,  240, 240, 0, 0, 2,  { 0 }, { 48000000 }, 0x48, cmd9307_init },
#endif
};

