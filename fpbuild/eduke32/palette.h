#ifndef PALETTE_H
#define PALETTE_H

#include "common_game.h"
#ifdef kMaxPAL
#define BASEPALCOUNT kMaxPAL
#else
#define BASEPALCOUNT 5
#endif

extern uint8_t basepaltable[BASEPALCOUNT][768];

void paletteSetColorTable(int32_t id, const uint8_t *table);

static inline
void videoSetPalette(char dabrightness, uint8_t dapalid, uint8_t flags) {
	setbrightness(dabrightness, basepaltable[dapalid], flags);
}

#define paletteFreeLookupTable(x)

#endif
