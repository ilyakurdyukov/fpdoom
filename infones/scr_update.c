#include <stdint.h>

#define DEF(ret, name, args) \
ret name##_asm args; ret name args

DEF(void, scr_update_1d1, (void *src, uint16_t *dst, unsigned h)) {
	uint32_t *s = (uint32_t*)src;
	uint32_t *d = (uint32_t*)dst;
	unsigned n = h << 8;
	do {
		uint32_t a = *s++;
		a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
		*d++ = a;
	} while ((n -= 2));
}

// pixel aspect ratio 4:3
DEF(void, scr_update_4x3d3, (void *src, uint16_t *dst, unsigned h)) {
	uint16_t *s = (uint16_t*)src + 8;
	uint32_t r = 0x400801, m = 0x07c0f81f;
	do {
		h -= 240 << 16;
		do {
			uint32_t a, aa;
			a = s[0]; a += a & ~0x1f; *dst++ = a;
			a = s[1] | s[2] << 16; s += 3;
			a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
			aa = (a >> 16 | a << 16) & m; aa += a & m;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			dst[0] = a;
			dst[1] = aa | aa >> 16;
			dst[2] = a >> 16;
			dst += 3;
		} while ((int32_t)(h += 3 << 16) < 0);
		s += 16;
	} while (--h);
}

DEF(void, scr_update_1d2, (void *src, uint16_t *dst, unsigned h)) {
	uint32_t *s = (uint32_t*)src;
	uint32_t a, b, c, w = 256;
	do {
		h -= w << 16;
		do {
			a = *s; b = s[128]; s++;
			c = (a & 0x03e07c1f) + (b & 0x03e07c1f);
			a = (a & 0x7c1f03e0) + (b & 0x7c1f03e0);
			a = c + (a >> 16 | a << 16);
			// rounding half to even
			a += (0x200401 & a >> 2) + 0x200401;
			a >>= 2; a &= 0x03e07c1f;
			a |= a >> 16;
			*dst++ = a + (a & ~0x1f);
		} while ((int32_t)(h += 2 << 16) < 0);
		s += 128;
	} while ((h -= 2));
}

// pixel aspect ratio 4:3
DEF(void, scr_update_4x3d6, (void *src, uint16_t *dst, unsigned h)) {
	uint16_t *s = (uint16_t*)src + 8;
	do {
		h -= 240 << 16;
		do {
			uint32_t a1, a2, a3, b1, b2, b3;
			uint32_t t1, t2, t3, m = 0x1f001f;
			a1 = s[0]; a2 = s[1]; a3 = s[2];
			b1 = s[256]; b2 = s[257]; b3 = s[258];
			s += 3;
			a1 |= a3 << 16;
			a3 = b1 | b3 << 16;
			a2 |= b2 << 16;
			b1 = a2 >> 10 & m;
			b2 = a2 >> 5 & m;
			b3 = a2 & m;
			b1 += b1 >> 16 | b1 << 16;
			b2 += b2 >> 16 | b2 << 16;
			b3 += b3 >> 16 | b3 << 16;
			t1 = (a1 >> 10 & m) + (a3 >> 10 & m);
			t2 = (a1 >> 5 & m) + (a3 >> 5 & m);
			t3 = (a1 & m) + (a3 & m);
			b1 += t1 * 2;
			b2 += t2 * 2;
			b3 += t3 * 2;
#define X(a) a = a * 0xab + 0x1c001c0; \
	a += a >> 4 & 0x400040; a = a >> 10 & m;
			X(b1) X(b2) X(b3)
#undef X
			a1 = b1 << 11 | b2 << 6 | b3;
			*(uint32_t*)dst = a1; dst += 2;
		} while ((int32_t)(h += 3 << 16) < 0);
		s += 256 + 16;
	} while ((h -= 2));
}

// There is no perfect solution for this scaling.
// Any method will cause distortion on scrolling.
DEF(void, scr_update_4d3, (void *src, uint16_t *dst, unsigned h)) {
	uint16_t *s = (uint16_t*)src;
	do {
		uint32_t r = 0x400801, m = 0x07c0f81f;
		uint32_t a, b, aa, bb, ab, a0, a1, b0, b1;

		h -= 255 << 16;
		do {
			uint16_t *dst1 = dst + 342, *dst2 = dst + 342 * 2;
			a = s[0] | s[256] << 16;
			a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
			aa = (a >> 16 | a << 16) & m; aa += a & m;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			*dst++ = a;
			*dst1++ = aa | aa >> 16;
			*dst2++ = a >> 16;

			a = s[1] | s[2] << 16; b = s[257] | s[258] << 16; s += 3;
			a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
			b = (b & 0x7fff7fff) + (b & 0x7fe07fe0);
			a1 = (a >> 16 | a << 16) & m; a0 = a & m;
			b1 = (b >> 16 | b << 16) & m; b0 = b & m;
			aa = a0 + a1; bb = b0 + b1;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			bb += r & bb >> 1; bb = bb >> 1 & m;
			dst[0] = a;
			dst[1] = aa | aa >> 16;
			dst[2] = a >> 16;
			dst2[0] = b;
			dst2[1] = bb | bb >> 16;
			dst2[2] = b >> 16;
			aa = a0 + b0; bb = a1 + b1; ab = aa + bb;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			bb += r & bb >> 1; bb = bb >> 1 & m;
			aa |= bb >> 16 | bb << 16;
			ab += (r & ab >> 2) + r; ab = ab >> 2 & m;
			dst1[0] = aa;
			dst1[1] = ab | ab >> 16;
			dst1[2] = aa >> 16;
			dst += 3;
		} while ((int32_t)(h += 3 << 16) < 0);

		a = s[0] | s[256] << 16; s++;
		a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
		aa = (a >> 16 | a << 16) & m; aa += a & m;
		aa += r & aa >> 1; aa = aa >> 1 & m;
		dst[0] = a;
		dst[342] = aa | aa >> 16;
		dst[342 * 2] = a >> 16;
		dst += 2;

		// 240 - 16 = 74 * 3 + 2
		if (!(h -= 2)) break;

		s += 256; dst += 342 * 2;
		h -= 255 << 16;
		do {
			a = s[0]; a += a & ~0x1f; *dst++ = a;
			a = s[1] | s[2] << 16; s += 3;
			a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
			aa = (a >> 16 | a << 16) & m; aa += a & m;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			dst[0] = a;
			dst[1] = aa | aa >> 16;
			dst[2] = a >> 16;
			dst += 3;
		} while ((int32_t)(h += 3 << 16) < 0);
		a = *s++; a += a & ~0x1f;
		*dst = a; dst += 2;
	} while (--h);
}

// pixel aspect ratio 9:8
DEF(void, scr_update_9x8d6, (void *src, uint16_t *dst, unsigned h)) {
	uint32_t *s = (uint32_t*)src;
	do {
		uint32_t r = 0x400801, m = 0x07c0f81f;
		uint32_t a, b, aa, bb, ab, a0, a1, b0, b1;

		h -= 256 << 16;
		do {
			uint16_t *dst1 = dst + 0x180, *dst2 = dst + 0x180 * 2;
			a = *s; b = s[128]; s++;
			a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
			b = (b & 0x7fff7fff) + (b & 0x7fe07fe0);
			a1 = (a >> 16 | a << 16) & m; a0 = a & m;
			b1 = (b >> 16 | b << 16) & m; b0 = b & m;
			aa = a0 + a1; bb = b0 + b1;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			bb += r & bb >> 1; bb = bb >> 1 & m;
			dst[0] = a;
			dst[1] = aa | aa >> 16;
			dst[2] = a >> 16;
			dst2[0] = b;
			dst2[1] = bb | bb >> 16;
			dst2[2] = b >> 16;
			aa = a0 + b0; bb = a1 + b1; ab = aa + bb;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			bb += r & bb >> 1; bb = bb >> 1 & m;
			aa |= bb >> 16 | bb << 16;
			ab += (r & ab >> 2) + r; ab = ab >> 2 & m;
			dst1[0] = aa;
			dst1[1] = ab | ab >> 16;
			dst1[2] = aa >> 16;
			dst += 3;
		} while ((int32_t)(h += 2 << 16) < 0);

		// 240 - 16 = 74 * 3 + 2
		if (!(h -= 2)) break;

		s += 128; dst += 0x180 * 2;
		h -= 256 << 16;
		do {
			a = *s; s++;
			a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
			a1 = (a >> 16 | a << 16) & m; a0 = a & m;
			aa = a0 + a1;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			dst[0] = a;
			dst[1] = aa | aa >> 16;
			dst[2] = a >> 16;
			dst += 3;
		} while ((int32_t)(h += 2 << 16) < 0);
	} while (--h);
}

DEF(void, scr_update_3d2, (void *src, uint16_t *dst, unsigned h)) {
	uint32_t *s = (uint32_t*)src;
	uint32_t a, b, w = 256;
	do {
		h -= w << 16;
		do {
			uint16_t *dst1 = dst + 0x180, *dst2 = dst + 0x180 * 2;
			uint32_t r = 0x400801, m = 0x07c0f81f;
			uint32_t aa, bb, ab, a0, a1, b0, b1;
			a = *s; b = s[128]; s++;
			a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
			b = (b & 0x7fff7fff) + (b & 0x7fe07fe0);
			a1 = (a >> 16 | a << 16) & m; a0 = a & m;
			b1 = (b >> 16 | b << 16) & m; b0 = b & m;
			aa = a0 + a1; bb = b0 + b1;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			bb += r & bb >> 1; bb = bb >> 1 & m;
			dst[0] = a;
			dst[1] = aa | aa >> 16;
			dst[2] = a >> 16;
			dst2[0] = b;
			dst2[1] = bb | bb >> 16;
			dst2[2] = b >> 16;
			aa = a0 + b0; bb = a1 + b1; ab = aa + bb;
			aa += r & aa >> 1; aa = aa >> 1 & m;
			bb += r & bb >> 1; bb = bb >> 1 & m;
			aa |= bb >> 16 | bb << 16;
			ab += (r & ab >> 2) + r; ab = ab >> 2 & m;
			dst1[0] = aa;
			dst1[1] = ab | ab >> 16;
			dst1[2] = aa >> 16;
			dst += 3;
		} while ((int32_t)(h += 2 << 16) < 0);
		s += 128; dst += 0x180 * 2;
	} while ((h -= 2));
}

// pixel aspect ratio 4:3
DEF(void, scr_update_4x3d2, (void *src, uint16_t *dst, unsigned h)) {
	uint32_t *s = (uint32_t*)src + 4;
	uint32_t *d = (uint32_t*)dst;
	do {
		h -= 240 << 16;
		do {
			uint32_t *d1 = d + 240, *d2 = d + 240 * 2;
			uint32_t r = 0x08410841, m = ~(r >> 1);
			uint32_t a, b, c;
			a = *s; b = s[128]; s++;
			a = (a & 0x7fff7fff) + (a & 0x7fe07fe0);
			b = (b & 0x7fff7fff) + (b & 0x7fe07fe0);
			c = a ^ (a >> 16 | a << 16);
			d[0] = a ^ c << 16;
			d[1] = a ^ c >> 16;
			c = b ^ (b >> 16 | b << 16);
			d2[0] = b ^ c << 16;
			d2[1] = b ^ c >> 16;
			// This could be a meme: "we have SIMD at home"
			c = ((a >> 1) & m) + ((b >> 1) & m);
			// rounding half to even
			c += ((c & (a | b)) | (a & b)) & r;
			a = c ^ (c >> 16 | c << 16);
			d1[0] = c ^ a << 16;
			d1[1] = c ^ a >> 16;
			d += 2;
		} while ((int32_t)(h += 2 << 16) < 0);
		s += 128 + 8; d += 480;
	} while ((h -= 2));
}

#undef DEF
#ifdef USE_ASM
#define scr_update_1d1 scr_update_1d1_asm
#define scr_update_4x3d3 scr_update_4x3d3_asm
#define scr_update_1d2 scr_update_1d2_asm
#define scr_update_4x3d6 scr_update_4x3d6_asm
#define scr_update_4d3 scr_update_4d3_asm
#define scr_update_9x8d6 scr_update_9x8d6_asm
#define scr_update_3d2 scr_update_3d2_asm
#define scr_update_4x3d2 scr_update_4x3d2_asm
#endif

typedef void (*scr_update_t)(void *src, uint16_t *dst, unsigned h);

const scr_update_t scr_update_fn[] = {
	scr_update_1d1, scr_update_4x3d3,
	scr_update_1d2, scr_update_4x3d6,
	scr_update_4d3, scr_update_9x8d6,
	scr_update_3d2, scr_update_4x3d2
};


