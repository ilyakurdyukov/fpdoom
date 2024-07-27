#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "syscode.h"
#include "sdio.h"
#include "cmd_def.h"
#include "usbio.h"

#if LIBC_SDIO >= 3
#define printf(...) (void)0
#else
int _argv_init(char ***argvp, int skip);
static void libc_resetio(void) {
	uint16_t buf[2];
	unsigned tmp = CMD_RESETIO | 1 << 8;
	buf[0] = tmp; tmp += CHECKSUM_INIT;
	buf[1] = tmp + (tmp >> 16);
	usb_write(&buf, sizeof(buf));
}
#define FAT_READ_SYS \
	if (sdio_read_block(sector, buf)) break;
#endif
#include "microfat.h"

int sdmain(uint8_t *ram) {
	int argc; char **argv;
	fatdata_t fatdata;
	fat_entry_t *p;
	const char *bin_name = "fpbin/fpmain.bin";
#if LIBC_SDIO < 3
	argc = _argv_init(&argv, 0);
	if (argc > 1 && !strcmp(argv[0], "--bin")) {
		bin_name = argv[1];
		argc -= 2; argv += 2;
	}
#endif
	sdio_init();
	if (sdcard_init()) return 1;
	SDIO_VERBOSITY(0);
	{
		uint8_t *mem = (uint8_t*)CHIPRAM_ADDR + 0x9000;
		fatdata.buf = mem;
	}
	if (fat_init(&fatdata, 0)) {
		printf("fat_init failed\n");
		return 1;
	}
	p = fat_find_path(&fatdata, bin_name);
	if (!p || (p->entry.attr & FAT_ATTR_DIR)) {
		printf("file not found\n");
		return 1;
	}
	{
		unsigned clust = fat_entry_clust(p);
		uint32_t size = p->entry.size, n;
		printf("start = 0x%x, size = 0x%x\n", clust, size);
		if (size > 3 << 20) {
			printf("binary is too big\n");
			return 1;
		}
		n = fat_read_simple(&fatdata, clust, ram, size);
		printf("read = 0x%x\n", n);
		if (n < size) {
			printf("fat_read failed\n");
			return 1;
		}
	}
	{
		char *p = (char*)CHIPRAM_ADDR;
#if LIBC_SDIO < 3
		MEM4(CHIPRAM_ADDR) = sdio_shl;
#endif
		p += 4;
#if LIBC_SDIO < 3
		libc_resetio();
		if (argc) {
			size_t size = argv[argc - 1] - argv[0];
			size += strlen(argv[argc - 1]) + 1;
			if (size >> 12) {
				printf("args is too big (%u)\n", size);
				return 1;
			}
			memcpy(p + sizeof(short), argv[0], size);
		}
#else
		argc = 0;
#endif
		*(short*)p = argc;
		clean_dcache();
		((void(*)(void))ram)();
	}
	return 0;
}

