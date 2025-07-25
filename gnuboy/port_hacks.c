#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "pcm.h"
#include "rc.h"
#include "sys.h"

#include "syscode.h"

struct pcm pcm;

static byte buf[4096];

rcvar_t pcm_exports[] = {
	RCV_END
};

void pcm_init() {
	pcm.hz = 11025;
	pcm.buf = buf;
	pcm.len = sizeof(buf);
	pcm.pos = 0;
}

void pcm_close() {
	memset(&pcm, 0, sizeof(pcm));
}

int pcm_submit() {
	pcm.pos = 0;
	return 0;
}

void pcm_pause(int dopause) {}

rcvar_t joy_exports[] = {
	RCV_END
};

void joy_init() {}
void joy_close() {}
void joy_poll() {}

void *sys_timer() {
	uint32_t *tv;
	tv = malloc(sizeof(*tv));
	*tv = sys_timer_ms();
	return tv;
}

int sys_elapsed(struct timeval *prev) {
	uint32_t now, ms;
	now = sys_timer_ms();
	ms = now - *(uint32_t*)prev;
	*(uint32_t*)prev = now;
	return ms * 1000;
}

void sys_sleep(int us) {
	if (us > 0) sys_wait_us(us);
}

void sys_checkdir(char *path, int wr) {}

void sys_initpath() {
	char *buf = ".";
	rc_setvar("rcpath", 1, &buf);
	rc_setvar("savedir", 1, &buf);
}

void sys_sanitize(char *s) {}

#if NO_MENU
#include "menu.h"

rcvar_t menu_exports[] = {
	RCV_END
};

void menu_init(void) {}
void menu_initpage(enum menu_page page) {}
void menu_enter(void) {}
#endif

