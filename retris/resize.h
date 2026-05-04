
static inline unsigned gamma_src(unsigned x) {
#if FAST_GAMMA
	return x * x;
#else
	return gamma_src1[x];
#endif
}

static inline unsigned gamma_dest(unsigned x) {
	unsigned i = 128, a = 255;
	do { unsigned b = a - i;
#if FAST_GAMMA
		if (x < b * b + b) a = b;
#else
		if (x <= gamma_dest2[b]) a = b;
#endif
	} while ((i /= 2));
	return a;
}

static void image_resize(const uint8_t *src, unsigned w, unsigned h,
		uint8_t *dest, unsigned w2, unsigned h2) {
	unsigned x, y, xx, yy, x1, y1, x2, y2, x3, y3;
	unsigned m1 = 8, m, div, sum, sumx;

	do {
		y2 = ((h << m1) + h2 - 1) / h2;
		x2 = ((w << m1) + w2 - 1) / w2;
	} while (y2 * x2 > 0x10000 && --m1);
	m = (1 << m1) - 1;
	//printf("m1 = %u\n", m1);

	for (y1 = y = 0; y < h2; y++, y1 = y2) {
		y2 = ((y + 1) << m1) * h / h2;
		for (x1 = x = 0; x < w2; x++, x1 = x2) {
			x2 = ((x + 1) << m1) * w / w2;
			sum = 0;
			for (yy = y1; yy < y2; yy = y3) {
				const uint8_t *p = &src[(yy >> m1) * w + (x1 >> m1)];
#if 0
				sumx = 0; xx = x1; do {
					x3 = (xx | m) + 1; if (x3 > x2) x3 = x2;
					sumx += gamma_src(*p++) * (x3 - xx);
				} while ((xx = x3) < x2);
#else
				unsigned a = 0, b; (void)x3;
				sumx = gamma_src(*p) * (x1 & m);
				xx = ((x2 + m) >> m1) - (x1 >> m1);
				do a += b = gamma_src(*p++); while (--xx);
				sumx = (a << m1) - sumx - b * ((0u - x2) & m);
#endif
				y3 = (yy | m) + 1; if (y3 > y2) y3 = y2;
				sum += sumx * (y3 - yy);
			}
			div = (x2 - x1) * (y2 - y1);
			dest[y * w2 + x] = gamma_dest((sum + div / 2) / div);
		}
	}
}

