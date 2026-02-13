#ifndef SYSCODE_H
#define SYSCODE_H

#include <stdint.h>

#ifndef INIT_MMU
#define INIT_MMU 1
#endif

#if UMS9117
//#define CHIPRAM_ADDR 0
// not chipram, used to store app args
#define CHIPRAM_ADDR 0x80004000
#define FPBIN_DIR "fpbin_t117/"

#define KEYPAD_BASE ((keypad_base_t*)0x40250000)
#define LCDC_BASE ((lcdc_base_t*)0x20800000)
#else
#define CHIPRAM_ADDR 0x40000000
#define FIRMWARE_SIZE (4 << 20)
#define FPBIN_DIR "fpbin/"

#define KEYPAD_BASE ((keypad_base_t*)0x87000000)
#define LCDC_BASE ((lcdc_base_t*)0x20d00000)
#endif

typedef volatile struct {
	uint32_t ctrl, int_en, int_raw, int_mask;
	uint32_t int_clr, dummy_14, polarity, debounce;
	uint32_t long_key, sleep_cnt, clk_divide, key_status;
	uint32_t sleep_stat, dbg_stat1, dbg_stat2;
} keypad_base_t;

typedef volatile struct {
	uint32_t ctrl, disp_size, lcm_start, lcm_size;
	uint32_t bg_color, fifo_status, sync_delay, dummy[1];
	struct { // 0x20
		uint32_t ctrl, y_base_addr, uv_base_addr, size_xy;
		uint32_t pitch, disp_xy, dummy[6];
	} img;
	struct { // 0x50, 0x80, 0xb0
		uint32_t ctrl, base_addr, alpha_base_addr, size_xy;
		uint32_t pitch, disp_xy, alpha, grey_rgb;
		uint32_t ck, dummy[3];
	} ocd1, ocd2, ocd3;
	struct { // 0x80
		uint32_t ctrl, base_addr, start_xy, size_xy;
		uint32_t pitch, dummy[3];
	} cap;
	struct { // 0x100
		uint32_t ctrl, contrast, saturation, brightness;
	} y2r;
	struct { // 0x110
		uint32_t en, clr, status, raw;
	} irq;
} lcdc_base_t;

/* from assembly code */ 
void enable_mmu(uint32_t *table, uint32_t domain);
void set_cpsr_c(uint32_t a);

int sys_lzma_decode(const uint8_t *src, unsigned src_size,
		uint8_t *dst, unsigned dst_size);
void scan_firmware(intptr_t fw_addr);
uint32_t sys_timer_ms(void);
void sys_wait_ms(uint32_t delay);
void sys_wait_us(uint32_t delay);
void sys_wait_clk(uint32_t delay);
int sys_getkeymap(uint8_t *dest);

uint32_t adi_read(uint32_t addr);
void adi_write(uint32_t addr, uint32_t val);
void sys_init(void);
void sys_start(void);
int sys_event(int *rkey);
void sys_brightness(unsigned val);
void sys_framebuffer(void *base);
void sys_start_refresh(void);
void sys_wait_refresh(void);
__attribute__((noreturn))
void sys_exit(void);
__attribute__((noreturn))
#define SYS_RESET_DELAY (0x8000 / 2) /* 0.5 sec */
void sys_wdg_reset(unsigned val);

void invalidate_icache(void);
void clean_dcache(void);
void clean_dcache_range(void *start, void *end);
void clean_invalidate_dcache(void);
void clean_invalidate_dcache_range(void *start, void *end);

enum {
	EVENT_KEYDOWN, EVENT_KEYUP, EVENT_END, EVENT_QUIT
};

// extern

extern struct sys_data {
	short *keymap_addr;
	struct { uint32_t num, ver, adi; } chip_id;
	struct sys_display { uint16_t w1, h1, w2, h2; } display;
	uint16_t keytrn[2][64]; // must be aligned by 4
	uint8_t brightness, init_done;
	uint8_t keyrows, keycols, keyflags;
	uint8_t scaler;
	/* rotate = key << 4 | screen */
	uint8_t rotate, lcd_cs;
	uint8_t spi_mode;
	int8_t charge;
	uint8_t gpio_init;
#if UMS9117
	uint8_t bl_extra[4];
#else
	uint8_t bl_gpio;
#endif
	uint8_t page_reset;
	uint16_t mac;
	uint32_t spi;
	uint32_t lcd_id;
	uint8_t *framebuf;
	uint8_t user[4];
} sys_data;

// appinit

void keytrn_init(void);
void lcd_appinit(void);

#define KEYPAD_ENUM(M) \
	M(0x01, DIAL) \
	M(0x04, UP) M(0x05, DOWN) M(0x06, LEFT) M(0x07, RIGHT) \
	M(0x08, LSOFT) M(0x09, RSOFT) M(0x0d, CENTER) \
	M(0x0e, CAMERA) M(0x1d, EXT_1D) M(0x1f, EXT_1F) \
	M(0x21, EXT_21) M(0x23, HASH) M(0x24, VOLUP) M(0x25, VOLDOWN) \
	M(0x29, EXT_29) M(0x2a, STAR) M(0x2b, PLUS) \
	M(0x2c, EXT_2C) M(0x2d, MINUS) M(0x2f, EXT_2F) \
	M(0x30, 0) M(0x31, 1) M(0x32, 2) M(0x33, 3) M(0x34, 4) \
	M(0x35, 5) M(0x36, 6) M(0x37, 7) M(0x38, 8) M(0x39, 9)

#endif // SYSCODE_H
