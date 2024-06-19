#ifndef __SIGNAL
#define __SIGNAL

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);

#define SIGINT 2

#endif
