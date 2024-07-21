#ifndef MICROFAT_H
#define MICROFAT_H

#include <stdint.h>

#ifndef FAT_WRITE
#define FAT_WRITE 0
#endif

#ifndef FAT_LFN_MAX
#define FAT_LFN_MAX 20
#endif

#ifndef FAT_SHORT_FIND
#define FAT_SHORT_FIND 0
#endif

#ifndef FAT_DEBUG
#define FAT_DEBUG 0
#endif

typedef struct {
	uint8_t *buf, *buf2;
	uint32_t buf_pos, buf2_pos;
	uint32_t fat1, fat2, data_seg;
	uint32_t root, curdir, cnum;
#if FAT_WRITE
	const char *lastname; uint32_t lastdir;
#endif
	uint8_t csize, csh, flags;
#ifdef FATDATA_EXTRA
	FATDATA_EXTRA
#endif
} fatdata_t;

#define FAT_CLUST_ERR 0
#define FAT_FLUSH_BUF1 0x10
#define FAT_FLUSH_BUF2 0x20

typedef union {
	uint8_t raw[16];
	struct {
		uint8_t name[8], ext[3];
		uint8_t attr, attr2, ctime10ms;
		uint16_t ctime, cdate, adate, clust_hi;
		uint16_t mtime, mdate, clust_lo;
		uint32_t size;
	} entry;
	struct {
		uint8_t seq, name1[5 * 2];
		uint8_t attr, attr2, chk;
		uint8_t name2[6 * 2];
		uint16_t clust_lo;
		uint8_t name3[2 * 2];
	} lfn;
} fat_entry_t;

enum {
	FAT_ATTR_RO = 1, /* read only */
	FAT_ATTR_HID = 2, /* hidden */
	FAT_ATTR_SYS = 4, /* system */
	FAT_ATTR_VOL = 8, /* volume label */
	FAT_ATTR_DIR = 0x10, /* directory */
	FAT_ATTR_ARC = 0x20, /* archive */

	FAT_ATTR_LFN = 0xf /* long file name */
};

int fat_init(fatdata_t *fatdata, int part_num);
uint8_t* fat_read_sec(fatdata_t *x, uint8_t *buf, uint32_t sector);
#if !FAT_WRITE
unsigned fat_next_clust(fatdata_t *fatdata, unsigned clust);
#else
uint8_t* fat_write_sec(fatdata_t *x, uint8_t *buf, uint32_t sector);
void fat_flush_buf1(fatdata_t *fatdata);
void fat_flush_buf2(fatdata_t *fatdata);
unsigned fat_alloc_clust(fatdata_t *fatdata, unsigned clust, uint32_t *start);
void fat_free_chain(fatdata_t *fatdata, unsigned clust, unsigned end);
static inline 
unsigned fat_next_clust(fatdata_t *fatdata, unsigned clust) {
	return fat_alloc_clust(fatdata, clust, (void*)0);
}
#endif

int fat_enum_name(fatdata_t *fatdata, unsigned clust,
		int (*cb)(void*, fat_entry_t*, const char*), void *cbdata);
fat_entry_t* fat_find_name(fatdata_t *fatdata, unsigned clust,
		const char *name, int len);
fat_entry_t* fat_find_path(fatdata_t *fatdata, const char *name);
unsigned fat_dir_clust(fatdata_t *fatdata, const char *name);
unsigned fat_read_simple(fatdata_t *fatdata, unsigned clust,
	void *buf, uint32_t size);
#if FAT_WRITE
fat_entry_t* fat_create_name(fatdata_t *fatdata, unsigned clust,
		const char *name);
unsigned fat_make_dir(fatdata_t *fatdata, unsigned clust,
		const char *name);
void fat_delete_entry(fatdata_t *fatdata, fat_entry_t *p);
int fat_rmdir_check(fatdata_t *fatdata, unsigned clust);
#endif

static inline
unsigned fat_entry_clust(fat_entry_t *p) {
	return p->entry.clust_hi << 16 | p->entry.clust_lo;
}

#if FAT_WRITE && defined(FAT_WRITE_SYS)
uint8_t* fat_write_sec(fatdata_t *fatdata, uint8_t *buf, uint32_t sector) {
	(void)fatdata;
	do {
		FAT_WRITE_SYS
		return buf;
	} while (0);
	return NULL;
}
#endif

#ifdef FAT_READ_SYS
uint8_t* fat_read_sec(fatdata_t *fatdata, uint8_t *buf, uint32_t sector) {
	if (buf == fatdata->buf) {
		if (fatdata->buf_pos == sector) return buf;
#if FAT_WRITE
		if (fatdata->flags & FAT_FLUSH_BUF1) {
			fat_flush_buf1(fatdata);
			buf = fatdata->buf;
		}
#endif
	}
	do {
		FAT_READ_SYS
		if (buf == fatdata->buf) fatdata->buf_pos = sector;
		return buf;
	} while (0);
	if (buf == fatdata->buf) fatdata->buf_pos = ~0; // discard cache
	return NULL;
}
#endif

#endif
