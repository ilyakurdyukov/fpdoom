#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define ERR_EXIT(...) \
	do { fprintf(stderr, __VA_ARGS__); \
	exit(1); } while (0)

static int pnm_next(FILE *f) {
	int n = -1;
	for (;;) {
		int a = fgetc(f);
		if (a == '#') do a = fgetc(f); while (a != '\n' && a != '\r' && a != EOF);
		if ((unsigned)a - '0' < 10) {
			if (n < 0) n = 0;
			if ((n = n * 10 + a - '0') >= 1 << 16) break;
		} else if (a == ' ' || a == '\n' || a == '\r' || a == '\t') {
			if (n >= 0) return n;
		} else if (a == EOF) return n;
		else break;
	}
	return -1;
}

static uint8_t* read_pgm(const char *fn, unsigned *dim) {
	FILE *f = NULL; uint8_t *data = NULL;
	do {
		unsigned t, w, h;
		if (!fn || !(f = fopen(fn, "rb"))) break;
		if (fgetc(f) != 'P') break;
		if (fgetc(f) != '5') break;
		dim[0] = w = pnm_next(f);
		if ((w - 1) >> 15) break;
		dim[1] = h = pnm_next(f);
		if ((h - 1) >> 15) break;
		if (pnm_next(f) != 255)
			ERR_EXIT("image max value must be 255\n");
		t = w * h;
		if (!(data = malloc(t))) break;
		if (fread(data, 1, t, f) != t) goto err;
		goto ok;
	} while (0);
err:
	if (data) { free(data); data = NULL; }
ok:
	if (f) fclose(f);
	return data;
}

#define FAST_GAMMA 0
#if !FAST_GAMMA
#define GAMMA_DEPTH 65536
static uint16_t gamma_src1[256];
static uint8_t gamma_dest1[GAMMA_DEPTH];
static uint16_t gamma_dest2[256];
#endif

#include "resize.h"

int main(int argc, char **argv) {
	int x, y, w, h, num;
	uint8_t *image, *p;
	unsigned dim[2];
#if !FAST_GAMMA
	float gamma = 2.2;
#endif

	if (argc < 2) return 1;
	image = read_pgm(argv[1], dim);
	if (!image) ERR_EXIT("read_pgm failed\n");

#if !FAST_GAMMA
	for (x = 0; x < 256; x++)
		gamma_src1[x] = powf(x / 255.0, gamma) * (GAMMA_DEPTH - 1) + 0.5f;
	for (x = 0; x < GAMMA_DEPTH; x++)
		gamma_dest1[x] = powf(x / (float)(GAMMA_DEPTH - 1), 1 / gamma) * 255 + 0.5f;

	for (x = y = 0; x < 256; x++) {
		while (y < GAMMA_DEPTH && x == gamma_dest1[y]) y++;
		gamma_dest2[x] = y - 1;
		//printf("%u : %u\n", x, y - 1);
	}

	for (x = 0; x < GAMMA_DEPTH; x++) {
		y = gamma_dest(x);
		if (y != gamma_dest1[x])
			printf("!!! %u : %u, %u\n", x, gamma_dest1[x], y);
	}
#endif

	if (argc == 2) {
#if !FAST_GAMMA
		printf("static const uint16_t gamma_src1[] = {\n");
		for (x = 0; x < 256; x++)
			printf("\t%u,\n", gamma_src1[x]);
		printf("};\n");
		printf("static const uint16_t gamma_dest2[] = {\n");
		for (x = 0; x < 256; x++)
			printf("\t%u,\n", gamma_dest2[x]);
		printf("};\n");
#endif
		p = image; w = dim[0]; h = dim[1];
		printf("static const struct { "
				"uint16_t w, h; uint8_t data[%u]; } "
				"scr_tiles = {\n", w * h);
		printf("\t%u, %u, {\n", w, h);
		for (y = 0; y < h; y++) {
			printf("\t");
			for (x = 0; x < w; x++)
				printf("0x%02x,", *p++);
			printf("\n");
		}
		printf("} };\n");
	} else if (argc == 5) {
		num = atoi(argv[2]);
		w = atoi(argv[3]);
		h = num * w;
		p = malloc(w * h);
		if (!p) return 1;
		image_resize(image, dim[0], dim[1], p, w, h);
		{
			FILE *f = fopen(argv[4], "wb");
			if (f) {
				fprintf(f, "P5\n%u %u\n255\n", w, h);
				fwrite(p, 1, w * h, f);
				fclose(f);
			}
		}
		free(p);
	}
	free(image);
}
