#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <elf.h>

#define ERR_EXIT(...) \
	do { fprintf(stderr, "!!! " __VA_ARGS__); exit(1); } while (0)

#define DBG_LOG(...) fprintf(stderr, __VA_ARGS__)

static unsigned Elf32_FindRel(void *map, size_t size, uint32_t *buf) {
	Elf32_Ehdr *elf = (Elf32_Ehdr*)map;
	Elf32_Shdr *shdr = (Elf32_Shdr*)((char*)elf + elf->e_shoff);
	unsigned i, n = 0;

	if (size < sizeof(Elf32_Ehdr) ||
			memcmp(elf->e_ident, ELFMAG, SELFMAG) ||
			elf->e_shentsize != sizeof(Elf32_Shdr) ||
			size < elf->e_shoff ||
			(size - elf->e_shoff) / sizeof(Elf32_Shdr) < elf->e_shnum)
		ERR_EXIT("bad ELF binary\n");

	for (i = 0; i < elf->e_shnum; i++) {
		int sh_type = shdr[i].sh_type;
		if (sh_type == SHT_REL || sh_type == SHT_RELA) {
			char *ptr = (char*)elf + shdr[i].sh_offset;
			int relsize = sh_type == SHT_REL ? sizeof(Elf32_Rel) : sizeof(Elf32_Rela);
			int k, nreloc = shdr[i].sh_size / relsize;

			if (!buf)
				DBG_LOG("found %s (%u)\n", sh_type == SHT_REL ? "SHT_REL" : "SHT_RELA", i);

			if (size < shdr[i].sh_offset ||
					size - shdr[i].sh_offset < shdr[i].sh_size)
				ERR_EXIT("section out of bounds\n");

			for (k = 0; k < nreloc; k++, ptr += relsize) {
				Elf32_Rela *rel = (Elf32_Rela*)ptr;
				//int sym = ELF32_R_SYM(rel->r_info);
				int type = ELF32_R_TYPE(rel->r_info);
				uint32_t r_offset = rel->r_offset;
				if (type == R_ARM_RELATIVE) {
					if (r_offset & 3) ERR_EXIT("unaligned offset (0x%08x)\n", r_offset);
					//DBG_LOG("0x%08x\n", r_offset);
					if (buf) buf[n] = r_offset;
					n++;
				} else {
					if (!buf)
						DBG_LOG("unknown rel type (0x%08x, type = %i)\n", r_offset, type);
				}
			}
		}
	}
	return n;
}

static char* loadfile(const char *fn, size_t *num) {
	size_t n, j = 0; char *buf = 0;
	FILE *fi = fopen(fn, "rb");
	if (fi) {
		fseek(fi, 0, SEEK_END);
		n = ftell(fi);
		fseek(fi, 0, SEEK_SET);
		buf = (char*)malloc(n);
		if (buf) {
			j = fread(buf, 1, n, fi);
		}
		fclose(fi);
	}
	if (num) *num = j;
	return buf;
}

static int compare(const void *a, const void *b) {
  return *(uint32_t*)a > *(uint32_t*)b;
}

static unsigned pack_reloc(const uint32_t *rel, unsigned n_rel,
		uint8_t *buf, int nbits, unsigned maxrun) {
	unsigned i, n = 2;
	uint32_t c1 = (1 << nbits) - 1, c2 = 0xff - c1;
	unsigned maxrun1 = maxrun / 2, maxrun2 = maxrun / 2;

	if (nbits < 1 || nbits > 7 || c2 <= maxrun)
		ERR_EXIT("bad pack options\n");

	if (buf) buf[0] = nbits, buf[1] = maxrun;

	for (i = 0; i < n_rel; i++, n++) {
		uint32_t a = rel[i], j = 0;

		// 0123456 (maxrun = 4)
		// 43210fe maxrun - a
		// 1212x00 method
		// 3221011 count

		if (maxrun2 && i && a != 1 && a == rel[i - 1]) {
			while (++j < maxrun2 && i + j < n_rel && rel[i + j] == a);
			i += j - 1; a = maxrun - j * 2 + 1;
		} else {
			if (a == 1) {
				while (++j <= maxrun1 && i + j < n_rel && rel[i + j] == a);
				i += --j;
			}
			a = j ? maxrun - j * 2 : a + maxrun;
		}
		for (; a >= c2; a >>= nbits, n++)
			if (buf) buf[n] = a & c1;
		if (buf) buf[n] = a + c1 + 1;
	}
	if (buf) buf[n] = maxrun + c1 + 1;
	return n + 1;
}

#define ERR ERR_EXIT("pack_check failed (%u)\n", __LINE__)
static void pack_check(const uint32_t *rel, unsigned n_rel,
		uint8_t *buf, unsigned len) {
	unsigned c2, nbits, maxrun, i = 2, j = 0, dist = 0;

	if (i > len) ERR;
	nbits = buf[0]; maxrun = buf[1];

	for (;;) {
		int32_t a = maxrun, b, shl = 0;
		do {
			if (i >= len) ERR;
			b = buf[i++];
			a -= b << shl;
			shl += nbits;
		} while (!(b >> nbits));
		if (!(a += 1 << shl)) break;

		b = a >> 1;
		dist = a & 1 ? dist : 1;
		if (b < 0) dist = -a;

		//DBG_LOG("a = %d, dist = %d\n", a, dist);
		do {
			if (j >= n_rel) ERR;
			//DBG_LOG("(%u, %u)\n", rel[j], dist);
			if (rel[j++] != dist) ERR;
		} while (--b >= 0);
	}
	if (i != len) ERR;
}
#undef ERR

#if 0
void apply_reloc(uint32_t *image, const uint8_t *rel, uint32_t diff) {
	unsigned nbits, maxrun; int32_t dist = 0;
	nbits = *rel++; maxrun = *rel++;
	for (;;) {
		int32_t a = maxrun, b, shl = 0;
		do a -= (b = *rel++) << shl, shl += nbits;
		while (!(b >> nbits));
		if (!(a += 1 << shl)) break;
		b = a >> 1;
		dist = a & 1 ? dist : -1;
		if (b < 0) dist = a;
		do *(image -= dist) += diff; while (--b >= 0);
	}
}
#endif

int main(int argc, char **argv) {
	size_t size;
	char *map;
	unsigned n_rel; uint32_t *rel;
	uint8_t *pack_buf; unsigned best_score = ~0;
	if (argc != 3) {
		DBG_LOG("Usage: pack_reloc elf_binary output\n");
		return 1;
	}
	map = loadfile(argv[1], &size);
	if (!map) ERR_EXIT("loadfile failed\n");
	n_rel = Elf32_FindRel(map, size, NULL);
	DBG_LOG("found %u relocations\n", n_rel);
	rel = malloc((n_rel ? n_rel * sizeof(*rel) : 1));
	if (!rel) ERR_EXIT("malloc failed\n");
	Elf32_FindRel(map, size, rel);
  qsort(rel, n_rel, sizeof(*rel), compare);

	{
		Elf32_Ehdr *elf = (Elf32_Ehdr*)map;
		uint32_t e_entry = elf->e_entry, prev = 0;
		unsigned i, j, besti = 1, bestj = 0;
		for (i = 0; i < n_rel; i++) {
			uint32_t off = rel[i];
			if (off < e_entry) ERR_EXIT("reloc < e_entry\n");
			off -= e_entry;
			if (off >= 4 << 20) ERR_EXIT("reloc >= 4MB\n");
			if (off == prev) ERR_EXIT("duplicated reloc\n");
			rel[i] = (off - prev) >> 2;
			//DBG_LOG("0x%08x\n", rel[i]);
			prev = off;
		}

		for (j = 0; j < 128; j += 8)
		for (i = 1; i <= 7; i++) {
			unsigned score = pack_reloc(rel, n_rel, NULL, i, j);
			if (best_score > score) {
				best_score = score; besti = i; bestj = j;
				DBG_LOG("pack(%u, %u) : %u\n", i, j, score);
			}
		}
		pack_buf = (uint8_t*)malloc(best_score);
		if (!pack_buf) ERR_EXIT("malloc failed\n");
		pack_reloc(rel, n_rel, pack_buf, besti, bestj);
		pack_check(rel, n_rel, pack_buf, best_score);
	}
	{
		FILE *f = fopen(argv[2], "wb");
		if (f) {
			fwrite(pack_buf, 1, best_score, f);
			fwrite("\0\0", 1, -best_score & 3, f);
			fclose(f);
		}
	}
	free(pack_buf);
	free(rel);
	free(map);
}
