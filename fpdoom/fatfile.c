#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "microfat.h"
#include "fatfile.h"

fatdata_t fatdata_glob;

static unsigned fatfile_seek_chain(fatdata_t *fatdata, fatfile_t *f, unsigned pos) {
	uint32_t *chain, k, next, clust;
	k = f->chain_last; next = f->start;
	chain = f->chain;
	if ((int32_t)k < 0) goto simple;
	if (pos <= k)
		return chain ? chain[pos] : next + pos;
	clust = next + k;
	if (chain) clust = chain[k];
	do {
		next = fat_next_clust(fatdata, clust);
		if (next - 2 >= fatdata->cnum) goto err;
		if (!chain) {
			unsigned rnd, i, csh;
			if (next == ++clust) { k++; continue; }
			rnd = 512 << (csh = fatdata->csh);
			i = ((f->size + rnd - 1) >> 9) >> csh;
			chain = malloc(i * sizeof(*chain));
			if (!chain) goto err;
			f->chain = chain;
			i = k + 1; do chain[--i] = --clust; while (i);
		}
		chain[++k] = clust = next;
	} while (k < pos);
end:
	f->chain_last = k;
	return clust;
err:
	clust = FAT_CLUST_ERR;
	goto end;

simple:
	k = ~k; clust = (uintptr_t)chain;
	// k = 0 for empty files
	if (++pos < k) k = 1, clust = next;
	if (k == pos) return clust;
	do {
#if FAT_WRITE
		next = fat_alloc_clust(fatdata, clust, &f->start);
#else
		next = fat_next_clust(fatdata, clust);
#endif
		if (next - 2 >= fatdata->cnum) break;
		clust = next;
	} while (++k < pos);
	f->chain_last = ~k;
	f->chain = (uint32_t*)(uintptr_t)clust;
	return next;
}

size_t fat_fread(void *dst, size_t size, size_t count, fatfile_t *f) {
	uint8_t *d = (uint8_t*)dst;
	size_t pos, tmp;
	uint32_t snum, next;
	fatdata_t *fatdata;
#if FAT_WRITE
	if (!(f->flags & 1)) return 0;
#endif
	if (size != 1) {
		uint64_t size2 = (uint64_t)size * count;
		count = size2;
		if (sizeof(size) == 4 && size2 >> 32) count = ~0;
	}
	pos = f->pos; tmp = f->size;
	// if (tmp <= pos) return 0;
	tmp -= pos;
	if (count > tmp) count = tmp;

	fatdata = &fatdata_glob;
	next = pos & 511; pos >>= 9;
	snum = pos & (fatdata->csize - 1);
	pos >>= fatdata->csh;
	if (count) for (;;) {
		uint32_t spos = fatfile_seek_chain(fatdata, f, pos++);
		if ((spos -= 2) >= fatdata->cnum) break;
		spos = spos << fatdata->csh | snum;
		snum = fatdata->csize - snum;
		spos += fatdata->data_seg;
		do {
			uint8_t *buf = fat_read_sec(fatdata, fatdata->buf, spos++);
			if (!buf) goto end;
			buf += next; next = 512 - next;
			if (next > count) next = count;
			memcpy(d, buf, next); d += next;
			if (!(count -= next)) goto end;
			next = 0;
		} while (--snum);
		snum = 0;
	}
end:
	tmp = d - (uint8_t*)dst;
	f->pos += tmp;
	if (size > 1) tmp /= size;
	return tmp;
}

int fat_fgetc(fatfile_t *f) {
	uint8_t c;
	if (fat_fread(&c, 1, 1, f) != 1) return EOF;
	return c;
}

size_t fat_fwrite(const void *src, size_t size, size_t count, fatfile_t *f) {
#if !FAT_WRITE
	return 0;
#else
	const uint8_t *s = (const uint8_t*)src;
	size_t pos, tmp;
	uint32_t snum, next;
	fatdata_t *fatdata;
	if (!(f->flags & 2)) return 0;
	if (size != 1) {
		uint64_t size2 = (uint64_t)size * count;
		count = size2;
		if (sizeof(size) == 4 && size2 >> 32) count = ~0;
	}
	pos = f->pos; tmp = 0xffffffff;
	// if (tmp <= pos) return 0;
	tmp -= pos;
	if (count > tmp) count = tmp;
	// check if there is data after the region being written
	tmp = f->size - pos > count ? 512 : 0;

	fatdata = &fatdata_glob;
	next = pos & 511; pos >>= 9;
	snum = pos & (fatdata->csize - 1);
	pos >>= fatdata->csh;
	if (count) for (;;) {
		uint32_t spos = fatfile_seek_chain(fatdata, f, pos++);
		if ((spos -= 2) >= fatdata->cnum) break;
		spos = spos << fatdata->csh | snum;
		snum = fatdata->csize - snum;
		spos += fatdata->data_seg;
		do {
			uint8_t *buf = fatdata->buf;
			if (fatdata->buf_pos != spos) {
				if (next || count < tmp) {
					buf = fat_read_sec(fatdata, buf, spos);
					if (!buf) goto end;
				} else {
					fat_flush_buf1(fatdata);
					fatdata->buf_pos = spos;
					if (512 > count) memset(buf + count, 0, 512 - count);
				}
			}
			buf += next; next = 512 - next;
			if (next > count) next = count;
			memcpy(buf, s, next); s += next;
			fatdata->flags |= FAT_FLUSH_BUF1;
			if (!(count -= next)) goto end;
			next = 0; spos++;
		} while (--snum);
		snum = 0;
	}
end:
	tmp = s - (const uint8_t*)src;
	f->pos = pos = f->pos + tmp;
	if (f->size < pos) f->size = pos;
	if (size > 1) tmp /= size;
	return tmp;
#endif
}

int fat_fputc(int ch, fatfile_t *f) {
	uint8_t c = ch;
	if (fat_fwrite(&c, 1, 1, f) != 1) return EOF;
	return ch;
}

fatfile_t *fat_fopen(const char *name, const char *mode) {
	fatdata_t *fatdata;
	fatfile_t *f; fat_entry_t *p;
	int m0, tmp, flags = 1;

	m0 = *mode++;
	if (m0 != 'r') {
		if (m0 == 'w') flags = 2;
		else return NULL;
	}

	tmp = *mode++;
	if (tmp == 'b') tmp = *mode++;
	if (tmp == '+') flags = 3;

#if !FAT_WRITE
	if (flags & 2) return NULL;
#endif

	fatdata = &fatdata_glob;
	p = fat_find_path(fatdata, name);
	if (!p) {
#if !FAT_WRITE
		return NULL;
#else
		if (m0 == 'r' || !(name = fatdata->lastname)) return NULL;
		p = fat_create_name(fatdata, fatdata->lastdir, name);
		if (!p) return NULL;
		p->entry.attr = FAT_ATTR_ARC;
		flags |= 4; // new file
#endif
	}
	if (p->entry.attr & FAT_ATTR_DIR) return NULL;

	f = malloc(sizeof(*f));
	if (!f) return f;
	f->start = fat_entry_clust(p);
#if !FAT_WRITE
	f->chain_last = 0;
	f->chain = NULL;
	f->size = p->entry.size;
#else
	f->flags = flags;
	f->entry_seg = fatdata->buf_pos;
	f->entry_pos = ((uint8_t*)p - fatdata->buf) >> 5;	
	f->size = m0 == 'r' ? p->entry.size : 0;
	if (flags & 2) {
		f->chain_last = f->start ? ~1 : ~0;
		f->chain = (uint32_t*)(uintptr_t)f->start;
	} else {
		f->chain_last = 0;
		f->chain = NULL;
	}
#endif
	f->pos = 0;
	return f;
}

int fat_fclose(fatfile_t *f) {
	if (!f) return EOF;
#if FAT_WRITE
	if (f->flags & 2) {
		fatdata_t *fatdata = &fatdata_glob;
		uint8_t *buf = fat_read_sec(fatdata, fatdata->buf, f->entry_seg);
		if (buf) {
			fat_entry_t *p = (fat_entry_t*)buf;
			p += f->entry_pos;
			if ((f->flags & 4) || f->size != p->entry.size) {
				uint32_t pos = 0, size, start = f->start, end;
				p->entry.size = size = f->size;
				if (size) pos = start;
				p->entry.clust_hi = pos >> 16;
				p->entry.clust_lo = pos;
				fatdata->flags |= FAT_FLUSH_BUF1;
				end = 0;
				if (size) {
					pos = ((size - 1) >> 9) >> fatdata->csh;
					start = fatfile_seek_chain(fatdata, f, pos);
					end = 0xfffffff;
				}
				fat_free_chain(fatdata, start, end);
				fat_flush_buf1(fatdata);
			}
		}
	}
#endif
	if ((int32_t)f->chain_last >= 0 && f->chain)
		free(f->chain);
	free(f);
	return 0;
}

#if SEEK_SET != 0 || SEEK_CUR != 1 || SEEK_END != 2
#error
#endif

int fat_fseek(fatfile_t *f, long offset, int origin) {
	uint32_t size = f->size, pos = f->pos;
	unsigned long tmp = offset;
	if ((unsigned)origin > SEEK_END) return -1;
	if (origin == SEEK_END) pos = size;
	if (origin == SEEK_SET) pos = 0;
	size -= pos;
	if (offset < 0) size = pos, tmp = -offset;
	if (size < tmp) return -1;
	f->pos = pos + offset;
	return 0;
}

long fat_ftell(fatfile_t *f) {
	return f->pos;
}

int fat_fflush(fatfile_t *f) {
	(void)f;
#if FAT_WRITE
	fatdata_t *fatdata = &fatdata_glob;
	fat_flush_buf1(fatdata);
	fat_flush_buf2(fatdata);
#endif
	return 0;
}
