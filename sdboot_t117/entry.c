#include "common.h"
#include "cmd_def.h"
#include "usbio.h"
#include "syscode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "init_t117.h"

void _debug_msg(const char *msg);
void _malloc_init(void *addr, size_t size);
void _stdio_init(void);

int sdmain(void);

/* dcache must be disabled for this function */
static uint32_t get_ram_size(uint32_t addr) {
	uint32_t v1 = 0x12345678, i = 3;
	do MEM4(addr + (i << 24)) = i + v1; while (--i);
	do i++; while (i < 4 && MEM4(addr + (i << 24)) == i + v1);
	return i << 24; // 16M, 32M, 48M, 64M
}

extern char __bss_start[];
extern char __bss_end[];
extern uint32_t _start[];

void init_chip(void);
int sdram_init(void);

void entry_main(void) {
	uint32_t ram_addr = 0x80000000, ram_size;

	init_t117();

#if LIBC_SDIO < 3
	// clear USB interrupts from BROM
	{
		uint32_t b = 0x20201be0 + (15 + 6) * 32;
		MEM4(b + 8) |= 0x14000000;
	}
	memset(__bss_start, 0, __bss_end - __bss_start);
	{
		static const uint8_t fdl_ack[8] = {
			0x7e, 0, 0x80, 0, 0, 0xff, 0x7f, 0x7e };
		static const struct {
			uint8_t hdr[5], str[0x22], end[5];
		} fdl_ver = {
			{ 0x7e, 0,0x81, 0,0x22 },
			{ "Spreadtrum Boot Block version 1.2" },
			{ 0xa9,0xe9, 0x7e }
		};
		uint8_t buf[8];
		usb_init();
		usb_write(&fdl_ver, sizeof(fdl_ver));
		usb_read(buf, 8, USB_WAIT);
		usb_write(fdl_ack, sizeof(fdl_ack));
	}
	{
		char buf[4]; uint32_t t0, t1;
		for (;;) {
			usb_read(buf, 1, USB_WAIT);
			if (buf[0] == HOST_CONNECT) break;
		}
		t0 = sys_timer_ms();
		do {
			t1 = sys_timer_ms();
			if (usb_read(buf, 4, 0)) t0 = t1;
		} while (t1 - t0 < 10);
	}
	_debug_msg("sdboot");
#endif

	if (sdram_init()) {
#if LIBC_SDIO < 3
		_debug_msg("sdram_init failed");
#endif
		for (;;);
	}

	ram_size = get_ram_size(ram_addr);
#if INIT_MMU
	{
		volatile uint32_t *tab = (uint32_t*)ram_addr;
		uint32_t cb = 3 << 2; // cached=1, buffered=1
		unsigned i;
		for (i = 0; i < 0x200; i++) tab[i] = 0; // no access
		for (; i < 0x800 + (ram_size >> 20); i++)
			tab[i] = i << 20 | 3 << 10 | 2; // read/write
		for (; i < 0x1000; i++) tab[i] = 0; // no access

		i = 0x00000000 >> 20; // IRAM
		tab[i] = i << 20 | 3 << 10 | cb | 2; // read/write

		for (i = 0; i < ram_size >> 20; i++)
			tab[ram_addr >> 20 | i] |= cb;
		// domains: 0 - manager, 1..3 - client, 4..15 - no access
		enable_mmu((uint32_t*)tab, 0x57);
	}
#endif
#if LIBC_SDIO < 3
	{
		char *addr = (void*)(ram_addr + 0x10000);
		uint32_t size = 0x20000; // 128KB
		_malloc_init(addr, size);
		_stdio_init();
		printf("malloc heap: %u bytes\n", size);
	}
#endif
	sdmain();
	sys_wdg_reset(SYS_RESET_DELAY);
}

