#include "common.h"
#include "cmd_def.h"
#include "usbio.h"
#include "syscode.h"

#include "init_sc6531e.h"
#include "init_sc6531da.h"
#include "init_sc6530.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void _debug_msg(const char *msg);
void _malloc_init(void *addr, size_t size);
void _stdio_init(void);

int sdmain(uint8_t *ram);

/* dcache must be disabled for this function */
static uint32_t get_ram_size(uint32_t addr) {
	uint32_t size = 1 << 20; /* start from 2MB */
	uint32_t v1 = 0x12345678;
	do {
		if (size >> 27) break;
		size <<= 1;
		MEM4(addr + size) = v1;
		MEM4(addr) = size;
	} while (MEM4(addr + size) == v1);
	return size;
}

extern char __bss_start[];
extern uint32_t _start[];

static int read_key(int chip) {
	keypad_base_t *kpd = KEYPAD_BASE;
	uint32_t tmp, add, rst;
	int ret;
	tmp = 0x8b0010a8; add = 0x1000;
#if CHIP
	(void)chip;
	if (CHIP != 1)
#else
	if (chip != 1)
#endif
		tmp = 0x8b0000a0, add = 4;
	MEM4(tmp) = 0x80040; // keypad power on
	rst = tmp + 0x60 - 0xa0;
	// reset
	MEM4(rst) = 1;
	DELAY(10)
	MEM4(rst + add) = 1;
	tmp += add; // power off

	kpd->int_clr = 0xfff;
	kpd->polarity = 0xffff;
	kpd->clk_divide = 0;
	kpd->debounce = 15;
	kpd->ctrl |= 1; // keypad enable
	sys_wait_ms(20);
#if LIBC_SDIO >= 3
	ret = 0;
#else
	ret = 0x100;
#endif
	if (kpd->int_raw & 1) ret = kpd->key_status & 0x77;
	kpd->int_clr = 0xfff;
	MEM4(tmp) = 0x40;	// keypad power off
	return ret;
}

void entry_main(uint32_t load_addr) {
	uint32_t fw_addr, ram_addr, ram_size;
	int chip, key;

#if CHIP
	chip = CHIP;
#else
	{
		uint32_t tmp = MEM4(0x205003fc) - 0x65300000;
		chip = 1;
		if (!(tmp >> 17))
			chip = (int32_t)(tmp << 15) < 0 ? 2 : 3;
	}
#endif

#if LIBC_SDIO < 3
	sys_wait_ms(1000);
#endif
	key = read_key(chip);
#if LIBC_SDIO >= 3
	key |= load_addr >> 30; // boot from CHIPRAM
	if (!key) goto end;
#endif

	if (chip == 1) init_sc6531e();
	if (chip == 2) init_sc6531da();
	if (chip == 3) init_sc6530();

	fw_addr = load_addr & 0xf0000000;
	if (fw_addr >> 30) // if run from CHIPRAM
		fw_addr = (chip == 1 ? 1 : 3) << 28;

#if LIBC_SDIO < 3
	memset(__bss_start, 0, 0x4000);
#endif
#if !CHIP
	_chip = chip;
#endif

#if LIBC_SDIO < 3
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

	ram_addr = fw_addr + 0x04000000;
	ram_size = get_ram_size(ram_addr);
#if INIT_MMU
	{
		volatile uint32_t *tab = (volatile uint32_t*)(ram_addr + ram_size - 0x4000);
		unsigned i;
		for (i = 0; i < 0x1000; i++)
			tab[i] = i << 20 | 3 << 10 | 0x12; // section descriptor
		// cached=1, buffered=1
		tab[CHIPRAM_ADDR >> 20] |= 3 << 2;
		for (i = 0; i < (ram_size >> 20); i++)
			tab[ram_addr >> 20 | i] |= 3 << 2;
		tab[load_addr >> 20] |= 3 << 2;
		enable_mmu((uint32_t*)tab, -1);
	}
#endif
#if LIBC_SDIO < 3
#if INIT_MMU
	ram_size -= 0x4000; // MMU table
#endif
	{
		uint32_t offs = ram_size - 0x20000; // 128KB
		char *addr = (void*)(ram_addr + offs);
		uint32_t size = ram_size - offs;
		_malloc_init(addr, size);
		_stdio_init();
		printf("malloc heap: %u bytes\n", size);
	}
	printf("key = 0x%02x\n", key);
#endif
	sdmain((uint8_t*)ram_addr);
end:
	{
		uint32_t old = _start[1];
		if (~old) ((void(*)(void))old)();
	}
	for (;;);
}

