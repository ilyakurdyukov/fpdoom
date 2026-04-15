#include <unistd.h>
#define EVENT_QUIT SYS_EVENT_QUIT
#define EVENT_END SYS_EVENT_END
#include <syscode.h>
#undef EVENT_QUIT
#undef EVENT_END
#define sys_init game_sys_init

extern uint16_t *framebuf;
void scr_tile_update(unsigned x, unsigned y, unsigned rgb, unsigned t, void *dest);
void framebuffer_init(void);

#if EMBEDDED == 2
int usleep(unsigned delay) { sys_wait_us(delay); return 0; }
#endif

uint64_t get_time_usec(void) { return sys_timer_ms() * 1000ll; }

static int game_colors[GAME_COLORS] = {
	0x000000, // BG: black
	0x7f7f7f, // border: grey
	0x7f7f7f, // gameover: grey
	0xff8000, // J: green / 2 + red
	0x00ff00, // T: green
	0x80ff00, // L: green + red / 2
	0x0000ff, // I: blue
	0xff00ff, // Z: magenta
	0x00ffff, // S: cyan
	0xff0000, // O: red
};

static int term_colors[GAME_COLORS] = {
	0x000000, // BG: black
	0x000000, // border: grey
	0x000000, // gameover: grey
	0xc4a000, // J: green / 2 + red
	0x4e9a06, // T: green
	0x8ae234, // L: green + red / 2
	0x3465a4, // I: blue
	0x75507b, // Z: magenta
	0x06989a, // S: cyan
	0xcc0000, // O: red
};

#define MAX_EVENTS (EVENT_QUIT + 1)

typedef struct {
	unsigned repeat_ms;
	int i_event;
	uint8_t st[MAX_EVENTS];
	uint32_t timers[MAX_EVENTS];
	int8_t events[MAX_EVENTS];
} jsctx_t;

typedef struct {
	jsctx_t js;

	unsigned old_lines;
	char field[20][20], field1[20][20];
	char notermgfx, colormode;
	char next_num, next_rev;
	char ascii_screen[20 * 20];
} sysctx_t;

static sysctx_t *sys_glob;

static void sys_close(sysctx_t *sys) { (void)sys; }

static void sys_init(sysctx_t *sys) {

	{
		jsctx_t *js = &sys->js;
		js->i_event = MAX_EVENTS;
		memset(js->st, 0, sizeof(js->st));
		memset(js->events, 0, sizeof(js->events));
	}


	{
		char *p = sys->ascii_screen;
		unsigned i;
		memset(p, ' ', sizeof(sys->ascii_screen));
#define X(y, x) p + ((y) * 20 + (x))
#define SET(y, x, s) memcpy(X(y, x), s, sizeof(s) - 1)
		for (i = 0; i < 19; i++)
			SET(i, 0, "<##########>");
		SET(i, 0, "{__________}");

		for (i = 0; i < 2; i++)
			SET(17 + i, 14, "*****");

		// level digits
		for (i = 0; i < 5; i++)
			SET(11 + i, 13, "ooooooo");

		// next
		if (sys->next_num) {
			unsigned n = sys->next_num;
			unsigned nx = 13, ny = n < 3 ? 1 : 0;
			if (sys->next_rev) ny += (n - 1) * 3;
			for (i = 0; i < 8; i++)
				SET(1 + i, nx, "<####>");
			SET(ny, nx, "(----)");
			n = n == 1 ? 4 : 2;
			SET(ny + n + 1, nx, "{____}");
		}

		if (0) {// debug
			for (i = 0; i < 20; i++)
				printf("%.*s\n", 20, X(i, 0));
		}
#undef SET
#undef X
		{
			const char *tiles = " #(){}-_<>o*", *ret;
			for (i = 0; i < 20 * 20; i++)
				ret = strchr(tiles, p[i]),
				p[i] = ret ? ret - tiles : 0;
		}
	}

	memset(sys->field1, -1, sizeof(sys->field));
	{
		char (*p)[20] = sys->field;
		unsigned i, n = sys->next_num;

		memset(p, 0, sizeof(sys->field));
		for (i = 0; i < 19; i++) p[i][0] = p[i][11] = 1;
		memset(p[i], 1, 12);

		if (n) {
			unsigned nx = 13, ny = n < 3 ? 1 : 0;
			if (sys->next_rev) ny += (n - 1) * 3;
			n = n == 1 ? 4 : 2;
			memset(p[ny] + nx, 1, 6);
			for (i = 1; i < 1 + n; i++)
				p[ny + i][nx] = p[ny + i][nx + 5] = 1;
			memset(p[ny + i] + nx, 1, 6);
		}
	}
	sys->old_lines = ~0;

	framebuffer_init();
	sys_start();
}

static void draw_digit(char *p, int num) {
	const char *font = &(num * 4)[
		".@. .@. @@@ @@@ @.@ @@@ .@@ @@@ @@@ @@@"
		"@.@ @@. ..@ ..@ @.@ @.. @.. ..@ @.@ @.@"
		"@.@ .@. @@@ @@@ @@@ @@@ @@@ .@. @@@ @@@"
		"@.@ .@. @.. ..@ ..@ ..@ @.@ .@. @.@ ..@"
		".@. @@@ @@@ @@@ ..@ @@@ @@@ .@. @@@ @@."];
	int x, y;
	for (x = 0; x < 3; x++)
	for (y = 0; y < 5; y++)
		p[y * 20 + x] = font[y * 39 + x] == '@' ? 1 : 0;
}

static void sys_update_score(sysctx_t *sys, game_t *T) {
	unsigned i, j, k, x1 = 13, y1 = 11;
	char (*p)[20] = sys->field;

	for (i = 0; i < 5; i++)
		memset(p[y1 + i] + x1, 0, 7);

	k = T->lines; j = k % 10; k /= 10;
	for (i = 0; i < 5; i++)
		p[17][14 + i] = j > i ? 1 : 0;
	for (i = 0; i < 5; i++)
		p[18][14 + i] = j > i + 5 ? 1 : 0;

	if (k < 10) draw_digit(p[y1] + x1 + 2, k);
	else {
		if (k > 99) k = 99;
		draw_digit(p[y1] + x1, k / 10);
		draw_digit(p[y1] + x1 + 4, k % 10);
	}
}

static void sys_refresh(sysctx_t *sys, game_t *T) {
	unsigned x, y, w = 10, h = 19;
	char *s = T->field + (w + 1);
	char (*p)[20] = sys->field;

	// copy playfield
	for (y = 0; y < h; y++)
		memcpy(p[y] + 1, s + y * (w + 1), w);
	// copy next
	{
		unsigned n = sys->next_num;
		unsigned nx = 13 + 1, ny = 4 - n;
		for (x = 0; x < n; x++, ny += 3) {
			unsigned a = x;
			if (sys->next_rev) a = n - 1 - a;
			a = (T->next >> a * 3) & 7;
			for (y = 0; y < 2; y++)
				memcpy(p[ny + y] + nx,
						game_pieces[a] + y * 4, 4);
		}
	}

	if (sys->old_lines != T->lines) {
		sys->old_lines = T->lines;
		sys_update_score(sys, T);
	}

	sys_wait_refresh();

	for (y = 0; y < 20; y++) {
		for (x = 0; x < 20; x++) {
			int t = p[y][x], c;
			if (t == sys->field1[y][x]) continue;
			if (!sys->colormode) c = game_colors[t], t = t > 1;
			else {
				c = 0;
				if (sys->colormode != 2) c = term_colors[t];
				if (t) t = sys->ascii_screen[y * 20 + x];
			}
			scr_tile_update(x, y, c, t, framebuf);
		}
	}

	sys_start_refresh();
	memcpy(sys->field1, sys->field, sizeof(sys->field));
}

static inline void joy_release_event(int8_t *ev) {
	int x = *ev;
	*ev = (x - 1) & ((x - 2) | 0x7f);
}

static int sys_events(sysctx_t *sys) {
	jsctx_t *js = &sys->js;
	if (js->i_event < MAX_EVENTS) goto check_events;
	for (;;) {
		int val, ev;
		val = sys_event(&ev);
		if (val == SYS_EVENT_QUIT) return EVENT_QUIT;
		if (val == SYS_EVENT_END) break;
		val = val == EVENT_KEYDOWN;
		if ((unsigned)ev >= MAX_EVENTS) {
			if (ev == EVENT_REWIND && val) return ev;
			continue;
		}
		if (js->st[ev] == val) continue;
		js->st[ev] = val;
		if (!val) joy_release_event(&js->events[ev]);
		else if (!js->events[ev]++) {
			js->timers[ev] = get_time_usec();
			if (ev < EVENT_RESTART) return ev;
		}
	}
	js->i_event = EVENT_DOWN;
check_events:
	{
		int i = js->i_event;
		uint32_t t1 = get_time_usec();
		uint32_t r = js->repeat_ms * 1000;
		for (; i <= EVENT_RIGHT; i++)
			if (js->events[i] && t1 - js->timers[i] >= r) {
				js->timers[i] += r;
				js->i_event = i;
				return i;
			}
		r = 1000 * 1000;
		for (i = EVENT_RESTART; i <= EVENT_QUIT; i++)
			if (js->events[i] > 0 && t1 - js->timers[i] >= r) {
				js->events[i] |= 0x80; // no repeat
				js->i_event = i;
				return i;
			}
		js->i_event = i;
	}
	return EVENT_END;
}

