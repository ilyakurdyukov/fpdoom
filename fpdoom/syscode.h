#ifndef SYSCODE_H
#define SYSCODE_H

#define INIT_MMU 1

#define CHIPRAM_ADDR 0x40000000
#if CHIP
#if CHIP == 1
#define FIRMWARE_ADDR 0x10000000
#elif CHIP == 2
#define FIRMWARE_ADDR 0x30000000
#endif
#define RAM_ADDR (FIRMWARE_ADDR + 0x4000000)
#endif
#define FIRMWARE_SIZE (4 << 20)
#define RAM_SIZE (4 << 20)

#if !CHIP
#define CHIP_FN(name) (_chip == 2 ? sc6531da_##name : sc6531e_##name)
#elif CHIP == 1 // SC6531E
#define CHIP_FN(name) sc6531e_##name
#elif CHIP == 2 // SC6531DA
#define CHIP_FN(name) sc6531da_##name
#else
#error
#endif

/* from assembly code */ 
void enable_mmu(uint32_t *table, uint32_t domain);

void scan_firmware(intptr_t fw_addr);
unsigned sys_timer_ms(void);
unsigned sys_wait_ms(unsigned delay);

enum {
	EVENT_KEYDOWN, EVENT_KEYUP, EVENT_END, EVENT_QUIT
};

#if !CHIP
#define CHIP_FN_DECL(pre, name, arg) \
	pre sc6531e_##name arg; \
	pre sc6531da_##name arg;
#define CHIP_FN2(name) (chip_fn[0].name)
#else
#define CHIP_FN_DECL(pre, name, arg) pre CHIP_FN(name) arg;
#define CHIP_FN2(name) CHIP_FN(name)
#endif

CHIP_FN_DECL(void, sys_init, (void))
CHIP_FN_DECL(void, sys_start, (void))
CHIP_FN_DECL(int, sys_event, (int *rkey))
CHIP_FN_DECL(void, sys_brightness, (unsigned val))
CHIP_FN_DECL(void, sys_framebuffer, (void*))
CHIP_FN_DECL(void, sys_start_refresh, (void))
CHIP_FN_DECL(void, sys_wait_refresh, (void))
#undef CHIP_FN_DECL

struct chip_fn2 {
	void (*sys_start)(void);
	int (*sys_event)(int *rkey);
	void (*sys_framebuffer)(void *base);
	void (*sys_brightness)(unsigned val);
	void (*sys_start_refresh)(void);
	void (*sys_wait_refresh)(void);
};

// extern

extern uint32_t *pinmap_addr;

extern struct sys_data {
	short *keymap_addr;
	struct { uint32_t num, ver, ahb, analog; } chip_id;
	struct sys_display { uint16_t w1, h1, w2, h2; } display;
	int brightness;
	uint16_t keytrn[65];
	/* key << 4 | screen */
	uint8_t rotate, lcd_cs;
	uint16_t mac;
	uint32_t spi;
} sys_data;

extern struct chip_fn2 chip_fn[];

// appinit

void keytrn_init(void);
void lcd_appinit(void);

#endif // SYSCODE_H
