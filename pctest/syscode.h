#include <stdint.h>

#define FPBIN_DIR ""

extern struct sys_data {
	struct sys_display { uint16_t w1, h1, w2, h2; } display;
	uint16_t keytrn[2][64];
	uint16_t mac;
	uint8_t scaler, rotate;
	uint8_t *framebuf;
	uint8_t user[4];
} sys_data;

enum {
	EVENT_KEYDOWN, EVENT_KEYUP, EVENT_END, EVENT_QUIT
};
int sys_event(int *rkey);
void sys_framebuffer(void *base);
void sys_start(void);
void sys_wait_refresh(void);
void sys_start_refresh(void);

unsigned sys_timer_ms(void);
void sys_wait_ms(uint32_t delay);
void sys_wait_us(uint32_t delay);
int sys_getkeymap(uint8_t *dest);

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

