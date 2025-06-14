#ifndef SDL_h_
#define SDL_h_

typedef void *SDL_Event;
typedef void *SDL_Window;

static inline void SDL_Quit(void) {}

#define SDL_ShowSimpleMessageBox(flags, title, message, window) \
	fprintf(stderr, "%s\n", message)

static inline int SDL_SetHint(const char *name, const char *value) {
	return 0;
}

#endif
