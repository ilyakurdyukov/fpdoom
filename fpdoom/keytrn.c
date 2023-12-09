#include "common.h"
#include "syscode.h"
#include "doomkey.h"

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

void keytrn_init(void) {
	uint8_t keymap[64], rkeymap[64];
	static const uint8_t keys[] = {
#define KEY(name, val) KEYPAD_##name, val,
		KEY(UP, KEY_UPARROW)
		KEY(LEFT, KEY_LEFTARROW)
		KEY(RIGHT, KEY_RIGHTARROW)
		KEY(DOWN, KEY_DOWNARROW)
		KEY(2, KEY_UPARROW)
		KEY(4, KEY_LEFTARROW)
		KEY(6, KEY_RIGHTARROW)
		KEY(5, KEY_DOWNARROW)
		KEY(LSOFT, KEY_ENTER)
		KEY(RSOFT, KEY_ESCAPE)
		KEY(CENTER, KEY_RCTRL) /* fire */
		KEY(DIAL, KEY_RSHIFT) /* run toggle */
		KEY(1, ',') /* strafe left */
		KEY(3, '.') /* strafe right */
		//KEY(8, ' ') /* use */
		KEY(7, '[') /* prev weapon */
		KEY(9, ']') /* next weapon */
		KEY(PLUS, KEY_TAB) /* map */
	};
	static const uint8_t keys_power[] = {
		KEY(UP, KEY_RSHIFT)
		KEY(LEFT, '[')
		KEY(RIGHT, ']')
		KEY(DOWN, KEY_ESCAPE)
		KEY(CENTER, KEY_ENTER)
#undef KEY
	};
	int i, flags = sys_getkeymap(keymap);

#define FILL_RKEYMAP(keys) \
	memset(rkeymap, 0, sizeof(rkeymap)); \
	for (i = 0; i < (int)sizeof(keys); i += 2) \
		rkeymap[keys[i]] = keys[i + 1];

	FILL_RKEYMAP(keys)

#define KEY(name) rkeymap[KEYPAD_##name]
	// no center key
	if (!(flags & 1)) {
		KEY(UP) = KEY_RCTRL;	/* fire */
		KEY(DOWN) = KEY_RCTRL;	/* fire */
	}
#undef KEY

#define FILL_KEYTRN(j) \
	for (i = 0; i < 64; i++) { \
		unsigned a = keymap[i]; \
		sys_data.keytrn[j][i] = a < 64 ? rkeymap[a] : 0; \
	}

	FILL_KEYTRN(0)
	FILL_RKEYMAP(keys_power)
	FILL_KEYTRN(1)
#undef FILL_RKEYMAP
#undef FILL_KEYTRN
}

