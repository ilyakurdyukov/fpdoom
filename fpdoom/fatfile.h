#ifndef FATFILE_H
#define FATFILE_H

#include <stddef.h>
#include "microfat.h"

typedef struct fatfile {
#if FAT_WRITE
	uint8_t flags, entry_pos;
	uint32_t entry_seg;
#endif
	uint32_t *chain, start, chain_last;
	uint32_t pos, size;
} fatfile_t;

extern fatdata_t fatdata_glob;

size_t fat_fread(void *dst, size_t size, size_t count, fatfile_t *f);
size_t fat_fwrite(const void *src, size_t size, size_t count, fatfile_t *f);
int fat_fgetc(fatfile_t *f);
int fat_fputc(int ch, fatfile_t *f);

fatfile_t *fat_fopen(const char *name, const char *mode);
int fat_fclose(fatfile_t *f);
int fat_fseek(fatfile_t *f, long offset, int origin);
long fat_ftell(fatfile_t *f);
int fat_fflush(fatfile_t *f);
#endif
