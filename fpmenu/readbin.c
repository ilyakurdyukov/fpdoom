
#undef SDIO_VERBOSE
#define SDIO_VERBOSE 0

#undef SDIO_SHL
#define SDIO_SHL CHIPRAM_ADDR

#include "common.h"
#include "syscode.h"

#if UMS9117
#include "../ums9117/sdio.c"
#else
#include "sdio.c"
#endif
#define FAT_READ_SYS \
	if (sdio_read_block(sector, buf)) break;
#include "microfat.c"

#undef DBG_LOG
#if MENU_DEBUG
#define DBG_LOG(...) printf(__VA_ARGS__)
#define RAM_SHIFT 0x20000
#define DBG_EXTRA_ARG(x) , x
#else
#define DBG_LOG(...) (void)0
#define RAM_SHIFT 0
#define DBG_EXTRA_ARG(x)
#endif

typedef int (*printf_t)(const char *fmt, ...);

void entry_main(unsigned clust, unsigned size, uint8_t *ram,
		fatdata_t *old_fatdata DBG_EXTRA_ARG(printf_t printf)) {
	fatdata_t fatdata;
	uint32_t i = 0, n = sizeof(fatdata_t) / 4;
	// fatdata = *old_fatdata;
	do ((uint32_t*)&fatdata)[i] = ((uint32_t*)old_fatdata)[i];
	while (i++ < n);

	DBG_LOG("clust = 0x%x, size = 0x%x, ram = %p\n", clust, size, ram);
	ram += RAM_SHIFT;

	n = fat_read_simple(&fatdata, clust, ram, size);
	DBG_LOG("n = 0x%x\n", n);
	if (n < size) for (;;);
#if RAM_SHIFT
	ram -= RAM_SHIFT;
	memcpy(ram, ram + RAM_SHIFT, size);
#endif
	clean_dcache();
	clean_icache();
#if UMS9117
	ram += 0x200;
#endif
	((void(*)(void))ram)();
}
