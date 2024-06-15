#ifndef COMMON_H
#define COMMON_H

extern uint32_t wrandomseed;

static inline void wsrand(int seed) {
	wrandomseed = seed;
}

// This aims to mimic Watcom C's implementation of rand
static inline int wrand(void) {
	wrandomseed = 1103515245 * wrandomseed + 12345;
	return (wrandomseed >> 16) & 0x7fff;
}

#define strnlen strnlen_new
static inline size_t strnlen(const char *s, size_t len) {
	size_t n = 0;
	while (n < len && s[n]) n++;
	return n;
}

#endif
