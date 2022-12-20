#include "common.h"
#include "syscode.h"
#include "doomkey.h"

enum {
	KEYPAD_DIAL = 0x01,
	KEYPAD_UP = 0x04,
	KEYPAD_DOWN = 0x05,
	KEYPAD_LEFT = 0x06,
	KEYPAD_RIGHT = 0x07,
	KEYPAD_LSOFT = 0x08,
	KEYPAD_RSOFT = 0x09,
	KEYPAD_PLAY = 0x0c, // PAUSE
	KEYPAD_CENTER = 0x0d,
	KEYPAD_NEXT = 0x17,
	KEYPAD_PREV = 0x18,
	// 0x1a, 0x1d ???
	KEYPAD_HASH = 0x23, // '#'
	KEYPAD_STAR = 0x2a, // '*'
	KEYPAD_PLUS = 0x2b, // '+', SOS
	KEYPAD_0 = 0x30,
	KEYPAD_1 = 0x31,
	KEYPAD_2 = 0x32,
	KEYPAD_3 = 0x33,
	KEYPAD_4 = 0x34,
	KEYPAD_5 = 0x35,
	KEYPAD_6 = 0x36,
	KEYPAD_7 = 0x37,
	KEYPAD_8 = 0x38,
	KEYPAD_9 = 0x39,
};

void keytrn_init(void) {
	short *keymap = sys_data.keymap_addr;
	int i, j, rotate = sys_data.rotate >> 4 & 3;
	static const unsigned char num_turn[4][4 + 9] = {
		{ "\004\005\006\007123456789" },
		{ "\007\006\004\005369258147" },
		{ "\005\004\007\006987654321" },
		{ "\006\007\005\004741852963" } };
	uint8_t rkeymap[64];
	int nrow = _chip == 2 ? 8 : 5;
	int ncol = _chip == 2 ? 6 : 8;

	memset(rkeymap, 64, sizeof(rkeymap));

	for (i = 0; i < ncol; i++)
	for (j = 0; j < nrow; j++) {
		unsigned a = keymap[i * nrow + j];
		if (a < sizeof(rkeymap)) {
			if (a - KEYPAD_UP < 4)
				a = num_turn[rotate][a - KEYPAD_UP];
			else if (a - KEYPAD_1 < 9)
				a = num_turn[rotate][a - KEYPAD_1 + 4];
			rkeymap[a] = j * 8 + i;
		}
	}

	memset(sys_data.keytrn, 0, 65 * 2);

#define KEY(name) sys_data.keytrn[rkeymap[KEYPAD_##name]]
	KEY(UP) = KEY_UPARROW;
	KEY(LEFT) = KEY_LEFTARROW;
	KEY(RIGHT) = KEY_RIGHTARROW;
	KEY(DOWN) = KEY_DOWNARROW;

	KEY(2) = KEY_UPARROW;
	KEY(4) = KEY_LEFTARROW;
	KEY(6) = KEY_RIGHTARROW;
	KEY(5) = KEY_DOWNARROW;

	KEY(LSOFT) = KEY_ENTER;
	KEY(RSOFT) = KEY_ESCAPE;

	// no center key
	if (rkeymap[KEYPAD_CENTER] == 64) {
		KEY(UP) = KEY_RCTRL;	/* fire */
		KEY(DOWN) = ' ';	/* use */
	} else {
		KEY(CENTER) = KEY_RCTRL;	/* fire */
	}
	KEY(DIAL) = KEY_RSHIFT;	/* run toggle */

	KEY(1) = ',';	/* strafe left */
	KEY(3) = '.';	/* strafe right */

	//KEY(8) = ' ';	/* use */
	KEY(7) = '['; /* prev weapon */
	KEY(9) = ']'; /* next weapon */
#undef KEY
}

