#include "common.h"
#include "cmd_def.h"
#include "usbio.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if LIBC_SDIO
#include "sdio.h"
#include "microfat.h"
#include "fatfile.h"
#endif

void _debug_msg(const char *msg);
void _malloc_init(void *addr, size_t size);
void _stdio_init(void);
int _argv_init(char ***argvp, int skip);
int _argv_copy(char ***argvp, int argc, char *src);

size_t app_mem_reserve(void *start, size_t size);
void sys_set_handlers(void);
int main(int argc, char **argv);

#ifdef CXX_SUPPORT
#include "cxx_init.h"
#endif

void entry_main2(char *image_addr, uint32_t image_size, uint32_t bss_size, int arg_skip) {
	int argc; char **argv = NULL;
	uint32_t ram_addr = (uint32_t)image_addr & 0xf0000000;
	uint32_t ram_size = MEM4(image_addr);

	memset(image_addr + image_size, 0, bss_size);

#if LIBC_SDIO < 3
	usb_init();
	_debug_msg("entry2");
#endif
	{
		char *addr = image_addr + image_size + bss_size;
		size_t size = ram_addr + ram_size - (uint32_t)addr;
		char *p;
		p = addr + size;
#if LIBC_SDIO
		p -= sizeof(fatdata_t);
		memcpy(&fatdata_glob, p, sizeof(fatdata_t));
		sdio_shl = MEM4(CHIPRAM_ADDR);
#endif
		// load sys_data
		p -= sizeof(sys_data);
		memcpy(&sys_data, p, sizeof(sys_data));
#ifdef APP_MEM_RESERVE
		size = app_mem_reserve(addr, size);
#endif
		_malloc_init(addr, size);
	}
	_stdio_init();
#if CHIPRAM_ARGS || LIBC_SDIO >= 3
	(void)arg_skip;
	{
		char *p = (char*)CHIPRAM_ADDR + 4;
		argc = _argv_copy(&argv, *(short*)p, p + sizeof(short));
	}
#else
	argc = _argv_init(&argv, arg_skip);
#endif
	sys_set_handlers();
#ifdef CXX_SUPPORT
	cxx_init(argc, argv);
#endif
	exit(main(argc, argv));
}

