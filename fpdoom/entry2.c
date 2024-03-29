#include "common.h"
#include "cmd_def.h"
#include "usbio.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void _debug_msg(const char *msg);
void _malloc_init(void *addr, size_t size);
void _stdio_init(void);
int _argv_init(char ***argvp, int skip);

int main(int argc, char **argv);

#ifdef CXX_SUPPORT
#include "cxx_init.h"
#endif

void entry_main2(char *image_addr, uint32_t image_size, uint32_t bss_size, int arg_skip) {
	int argc; char **argv = NULL;
	uint32_t fw_addr = (uint32_t)image_addr & 0xf0000000;
	uint32_t ram_addr = fw_addr + 0x04000000;
	uint32_t ram_size = MEM4(ram_addr);

	memset(image_addr + image_size, 0, bss_size);

#if !CHIP
	if (fw_addr == 0x30000000) {
		uint32_t t0 = MEM4(0x205003fc);
		_chip = (int32_t)(t0 << 15) < 0 ? 2 : 3;
		chip_fn[0] = chip_fn[1];
	} else _chip = 1;
	usb_init_base();
#endif

	// usb_init();
	_debug_msg("entry2");
	{
		char *addr = image_addr + image_size + bss_size;
		size_t size = ram_addr + ram_size - (uint32_t)addr;
#if INIT_MMU
		size -= 0x4000; // MMU table
#endif
		// load sys_data
		sys_data = *(struct sys_data*)(addr + size - sizeof(sys_data));
		_malloc_init(addr, size);
	}
	_stdio_init();
	argc = _argv_init(&argv, arg_skip);
#ifdef CXX_SUPPORT
	cxx_init(argc, argv);
#endif
	exit(main(argc, argv));
}

