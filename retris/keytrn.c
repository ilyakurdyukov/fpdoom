#include <string.h>
#include "syscode.h"

#define X(num, name) KEYPAD_##name = num,
enum { KEYPAD_ENUM(X) };
#undef X

enum {
	KEY_END = 0,
	KEY_ROTL, KEY_ROTR,
	KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_DROP,
	KEY_PAUSE, KEY_RESTART, KEY_QUIT,
	KEY_REWIND
};

void keytrn_init(void) {
	uint8_t keymap[64], rkeymap[64];
	static const uint8_t keys[] = {
#define KEY(name, val) KEYPAD_##name, val,
		KEY(UP, KEY_ROTR)
		KEY(LEFT, KEY_LEFT)
		KEY(RIGHT, KEY_RIGHT)
		KEY(DOWN, KEY_DOWN)
		KEY(CENTER, KEY_DROP)
		KEY(LSOFT, KEY_ROTL)
		KEY(RSOFT, KEY_ROTR)
		KEY(DIAL, KEY_REWIND)
		KEY(1, KEY_ROTL)
		KEY(2, KEY_ROTR)
		KEY(3, KEY_ROTR)
		KEY(4, KEY_LEFT)
		KEY(5, KEY_DOWN)
		KEY(6, KEY_RIGHT)
		KEY(7, KEY_LEFT)
		KEY(8, KEY_DOWN)
		KEY(9, KEY_RIGHT)
		KEY(0, KEY_DROP)
	};
	static const uint8_t keys_power[] = {
		KEY(UP, KEY_RESTART)
		KEY(DOWN, KEY_QUIT)
		KEY(LEFT, KEY_REWIND)
		KEY(RIGHT, KEY_PAUSE)
		KEY(CENTER, KEY_PAUSE)
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
#if 0
		KEY(UP) = KEY_DROP;
		KEY(DOWN) = KEY_DROP;
#endif
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

