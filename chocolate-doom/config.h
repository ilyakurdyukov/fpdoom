#define PACKAGE_NAME "Chocolate Doom"
#define PACKAGE_TARNAME "chocolate-doom"
#define PACKAGE_VERSION "3.1.0"
#define PACKAGE_STRING PACKAGE_NAME " " PACKAGE_VERSION
#define PROGRAM_PREFIX "chocolate-"

/* #undef HAVE_FLUIDSYNTH */
/* #undef HAVE_LIBSAMPLERATE */
/* #undef HAVE_LIBPNG */
#if EMBEDDED == 1 && GAME_STRIFE
#define HAVE_DIRENT_H
#endif
#define HAVE_DECL_STRCASECMP 1
#define HAVE_DECL_STRNCASECMP 1
