#ifndef SYSCODE_H
#define SYSCODE_H

#define INIT_MMU 1

#define CHIPRAM_ADDR 0x40000000
#if CHIP
#if CHIP == 1
#define FIRMWARE_ADDR 0x10000000
#elif CHIP == 2 || CHIP == 3
#define FIRMWARE_ADDR 0x30000000
#endif
#define RAM_ADDR (FIRMWARE_ADDR + 0x4000000)
#endif
#define FIRMWARE_SIZE (4 << 20)
#define RAM_SIZE (4 << 20)

#if !CHIP
#define CHIP_FN(name) (_chip != 1 ? sc6531da_##name : sc6531e_##name)
#elif CHIP == 1 // SC6531E
#define CHIP_FN(name) sc6531e_##name
#elif CHIP == 2 // SC6531DA
#define CHIP_FN(name) sc6531da_##name
#elif CHIP == 3 // SC6530
#define CHIP_FN(name) sc6530_##name
#else
#error
#endif

/* from assembly code */ 
void enable_mmu(uint32_t *table, uint32_t domain);

void scan_firmware(intptr_t fw_addr);
unsigned sys_timer_ms(void);
void sys_wait_ms(uint32_t delay);
void sys_wait_us(uint32_t delay);
void sys_wait_clk(uint32_t delay);
int sys_getkeymap(uint8_t *dest);

void clean_dcache(void);
void clean_invalidate_dcache_range(void *start, void *end);

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
	struct { uint32_t num, ver, adi; } chip_id;
	struct sys_display { uint16_t w1, h1, w2, h2; } display;
	int brightness;
	uint16_t keytrn[2][64];
	uint8_t keycols;
	uint8_t scaler;
	/* rotate = key << 4 | screen */
	uint8_t rotate, lcd_cs;
	uint8_t spi_mode;
	int8_t charge;
	uint8_t gpio_init;
	uint8_t bl_gpio;
	uint16_t mac;
	uint32_t spi;
	uint32_t lcd_id;
} sys_data;

extern struct chip_fn2 chip_fn[];

// appinit

void keytrn_init(void);
void lcd_appinit(void);

uint8_t* framebuffer_init(void);
extern void (*app_pal_update)(uint8_t *pal, void *dest, const uint8_t *gamma);
extern void (*app_scr_update)(uint8_t *src, void *dest);

#define KEYPAD_ENUM(M) \
	M(0x01, DIAL) \
	M(0x04, UP) M(0x05, DOWN) M(0x06, LEFT) M(0x07, RIGHT) \
	M(0x08, LSOFT) M(0x09, RSOFT) \
	M(0x0d, CENTER) \
	M(0x23, HASH) M(0x2a, STAR) M(0x2b, PLUS) \
	M(0x30, 0) M(0x31, 1) M(0x32, 2) M(0x33, 3) M(0x34, 4) \
	M(0x35, 5) M(0x36, 6) M(0x37, 7) M(0x38, 8) M(0x39, 9)

#endif // SYSCODE_H
