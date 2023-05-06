#include <stdio.h>
#include <stdint.h>
#include <math.h>

unsigned char britable[16][256];
short sintable[2048], radarang[1280];

static uint32_t crc32once(const void *src, size_t n) {
	const uint8_t *s = src; int j; uint32_t c;
	for (c = ~0; n--;)
		for (c ^= *s++, j = 8; j--;)
			c = c >> 1 ^ ((0 - (c & 1)) & 0xedb88320);
	return ~c;
}

static void calcbritable(void) {
	int i, j; double a, b;
	for (i = 0; i < 16; i++) {
		a = (double)8 / ((double)i + 8);
		b = (double)255 / pow((double)255, a);
		for (j = 0; j < 256; j++)
			britable[i][j] = (unsigned char)(pow((double)j, a) * b);
	}
}

static int calctables(void) {
	int i;
	for (i = 0; i < 2048; i++) {
		sintable[i] = (short)(16384 * sin((double)i * 3.14159265358979 / 1024));
	}
	for (i = 0; i < 640; i++) {
		radarang[i] = (short)(atan(((double)i - 639.5) / 160) * 64 * 1024 / 3.14159265358979);
		radarang[1279 - i] = -radarang[i];
	}
	calcbritable();

	if (crc32once(sintable, sizeof(sintable)) != 0xee1e7aba) {
		puts("Calculation of sintable yielded unexpected results.");
		return 1;
	}
	if (crc32once(radarang, sizeof(radarang)/2) != 0xee893d92) {
		puts("Calculation of radarang yielded unexpected results.");
		return 1;
	}
	return 0;
}

int main(int argc, char **argv) {
	const char *fn = "engine_tables.h";
	unsigned i, j;
	FILE *f;
	if (argc >= 2) fn = argv[1];

	if (calctables()) return 1;

	f = fopen(fn, "wb");
	if (!f) return 1;

#define PRINTARR(arr, n) \
	for (j = 0; j < n; j++) { \
		fprintf(f, "%i, ", arr[j]); \
		if ((j + 1) % 8 == 0) fprintf(f, "\n"); \
	}

	fprintf(f, "const unsigned char britable[16][256] = { {\n");
	for (i = 0; i < 16; i++) {
		if (i) fprintf(f, "}, {\n");
		PRINTARR(britable[i], 256)
	}
	fprintf(f, "} };\n");
	fprintf(f, "const short sintable[2048] = {\n");
	PRINTARR(sintable, 2048)
	fprintf(f, "};\n");
	fprintf(f, "const short radarang[1280] = {\n");
	PRINTARR(radarang, 1280)
	fprintf(f, "};\n");
	fclose(f);
}

