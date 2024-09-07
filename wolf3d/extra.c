#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define ANGLES 360
#define FINEANGLES 3600
// the original code doesn't round
#define ROUNDANGLE 1

int CalcRotateMult;

#if NO_FLOAT
extern const uint8_t atan2_tab[0x801];

static unsigned frac_divide(unsigned y, unsigned x, int n) {
	unsigned a = 0;
	do {
		a <<= 1;
		if (y >= x) y -= x, a++;
		y <<= 1;
	} while (--n >= 0);
	// round to nearest even
	if (y + (a & 1) > x) a++;
	return a;
}

static int atan2i(int y, int x) {
	unsigned t, f = 0x10123432 | 0x80080880;
	if (y < 0) y = -y, f >>= 16;
	if (x < 0) x = -x, f >>= 8;
	if (x < y) t = x, x = y, y = t, f >>= 4;
	x = frac_divide(y, x, 11);
	x += atan2_tab[x];
	if (f & 8) x = -x;
	return ((f & 7) << 12) + x;
}

// 7.5% chance of deviation by 1

int CalcAngle(int32_t y, int32_t x) {
	int angle = atan2i(y, x) - 0x2000;
	if (angle < 0) angle += 0x4000;
	angle *= ANGLES;
	if (ROUNDANGLE) angle += 0x2000;
	return angle >> 14;
}

int ProjectionAngle(int32_t y, int32_t x) {
	int angle = atan2i(y, x) - 0x2000;
	angle *= FINEANGLES;
	if (ROUNDANGLE) angle += 0x2000;
	else if (angle < 0) angle += 0x3fff;
	return angle >> 14;
}
#else
#include <math.h>

int CalcAngle(int32_t y, int32_t x) {
	float angle = atan2f(y, x);
	if (angle < 0) angle += (float)(M_PI * 2);
	angle *= (float)(ANGLES / (M_PI * 2));
	if (ROUNDANGLE) angle += 0.5f;
	return angle;
}

int ProjectionAngle(int32_t y, int32_t x) {
	float angle = atan2f(y, x);
	angle *= (float)(FINEANGLES / (M_PI * 2));
	if (ROUNDANGLE) angle += angle >= 0 ? 0.5f : -0.5f;
	return angle;
}
#endif

#if LINUX_FN_FIX
FILE *fopen_fix(const char *path, const char *mode) {
	FILE *f = fopen(path, mode);
	// printf("!!! fopen(\"%s\", \"%s\")\n", path, mode);
	if (!f && *mode == 'r') {
		char buf[64]; unsigned i = 0, a;
		do {
			if (i >= sizeof(buf)) return f;
			a = path[i]; buf[i++] = toupper(a);
		} while (a);
		f = fopen(buf, mode);
	}
	return f;
}
#endif

void Error(const char *str) {
	printf("error: %s\n", str);
}

void Help(const char *str) {
	printf("info: %s\n", str);
}

#if TRACE_UPDATE
void print_trace(const char *file, int line) {
	static const char *file1;
	static int line1;
  if (line != line1 || !file1 || strcmp(file, file1)) {
		file1 = file; line1 = line;
		printf("%s:%u\n", file, line);
	}
}
#endif

