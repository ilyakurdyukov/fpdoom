
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
	unsigned m1 = 8, m, div, sum;

	for (; m1 > 1; m1--) {
		m = 1 << m1;
		y2 = (m * h + h2 - 1) / h2;
		x2 = (m * w + w2 - 1) / w2;
		if (y2 * x2 < 0x10000) break;
	}
	//printf("m1 = %u\n", m);

	for (y1 = y = 0; y < h2; y++, y1 = y2) {
		y2 = (y + 1) * m * h / h2;
		for (x1 = x = 0; x < w2; x++, x1 = x2) {
			x2 = (x + 1) * m * w / w2;
			sum = 0;
			for (yy = y1; yy < y2; yy += y3) {
				y3 = ((yy ^ y2) < m ? y2 & (m - 1) : m) - (yy & (m - 1));
				for (xx = x1; xx < x2; xx += x3) {
					x3 = ((xx ^ x2) < m ? x2 & (m - 1) : m) - (xx & (m - 1));
					sum += gamma_src(src[(yy >> m1) * w + (xx >> m1)]) * x3 * y3;
				}
			}
			div = (x2 - x1) * (y2 - y1);
			dest[y * w2 + x] = gamma_dest((sum + div / 2) / div);
		}
	}
}

