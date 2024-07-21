
#include <string.h>
#include "microfat.h"

#if FAT_DEBUG
#include <stdio.h>
#define FAT_LOG(...) fprintf(stderr, __VA_ARGS__)
static inline
int fat_entry_low(fatdata_t *fatdata, fat_entry_t *p) {
	return (fatdata->buf_pos & 7) << 9 |
			((uint8_t*)p - fatdata->buf);
}
#define ENTRY_FMT "0x%x%03x"
#define ENTRY_POS(p) fatdata->buf_pos >> 3, fat_entry_low(fatdata, p)
#else
#define FAT_LOG(...)
#endif

int fat_init(fatdata_t *fatdata, int part_num) {
	uint8_t *buf, *p;
	uint32_t pos;
	static const char fat32_type[8] = { 'F','A','T','3','2',' ',' ',' ' };
	int ret = -1;
	uint32_t csize, csh, fat_num, res_sec;
	uint32_t fat_sec, tmp, tmp2, part_size;

	do {
		fatdata->buf_pos = ~0;
		fatdata->buf2_pos = ~0;
		buf = fatdata->buf;
		fatdata->buf2 = buf + 512;
		if (!(buf = fat_read_sec(fatdata, buf, 0))) break;
		if (*(uint16_t*)(buf + 0x1fe) != 0xaa55) break;
		// need to keep alignment
		p = buf + 0x1be + (part_num << 4);
		pos = *(uint16_t*)(p + 8) | *(uint16_t*)(p + 8 + 2) << 16;
		if (!pos) return 1;
		part_size = *(uint16_t*)(p + 12) | *(uint16_t*)(p + 12 + 2) << 16;
		FAT_LOG("mbr: partition start = 0x%x\n", pos);
		FAT_LOG("mbr: partition size = %u\n", part_size);
		// special case for pos = 0 and size = 0
		if (pos > 0 - part_size) break;
		if (!(buf = fat_read_sec(fatdata, fatdata->buf, pos))) break;
		if (memcmp(buf + 0x52, fat32_type, 8)) break;

		// need to keep alignment
		tmp = *(uint8_t*)(buf + 0xb) | *(uint8_t*)(buf + 0xc) << 8;
		if (tmp != 512) break;
		csize = *(uint8_t*)(buf + 0xd);
		FAT_LOG("fat: cluster size = %u\n", csize);
		if (!csize || (csize & (csize - 1))) break;
		fatdata->csize = csize;
		csh = ~0; do csh++; while (csize >>= 1);
		fatdata->csh = csh;

		// sectors to first FAT
		res_sec = *(uint16_t*)(buf + 0x0e);
		fat_num = *(uint8_t*)(buf + 0x10);
		FAT_LOG("fat: reserved sectors = %u\n", res_sec);
		FAT_LOG("fat: number of FATs = %u\n", fat_num);
		if (!res_sec || (fat_num - 1) >> 1) break;

		// 0x1c: should contain position
		part_size = *(uint32_t*)(buf + 0x20);
		FAT_LOG("fat: partition size = %u\n", part_size);
		if (pos > 0 - part_size) break;
		fat_sec = *(uint32_t*)(buf + 0x24);
		FAT_LOG("fat: sectors per FAT = %u\n", fat_sec);
		if ((fat_sec - 1) >> 21) break;
		tmp = res_sec + (fat_sec << (fat_num - 1));
		fatdata->data_seg = pos + tmp;
		pos += res_sec;
		tmp2 = part_size - tmp; // data size
		if (part_size < tmp) break;
		tmp2 >>= csh;
		tmp = (fat_sec << 7) - 2;
		if (tmp2 > tmp) tmp2 = tmp;
		fatdata->cnum = tmp2;
		fatdata->fat1 = pos;
		fatdata->fat2 = fat_num > 1 ? pos + fat_sec : 0;

		tmp = *(uint32_t*)(buf + 0x2c);
		FAT_LOG("fat: root cluster = %u\n", tmp);
		if ((tmp - 2) > tmp2) break;
		fatdata->root = tmp;
		fatdata->curdir = tmp;
		fatdata->flags = 0;
		ret = 0;
	} while (0);
	return ret;
}

#if !FAT_WRITE
unsigned fat_next_clust(fatdata_t *fatdata, unsigned clust) {
	uint8_t *buf;
	uint32_t pos = clust >> 7;
	buf = fatdata->buf2;
	if (fatdata->buf2_pos != pos) {
		buf = fat_read_sec(fatdata, buf, pos + fatdata->fat1);
		if (!buf) return FAT_CLUST_ERR;
		fatdata->buf2_pos = pos;
	}
	clust = ((uint32_t*)buf)[clust & 127];
	return clust;
}
#else
void fat_flush_buf1(fatdata_t *fatdata) {
	if (fatdata->flags & FAT_FLUSH_BUF1) {
		fatdata->flags &= ~FAT_FLUSH_BUF1;
		fat_write_sec(fatdata, fatdata->buf, fatdata->buf_pos);
	}
}

void fat_flush_buf2(fatdata_t *fatdata) {
	uint32_t fat2;
	if (fatdata->flags & FAT_FLUSH_BUF2) {
		fatdata->flags &= ~FAT_FLUSH_BUF2;
		fat_write_sec(fatdata, fatdata->buf2, fatdata->buf2_pos + fatdata->fat1);
		if ((fat2 = fatdata->fat2))
			fat_write_sec(fatdata, fatdata->buf2, fatdata->buf2_pos + fat2);
	}
}

uint8_t* fat_read_chain(fatdata_t *fatdata, unsigned clust) {
	uint8_t *buf;
	uint32_t pos = clust >> 7;
	buf = fatdata->buf2;
	if (fatdata->buf2_pos != pos) {
		if (fatdata->flags & FAT_FLUSH_BUF2) {
			fat_flush_buf2(fatdata);
			buf = fatdata->buf2;
		}
		buf = fat_read_sec(fatdata, buf, pos + fatdata->fat1);
		if (!buf) return buf;
		fatdata->buf2_pos = pos;
	}
	return buf;
}

unsigned fat_alloc_clust(fatdata_t *fatdata, unsigned clust, uint32_t *start) {
	uint8_t *buf;
	unsigned next = clust, pos;
	if (clust) {
		buf = fat_read_chain(fatdata, clust);
		if (!buf) return FAT_CLUST_ERR;
		next = ((uint32_t*)buf)[clust & 127];
		if (next < 0xffffff8) {
			FAT_LOG("fat: clust next 0x%x\n", next);
			return next;
		}
	}
	if (!start) return next;
	FAT_LOG("fat: alloc prev 0x%x\n", clust);
	next = clust ? clust + 1 : 2;
	for (;;) {
		unsigned n = fatdata->cnum + 2, end;
		if (next == n) next = 2;
		if (next == clust) break;
		if (next < clust) n = clust;
		end = (next + 128) & ~127;
		if (end > n) end = n;
		buf = fat_read_chain(fatdata, next);
		if (!buf) return FAT_CLUST_ERR;
		for (; next < end; next++)
			if (!((uint32_t*)buf)[next & 127]) goto found;
	}
	FAT_LOG("fat: alloc failed\n");
	return FAT_CLUST_ERR;

found:
	FAT_LOG("fat: alloc next 0x%x\n", next);
	((uint32_t*)buf)[next & 127] = 0xfffffff;
	fatdata->flags |= FAT_FLUSH_BUF2;
	if (!clust) *start = next;
	else {
		buf = fat_read_chain(fatdata, clust);
		if (!buf) return FAT_CLUST_ERR;
		((uint32_t*)buf)[clust & 127] = next;
		fatdata->flags |= FAT_FLUSH_BUF2;
	}
	return next;
}

void fat_free_chain(fatdata_t *fatdata, unsigned clust, unsigned val) {
	uint8_t *buf; unsigned pos;
	for (;; val = 0) {
		if (clust - 2 >= fatdata->cnum) break;
		FAT_LOG("fat: %s 0x%x\n", val ? "end" : "free", clust);
		buf = fat_read_chain(fatdata, clust);
		if (!buf) break;
		pos = clust & 127;
		clust = ((uint32_t*)buf)[pos];
		if (clust == val) break;
		((uint32_t*)buf)[pos] = val;
		fatdata->flags |= FAT_FLUSH_BUF2;
	}
	fat_flush_buf2(fatdata);
}
#endif

static int fat_enum_entry(fatdata_t *fatdata, unsigned clust,
		int (*cb)(void*, fat_entry_t*), void *cbdata) {
	uint8_t *buf;
	uint32_t spos, send, idx;
	for (;;) {
		if ((spos = clust - 2) >= fatdata->cnum) break;
		spos = (spos << fatdata->csh) + fatdata->data_seg;
		send = spos + fatdata->csize;
		do {
			for (idx = 0; idx < 16; idx++) {
				int a, ret; fat_entry_t *p;
				buf = fatdata->buf;
				if (fatdata->buf_pos != spos) {
					buf = fat_read_sec(fatdata, buf, spos);
					if (!buf) return -1;
				}
				p = (fat_entry_t*)buf + idx;
				a = p->raw[0];
				if (!a) return 0; // end
				if (a == 0xe5) continue; // deleted
				ret = cb(cbdata, p);
				if (ret) return ret;
			}
		} while (++spos < send);
		clust = fat_next_clust(fatdata, clust);
	}
	return -1;
}

typedef struct {
	int (*cb)(void*, fat_entry_t*, const char *name);
	void *cbdata;
	uint8_t seq, chk;
	char name[FAT_LFN_MAX * 13 + 1];
} fat_enum_entry_t;

static const char fat_lfn_idx[13] = { 1, 3, 5, 7, 9,
	14, 16, 18, 20, 22, 24, 28, 30 };

static int fat_sfn_checksum(fat_entry_t *p) {
	unsigned i, a;
	for (i = a = 0; i < 11; i++)
		a = ((a << 7 | a >> 1) + p->raw[i]) & 0xff;
	return a;
}

static int fat_enum_name_cb(void *cbdata, fat_entry_t *p) {
	fat_enum_entry_t *x = cbdata;
	unsigned a, i, j, k;
	a = p->entry.attr;
	if (a == FAT_ATTR_LFN && !p->entry.attr2) {
		unsigned seq = x->seq;
		a = p->lfn.seq;
		if (a != seq) {
			if ((unsigned)(a - 0x41) >= FAT_LFN_MAX) goto err;
			seq = a - 0x40;
			x->name[seq * 13] = 0;
			x->chk = p->lfn.chk;
		} else if (x->chk != p->lfn.chk) goto err;
		x->seq = --seq;
		i = seq * 13;
		for (j = 0; j < 13; j++) {
			k = fat_lfn_idx[j];
			a = p->raw[k + 1] ? '?' : p->raw[k];
			x->name[i++] = a;
			if (!a) break;
		}
		//if (!seq) FAT_LOG("lfn: %s\n", x->name);
		return 0;
	}
	if (a & FAT_ATTR_VOL) goto err;
	a = ~0;
	if (!x->seq) {
		x->seq = a;
		a = fat_sfn_checksum(p);
	}
	if (a != x->chk) {
		for (i = j = 0; i < 8;) {
			a = p->entry.name[i];
			if (!a) a = ' ';
			x->name[i++] = a;
			if (a != ' ') j = i;
		}
		if (!j) return 0;
		x->name[j] = '.';
		for (k = j + 1, i = 0; i < 3;) {
			a = p->entry.ext[i++];
			if (!a) a = ' ';
			x->name[k++] = a;
			if (a != ' ') j = k;
		}
		x->name[j] = 0;
	}
	return x->cb(x->cbdata, p, x->name);

err:
	x->seq = ~0;
	return 0;
}

int fat_enum_name(fatdata_t *fatdata, unsigned clust,
		int (*cb)(void*, fat_entry_t*, const char*), void *cbdata) {
	fat_enum_entry_t cbdata1;
	cbdata1.cb = cb;
	cbdata1.cbdata = cbdata;
	cbdata1.seq = ~0;
	return fat_enum_entry(fatdata, clust, &fat_enum_name_cb, &cbdata1);
}

typedef struct {
	union {
		fat_entry_t *result;
		const char *name;
	} u;
	unsigned len;
#if !FAT_SHORT_FIND
	uint8_t seq, chk;
#endif
} fat_find_name_t;

#if FAT_SHORT_FIND
static int fat_find_name_cb(void *cbdata, fat_entry_t *p, const char *name) {
	fat_find_name_t *x = cbdata;
	unsigned len = strlen(name), a, i, k;
	const char *target = x->u.name;
	if (len != x->len) return 0;
	for (i = 0; i < len; i++) {
		a = name[i];
		k = a ^ target[i];
		if ((unsigned)(a & ~32) - 'A' < 26) k &= ~32;
		if (k) return 0;
	}
	x->u.result = p;
	return 1;
}
#else // fast, without a name buffer, less code if fat_enum_name() is not used
static int fat_find_name_cb(void *cbdata, fat_entry_t *p) {
	fat_find_name_t *x = cbdata;
	unsigned a, i, j, k;
	const char *name;
	a = p->entry.attr;
	if (a == FAT_ATTR_LFN && !p->entry.attr2) {
		unsigned seq = x->seq, len;
		a = p->lfn.seq;
		k = p->lfn.chk;
		if (a != seq) {
			if ((unsigned)(a - 0x41) >= 20) goto err;
			seq = a - 0x40;
			x->chk = k;
		} else if (x->chk != k) goto err;
		x->seq = --seq;
		i = seq * 13;
		len = x->len;
		name = x->u.name;
		for (j = 0; j < 13; j++, i++) {
			k = fat_lfn_idx[j];
			a = p->raw[k + 1] ? '?' : p->raw[k];
			k = a ^ (i < len ? name[i] : 0);
			if ((unsigned)(a & ~32) - 'A' < 26) k &= ~32;
			if (k) goto err;
			if (!a) break;
		}
		return 0;
	}
	if (a & FAT_ATTR_VOL) goto err;
	a = ~0;
	if (!x->seq) {
		x->seq = a;
		a = fat_sfn_checksum(p);
	}
	if (a != x->chk) {
		char buf[12];
		for (i = j = 0; i < 8;) {
			a = p->entry.name[i];
			if (!a) a = ' ';
			buf[i++] = a;
			if (a != ' ') j = i;
		}
		if (!j) return 0;
		buf[j] = '.';
		for (k = j + 1, i = 0; i < 3;) {
			a = p->entry.ext[i++];
			if (!a) a = ' ';
			buf[k++] = a;
			if (a != ' ') j = k;
		}
		if (j != x->len) return 0;
		name = x->u.name;
		for (i = 0; i < j; i++) {
			a = buf[i];
			k = a ^ name[i];
			if ((unsigned)(a & ~32) - 'A' < 26) k &= ~32;
			if (k) return 0;
		}
	}
	x->u.result = p;
	return 1;

err:
	x->seq = ~0;
	return 0;
}
#endif

fat_entry_t* fat_find_name(fatdata_t *fatdata, unsigned clust,
		const char *name, int len) {
	fat_find_name_t cbdata;
	cbdata.u.name = name;
	cbdata.len = len < 0 ? (unsigned)strlen(name) : (unsigned)len;
#if FAT_SHORT_FIND
	if (fat_enum_name(fatdata, clust, &fat_find_name_cb, &cbdata) == 1) {
#else
	cbdata.seq = ~0;
	if (fat_enum_entry(fatdata, clust, &fat_find_name_cb, &cbdata) == 1) {
#endif
		FAT_LOG("fat: entry " ENTRY_FMT "\n", ENTRY_POS(cbdata.u.result));
		return cbdata.u.result;
	}
	return NULL;
}

fat_entry_t* fat_find_path(fatdata_t *fatdata, const char *name) {
	unsigned clust;
	fat_find_name_t cbdata;
	fat_entry_t *p = NULL;
#if FAT_WRITE
	fatdata->lastname = NULL;
#endif
	clust = fatdata->curdir;
	if (*name == '/') name++, clust = fatdata->root;
	for (;; name++) {
		unsigned i, a;
		for (i = 0; (a = name[i]); i++)
			if (a == '/' || a == '\\') break;
		if (!i) {
			if (!a) return p;
			continue;
		}
#if FAT_WRITE
		if (!a) fatdata->lastname = name;
		fatdata->lastdir = clust;
#endif
		{
			const char *name1 = name;
			name += i;
			p = fat_find_name(fatdata, clust, name1, i);
		}
		if (!p || !*name) return p;
		if (!(p->entry.attr & FAT_ATTR_DIR)) break;
		clust = fat_entry_clust(p);
		/* found in ".." entries */
		if (!clust) clust = fatdata->root;
	}
	return NULL;
}

unsigned fat_dir_clust(fatdata_t *fatdata, const char *name) {
	fat_entry_t *p;
	unsigned a = *name, clust;
	if (!a) return 0;
	if (!name[1]) {
		if (a == '/') return fatdata->root;
		if (a == '.') return fatdata->curdir;
	}
	p = fat_find_path(fatdata, name);
	clust = 0;
	if (p && (p->entry.attr & FAT_ATTR_DIR)) {
		clust = fat_entry_clust(p);
		if (!clust) clust = fatdata->root;
	}
	return clust;
}

unsigned fat_read_simple(fatdata_t *fatdata, unsigned clust,
	void *buf, uint32_t size) {
	uint8_t *dst = buf;
	uint32_t spos, send, pos = 0;
	for (;;) {
		if ((spos = clust - 2) >= fatdata->cnum) break;
		spos = (spos << fatdata->csh) + fatdata->data_seg;
		send = spos + fatdata->csize;
		do {
			if (pos >= size || !fat_read_sec(fatdata, dst + pos, spos))
				return pos;
			pos += 512;
		} while (++spos < send);
		if (pos >= size) break;
		clust = fat_next_clust(fatdata, clust);
	}
	return pos;
}

#if FAT_WRITE
static int make_dos_name(const char *name, uint8_t *shortname) {
	static const uint8_t allowed[] = {
		0,0,0,0, 0xfb,0x23,0xff,0x03,
		0xff,0xff,0xff,0xc7, 0xff,0xff,0xff,0x6f };
	unsigned a = ' ', prev, i = 0, n = 8;
	memset(shortname, ' ', 11);
	for (;;) {
		prev = a; a = *name++;
		if (!a) break;
		if (a == '.') {
			if (prev == ' ' || n > 8 || *name <= 32) return 0;
			i = 8; n = 11; continue;
		}
		if (i >= n || a >= 127) return 0;
		if (!(allowed[a >> 3] >> (a & 7) & 1)) return 0;
		if (a - 'a' < 26) a -= 32;
		shortname[i++] = a;
	}
	if (shortname[0] == ' ' || prev == ' ') return 0;
	return i;
}

static fat_entry_t* fat_dir_expand(fatdata_t *fatdata, unsigned clust) {
	uint32_t spos; unsigned i, j;
	if ((spos = clust - 2) >= fatdata->cnum) return NULL;
	spos = (spos << fatdata->csh) + fatdata->data_seg;
	/* clean cluster */
	for (j = 1; j < fatdata->csize; j++) {
		uint8_t *buf = fat_read_sec(fatdata, fatdata->buf, spos + j);
		if (!buf) return NULL;
		for (i = 0; i < 512; i += 32) {
			if (!buf[i]) continue;
			memset(buf, 0, 512);
			fatdata->flags |= FAT_FLUSH_BUF1;
			break;
		}
	}
	fat_flush_buf1(fatdata);
	memset(fatdata->buf, 0, 512);
	fatdata->buf_pos = spos;
	FAT_LOG("fat: dir expand " ENTRY_FMT "\n",
			ENTRY_POS((fat_entry_t*)fatdata->buf));
	return (fat_entry_t*)fatdata->buf;
}

fat_entry_t* fat_create_name(fatdata_t *fatdata, unsigned clust,
		const char *name) {
	uint8_t *buf; fat_entry_t *p;
	uint32_t spos, send, idx, start = 0;
	unsigned next;
	uint8_t shortname[11];
	if (!make_dos_name(name, shortname)) return NULL;
	FAT_LOG("fat: short name = \"%.11s\"\n", shortname);
	for (;;) {
		if ((spos = clust - 2) >= fatdata->cnum) break;
		spos = (spos << fatdata->csh) + fatdata->data_seg;
		send = spos + fatdata->csize;
		do {
			buf = fat_read_sec(fatdata, fatdata->buf, spos);
			for (idx = 0; idx < 16; idx++) {
				unsigned a;
				p = (fat_entry_t*)buf + idx;
				a = p->raw[0];
				if (!a || a == 0xe5) goto found;
			}
		} while (++spos < send);
		next = fat_alloc_clust(fatdata, clust, &start);
		if (start) goto create;
		clust = next;
	}
	return NULL;

create:
	p = fat_dir_expand(fatdata, next);
	if (!p) return p;
found:
	FAT_LOG("fat: new entry " ENTRY_FMT "\n", ENTRY_POS(p));
	memset(p->raw, 0, 32);
	memcpy(p->raw, shortname, 11);
	fatdata->flags |= FAT_FLUSH_BUF1;
	return p;
}

unsigned fat_make_dir(fatdata_t *fatdata, unsigned prev, const char *name) {
	fat_entry_t *p; uint32_t start;
	unsigned clust, i;
	p = fat_create_name(fatdata, prev, name);
	if (!p) return FAT_CLUST_ERR;
	p->entry.attr = FAT_ATTR_DIR;
	clust = fat_alloc_clust(fatdata, 0, &start);
	if (clust - 2 >= fatdata->cnum) goto end;
	p->entry.clust_hi = clust >> 16;
	p->entry.clust_lo = clust;
	p = fat_dir_expand(fatdata, clust);
	if (!p) {
		clust = FAT_CLUST_ERR;
		goto end;
	}
	memset(p->raw, ' ', 11);
	p->raw[0] = '.';
	p->entry.attr = FAT_ATTR_DIR;
	p->entry.clust_hi = clust >> 16;
	p->entry.clust_lo = clust;
	p++;
	memset(p->raw, ' ', 11);
	p->raw[0] = '.';
	p->raw[1] = '.';
	p->entry.attr = FAT_ATTR_DIR;
	if (prev == fatdata->root) prev = 0;
	p->entry.clust_hi = prev >> 16;
	p->entry.clust_lo = prev;
	fatdata->flags |= FAT_FLUSH_BUF1;
end:
	fat_flush_buf1(fatdata);
	fat_flush_buf2(fatdata);
	return clust;
}

void fat_delete_entry(fatdata_t *fatdata, fat_entry_t *p) {
	unsigned clust = fat_entry_clust(p);
	uint32_t prev[2], chk, seq, n;
	if (clust) fat_free_chain(fatdata, clust, 0);
	FAT_LOG("fat: delete entry " ENTRY_FMT "\n", ENTRY_POS(p));

	chk = fat_sfn_checksum(p);
	seq = 0; n = ~0;
	for (;;) {
		unsigned a;
		/* mark entry or LFN as deleted */
		p->raw[0] = 0xe5;
		fatdata->flags |= FAT_FLUSH_BUF1;
		if (seq >= 20) break;
		if ((uint8_t*)p == fatdata->buf) {
			/* LFN crosses sector boundary */
			uint32_t next = fatdata->buf_pos;
			a = next - fatdata->data_seg;
			if (a & (fatdata->csize - 1)) next--;
			else {
				/* LFN crosses cluster boundary */
				if (n == ~0u) {
					next = fatdata->lastdir; n = 0;
					for (;;) {
						if ((next -= 2) >= fatdata->cnum) break;
						next = (next << fatdata->csh) + fatdata->data_seg;
						if (fatdata->buf_pos == next) goto found;
						prev[n++ & 1] = next;
						next = fat_next_clust(fatdata, next);
					}
					FAT_LOG("fat: LFN delete error\n");
					goto end;
				}
found:
				if (!n) break;
				next = prev[--n & 1];
			}
			p = (fat_entry_t*)fat_read_sec(fatdata, (uint8_t*)p, next);
			if (!p) break;
			p += 512 / 32;
		}
		p--;
		if (p->entry.attr != FAT_ATTR_LFN || p->entry.attr2) break;
		if (p->lfn.chk != chk) break;
		a = seq + 1; seq = p->lfn.seq;
		if ((a ^ seq) & ~0x40) break;
	}
	FAT_LOG("fat: last LFN seq: 0x%02x\n", seq);
end:
	fat_flush_buf1(fatdata);
}

static int fat_rmdir_check_cb(void *cbdata, fat_entry_t *p) {
	unsigned i;
	if (p->entry.attr == FAT_ATTR_LFN && !p->entry.attr2)
		return 0;
	if (p->raw[0] == '.') {
		i = 1 + (p->raw[1] == '.');
		do if (p->raw[i] != ' ') return 1;
		while (++i < 11);
		return 0;
	}
	return 1;
}

int fat_rmdir_check(fatdata_t *fatdata, unsigned clust) {
	return fat_enum_entry(fatdata, clust, &fat_rmdir_check_cb, NULL);
}
#endif

